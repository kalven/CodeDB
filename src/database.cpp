// CodeDB - public domain - 2010 Daniel Andersson

#include "database.hpp"
#include "profiler.hpp"
#include "compress.hpp"
#include "config.hpp"
#include "mvar.hpp"

#include <boost/lexical_cast.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>

#include <boost/thread.hpp>

#include <iostream>

database::~database()
{
}

namespace
{
    class compressed_database : public database
    {
      public:
        compressed_database(const bfs::path& packed, const bfs::path& index)
          : m_packed(packed, bfs::ifstream::binary)
          , m_chunk_offset(0)
          , m_file_index(0)
          , m_load_profiler(make_profiler("load"))
          , m_decomp_profiler(make_profiler("decompress"))
        {
            if(!m_packed.is_open())
                throw std::runtime_error("Unable to open blob " + packed.string() + " for reading");

            char hd[4];
            m_packed.read(hd, sizeof(hd));
            if(hd[0] != 'C' || hd[1] != 'D' || hd[2] != 'B' || hd[3] != 'Z')
                throw std::runtime_error("Blob " + packed.string() + " is not valid");

            load_index(index);
        }

        ~compressed_database()
        {
            stop();
        }

      private:

        struct ex_file : public database::file
        {
            std::size_t m_size;
        };

        void stop()
        {
            if(m_thread)
            {
                m_thread->interrupt();
                m_thread->join();
            }
        }

        void restart()
        {
            stop();

            m_packed.clear();
            m_packed.seekg(4);

            m_chunkvar.reset(new mvar<std::string>);
            m_thread.reset(new boost::thread([this] { threadfun(); }));

            m_chunk.clear();
            m_chunk_offset = 0;
            m_file_index = 0;
        }

        const file* next_file()
        {
            if(m_file_index == m_index.size())
                return 0;

            if(m_chunk_offset == m_chunk.size())
            {
                profile_scope prof(m_load_profiler);
                load_chunk();
            }

            ex_file* f = &m_index[m_file_index++];
            f->m_start = m_chunk.data() + m_chunk_offset;
            f->m_end   = f->m_start + f->m_size;

            m_chunk_offset += f->m_size;

            return f;
        }

        void load_index(const bfs::path& path)
        {
            profile_scope prof("load index");

            bfs::ifstream input(path);
            if(!input.is_open())
                throw std::runtime_error("Unable to open index " + path.string() + " for reading");

            std::size_t offset = 0;

            std::string line;
            while(getline(input, line))
            {
                if(line.empty() || line[0] == '#')
                    continue;

                unsigned int size;
                if(std::sscanf(line.c_str(), "%d:", &size) == 1)
                {
                    ex_file entry;

                    entry.m_name  = std::strchr(line.c_str(), ':') + 1;
                    entry.m_start = 0;
                    entry.m_end   = 0;
                    entry.m_size  = static_cast<std::size_t>(size);

                    m_index.push_back(entry);

                    offset += size;
                }
            }
        }

        void threadfun()
        {
            try
            {
                std::string compressed;
                std::string decompressed;

                for(;;)
                {
                    profile_start(m_decomp_profiler);

                    std::uint32_t csize;
                    m_packed.read(reinterpret_cast<char*>(&csize), sizeof(csize));
                    if(m_packed.gcount() != sizeof(csize))
                        break;

                    compressed.resize(csize);
                    m_packed.read(&compressed[0], csize);
                    if(static_cast<std::uint32_t>(m_packed.gcount()) != csize)
                        throw std::runtime_error("Unable to load compressed chunk data");

                    snappy_uncompress(compressed, decompressed);

                    profile_stop(m_decomp_profiler);

                    m_chunkvar->put(decompressed);
                }
            }
            catch(const std::exception& e)
            {
                std::cerr << "reader error: " << e.what() << std::endl;
                std::exit(0);
            }
        }

        void load_chunk()
        {
            m_chunkvar->get(m_chunk);
            m_chunk_offset = 0;
        }

        bfs::ifstream                  m_packed;
        std::vector<ex_file>           m_index;
        std::string                    m_chunk;
        std::size_t                    m_chunk_offset;
        std::size_t                    m_file_index;
        std::unique_ptr<mvar<std::string>> m_chunkvar;
        std::unique_ptr<boost::thread> m_thread;
        profiler&                      m_load_profiler;
        profiler&                      m_decomp_profiler;
    };
}

database_ptr open_database(const bfs::path& blob, const bfs::path& index)
{
    return database_ptr(new compressed_database(blob, index));
}
