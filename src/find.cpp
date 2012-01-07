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

            char lineno[24];
            std::sprintf(lineno, ":%u:", static_cast<unsigned>(match.m_line));

            m_out += lineno;
            m_out.append(line_start, match.m_line_end);

            return match.m_line_end;
        }

        std::string& m_out;
        bool         m_trim;
    };

    class mt_search
    {
      public:
        mt_search(database& db, bool trim, std::size_t prefix_size)
          : m_db(db)
          , m_head(0)
          , m_tail(0)
          , m_free(0)
          , m_data(8)
          , m_trim(trim)
          , m_prefix_size(prefix_size)
        {
            // Set up the free list
            for(auto i = m_data.begin(); i != m_data.end(); ++i)
            {
                i->m_next = m_free;
                m_free = &*i;
            }
        }

        void search_db(regex_ptr re, regex_ptr file_re, const char* id)
        {
            std::string uncompressed;

            profiler& p = make_profiler(id);

            // The worker thread loop consists of repeatedly asking for a
            // compressed chunk of data, uncompressing it and reporting the
            // reuslts. We do this until all chunks have been processed.
            for(;;)
            {
                thread_data* td = 0;
                {
                    profile_scope prof(p);
                    td = next_chunk();
                }
                if(td == 0)
                    break;

                snappy_uncompress(td->m_input.first, td->m_input.second, uncompressed);
                db_chunk chunk(uncompressed);

                string_receiver receiver(td->m_output, m_trim);
                search_chunk(chunk, *re, *file_re, m_prefix_size, receiver);

                report(td);
            }
        }

      private:

        struct thread_data
        {
            bool                                m_ready;
            std::pair<const char*, const char*> m_input;
            std::string                         m_output;
            thread_data*                        m_next;
            int                                 m_chunkid;
        };

        thread_data* next_chunk()
        {
            boost::mutex::scoped_lock lock(m_mutex);

            // First we grab a compressed chunk from the database, if there are
            // no chunks left then we're done.
            std::pair<const char*, const char*> compressed;
            if(!m_db.next_chunk(compressed))
                return 0;

            // Wait until there's a free chunk_data.
            while(m_free == 0)
                m_honk.wait(lock);

            // Remove it from the list of free chunks.
            thread_data* td = m_free;
            m_free = m_free->m_next;

            // Update head (and possibly tail) to reflect the new work-list.
            if(m_head && m_head != td)
                m_head->m_next = td;
            m_head = td;

            if(m_tail == 0)
                m_tail = td;

            td->m_output.clear();
            td->m_next    = 0;
            td->m_input   = compressed;
            td->m_ready   = false;

            return td;
        }

        void report(thread_data* tdata)
        {
            boost::mutex::scoped_lock lock(m_mutex);

            // if(m_tail != tdata && tdata->m_output.empty())
            // {
            //     // There's no point in waiting to output an empty result
            //     // chunk. In that case we remove ourself from the queue.
            //     thread_data* prev = m_tail;
            //     for(; prev; prev = prev->m_next)
            //         if(prev->m_next == tdata)
            //             break;

            //     if(prev)
            //         prev->m_next = tdata->m_next;
            //     if(m_head == tdata)
            //         m_head = prev;

            //     return;
            // }

            tdata->m_ready = true;

            if(m_tail == tdata)
            {
                // This chunk has data and is next in line to output.
                std::cout << tdata->m_output;
                thread_data* cur = tdata->m_next;

                // Push to freelist
                tdata->m_next = m_free;
                m_free = tdata;

                // Also spin through any other chunks that are ready. Write the
                // data and wake the thread that is waiting.
                while(cur && cur->m_ready)
                {
                    thread_data* tmp = cur;

                    std::cout << cur->m_output;
                    cur = cur->m_next;

                    tmp->m_next = m_free;
                    m_free = tmp;
                }

                m_tail = cur;

                if(m_tail == 0)
                    m_head = 0;

                m_honk.notify_all();
            }
        }

        database&                m_db;
        thread_data*             m_head;
        thread_data*             m_tail;
        thread_data*             m_free;
        std::vector<thread_data> m_data;
        bool                     m_trim;
        std::size_t              m_prefix_size;
        boost::condition         m_honk;
        boost::mutex             m_mutex;
    };
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

    bool trim = cfg.get_value("find-trim-ws") == "on";

    for(unsigned i = 0; i != opt.m_args.size(); ++i)
    {
        database_ptr db = open_database(cdb_path / "db");

        std::string pattern = opt.m_args[i];
        if(opt.m_options.count("-v"))
            pattern = escape_regex(pattern);

        mt_search mts(*db, trim, prefix_size);

        auto worker = [&](const char* w)
        {
            mts.search_db(compile_regex(pattern, 0, find_regex_options),
                          compile_regex(file_match, 0, file_regex_options),
                          w);
        };

        boost::thread_group workers;
        if(unsigned hw_threads = boost::thread::hardware_concurrency())
            for(unsigned i = 0; i != hw_threads-1; ++i)
                workers.create_thread(std::bind(worker, "w1"));

        worker("w0");

        workers.join_all();
    }
}
