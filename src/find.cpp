// CodeDB - public domain - 2010 Daniel Andersson

#include "find.hpp"
#include "regex.hpp"
#include "config.hpp"
#include "options.hpp"
#include "compress.hpp"
#include "file_lock.hpp"
#include "database.hpp"
#include "profiler.hpp"
#include "search.hpp"

#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/condition.hpp>

#include <iostream>

namespace
{
    class string_receiver : public match_receiver
    {
      public:
        string_receiver(std::string& out, bool trim)
          : m_out(out)
          , m_trim(trim)
        {
        }

      private:

        const char* on_match(const match_info& match)
        {
            const char* line_start = match.m_line_start;
            if(m_trim)
            {
                while(*line_start == ' ' || *line_start == '\t')
                    ++line_start;
            }

            m_out += match.m_file;

            char lineno[16];
            std::sprintf(lineno, ":%u:", static_cast<unsigned>(match.m_line));

            m_out += lineno;
            m_out.append(line_start, match.m_line_end);

            return match.m_line_end;
        }

        std::string& m_out;
        bool         m_trim;
    };
}

struct mt_thread
{
    mt_thread()
      : m_ready(false)
      , m_next(0)
    {
    }

    bool             m_ready;
    std::string      m_output;
    mt_thread*       m_next;
    boost::condition m_honk;
};

struct mt_search
{
    mt_search(database& db)
      : m_db(db)
      , m_head(0)
      , m_tail(0)
    {
    }

    bool next_chunk(std::pair<const char*, const char*>& compressed, mt_thread* thread)
    {
        boost::mutex::scoped_lock lock(m_mutex);

        if(!m_db.next_chunk(compressed))
            return false;

        // If there's a current head (and it isn't us)
        if(m_head != 0 && m_head != thread)
            m_head->m_next = thread;

        // If there's no current tail, then it's us!
        if(m_tail == 0)
            m_tail = thread;
        
        // thread is now at the head of the queue
        thread->m_next = 0;
        m_head = thread;

        thread->m_ready = false;

        return true;
    }

    void report(mt_thread* thread)
    {
        boost::mutex::scoped_lock lock(m_mutex);

        if(m_tail != thread && thread->m_output.empty())
        {
            // find previous
            mt_thread* prev = m_tail;
            for(; prev; prev = prev->m_next)
                if(prev->m_next == thread)
                    break;

            if(prev)
                prev->m_next = thread->m_next;
            if(m_head == thread)
                m_head = prev;

            return;
        }

        thread->m_ready = true;

        if(m_tail == thread)
        {
            std::cout << thread->m_output;
            mt_thread* cur = thread->m_next;

            while(cur && cur->m_ready)
            {
                std::cout << cur->m_output;
                cur->m_honk.notify_one();
                cur = cur->m_next;
            }

            m_tail = cur;
        }
        else
        {
            thread->m_honk.wait(lock);
        }
    }

    database&    m_db;
    mt_thread*   m_head;
    mt_thread*   m_tail;
    boost::mutex m_mutex;
};

void search_db_mt(mt_search&  mts,
                  regex_ptr   re,
                  regex_ptr   file_re,
                  std::size_t prefix_size)
{
    mt_thread tinfo;

    std::pair<const char*, const char*> compressed;
    std::string uncompressed;

    string_receiver receiver(tinfo.m_output, false);

    for(;;)
    {
        if(!mts.next_chunk(compressed, &tinfo))
            break;

        // Decompress chunk and create a chunk wrapper
        snappy_uncompress(compressed.first, compressed.second, uncompressed);
        db_chunk chunk(uncompressed);

        tinfo.m_output.clear();
        search_chunk(chunk, *re, *file_re, prefix_size, receiver);

        mts.report(&tinfo);
    }
}

void find(const bfs::path& cdb_path, const options& opt)
{    
    config cfg = load_config(cdb_path / "config");

    const bfs::path cdb_root = cdb_path.parent_path();
    const bfs::path search_root = bfs::initial_path();

    std::size_t prefix_size = 0;
    std::string file_match;
    if(opt.m_options.count("-a") == 0 && search_root != cdb_root)
    {
        file_match = "^" + escape_regex(
            search_root.generic_string().substr(cdb_root.string().size() + 1) + "/") + ".*";
        prefix_size = search_root.string().size() - cdb_root.string().size();
    }

    file_lock lock(cdb_path / "lock");
    lock.lock_sharable();

    const char* find_regex_options =
        opt.m_options.count("-i") ? "i" : "";
    const char* file_regex_options =
        cfg.get_value("nocase-file-match") == "on" ? "i" : "";

    for(unsigned i = 0; i != opt.m_args.size(); ++i)
    {
        database_ptr db = open_database(cdb_path / "db");
        
        std::string pattern = opt.m_args[i];
        if(opt.m_options.count("-v"))
            pattern = escape_regex(pattern);

        mt_search mts(*db);

        boost::thread_group workers;

        if(unsigned hw_threads = boost::thread::hardware_concurrency())
        {
            for(unsigned i = 0; i != hw_threads-1; ++i)
            {
                workers.create_thread([&]
                {
                    search_db_mt(mts,
                                 compile_regex(pattern, 0, find_regex_options),
                                 compile_regex(file_match, 0, file_regex_options),
                                 prefix_size);
                });
            }
        }

        search_db_mt(mts,
                     compile_regex(pattern, 0, find_regex_options),
                     compile_regex(file_match, 0, file_regex_options),
                     prefix_size);

        workers.join_all();
    }
}
