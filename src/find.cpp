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
    {
    }

    bool             m_ready;
    std::string      m_output;
    boost::condition m_honk;
};

struct mt_search
{
    mt_search(database& db)
      : m_db(db)
      , m_next_chunk(0)
      , m_next_print(0)
    {
    }

    database&               m_db;
    int                     m_next_chunk;
    int                     m_next_print;
    std::vector<mt_thread*> m_chunkinfo;
    boost::mutex            m_mutex;
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
        int current_chunk = 0;

        // Lock mutex while reading a chunk from the db
        {
            boost::mutex::scoped_lock lock(mts.m_mutex);
            if(!mts.m_db.next_chunk(compressed))
                break;

            mts.m_chunkinfo.push_back(&tinfo);
            current_chunk = mts.m_next_chunk++;
        }

        // Decompress chunk and create a chunk wrapper
        snappy_uncompress(compressed.first, compressed.second, uncompressed);
        db_chunk chunk(uncompressed);

        tinfo.m_output.clear();
        search_chunk(chunk, *re, *file_re, prefix_size, receiver);

        // Lock mutex to output result
        boost::mutex::scoped_lock lock(mts.m_mutex);

        if(current_chunk == mts.m_next_print)
        {
            // We're next in line to print our result, so we do that
            std::cout << tinfo.m_output;
            ++mts.m_next_print;

            // Then we check if there are any other threads with results
            // ready. If so, we print that as well and wake them up.
            while(mts.m_next_print < int(mts.m_chunkinfo.size()) && mts.m_chunkinfo[mts.m_next_print]->m_ready)
            {
                std::cout << mts.m_chunkinfo[mts.m_next_print]->m_output;

                mts.m_chunkinfo[mts.m_next_print]->m_ready = false;
                mts.m_chunkinfo[mts.m_next_print]->m_honk.notify_one();

                ++mts.m_next_print;
            }
        }
        else
        {
            tinfo.m_ready = true;
            tinfo.m_honk.wait(lock);
        }
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
