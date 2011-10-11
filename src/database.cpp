// CodeDB - public domain - 2010 Daniel Andersson

#include "database.hpp"
#include "compress.hpp"
#include "config.hpp"
#include "regex.hpp"

#include <boost/lexical_cast.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>

#include <iostream>

database::~database()
{
}

namespace
{
    class basic_database : public database
    {
      public:
        basic_database(const bfs::path& packed, const bfs::path& index)
          : m_mapping(packed.string().c_str(), bip::read_only)
          , m_region(m_mapping, bip::read_only)
        {
            load_index(index);
            restart();
        }

      private:

        void restart()
        {
            m_current_file = 0;
        }

        const file* next_file()
        {
            if(m_current_file == m_index.size())
                return 0;

            return &m_index[m_current_file++];
        }

        void load_index(const bfs::path& path)
        {
            bfs::ifstream input(path);
            if(!input.is_open())
                throw std::runtime_error("Unable to open index " + path.string() + " for reading");

            auto re = compile_regex("(\\d+):(.*)", 2);

            const char* data = static_cast<const char*>(m_region.get_address());

            std::string line;
            while(getline(input, line))
            {
                if(line.empty() || line[0] == '#')
                    continue;

                if(re->match(line))
                {
                    std::size_t size = boost::lexical_cast<std::size_t>(
                        static_cast<std::string>(re->what(1)));

                    file entry = {re->what(2), data, data+size};
                    m_index.push_back(entry);

                    data += size;
                }
            }

            if(data > static_cast<const char*>(m_region.get_address()) + m_region.get_size())
                throw std::runtime_error("Index mismatch");
        }

        bip::file_mapping  m_mapping;
        bip::mapped_region m_region;
        std::vector<file>  m_index;
        std::size_t        m_current_file;
    };

    class compressed_database : public database
    {
      public:
        compressed_database(const bfs::path& packed, const bfs::path& index)
          : m_packed(packed)
        {
            if(!m_packed.is_open())
                throw std::runtime_error("Unable to open blob " + packed.string() + " for reading");

            load_index(index);
            restart();
        }

      private:

        struct ex_file : public database::file
        {
            std::size_t m_size;
        };

        void restart()
        {
            m_packed.seekg(0);

            m_chunk.clear();
            m_chunk_offset = 0;
            m_file_index = 0;
        }

        const file* next_file()
        {
            if(m_file_index == m_index.size())
                return 0;

            if(m_chunk_offset == m_chunk.size())
                load_chunk();

            ex_file* f = &m_index[m_file_index++];
            f->m_start = m_chunk.data() + m_chunk_offset;
            f->m_end   = f->m_start + f->m_size;

            m_chunk_offset += f->m_size;

            return f;
        }

        void load_index(const bfs::path& path)
        {
            bfs::ifstream input(path);
            if(!input.is_open())
                throw std::runtime_error("Unable to open index " + path.string() + " for reading");

            auto re = compile_regex("(\\d+):(.*)", 2);

            std::size_t offset = 0;

            std::string line;
            while(getline(input, line))
            {
                if(line.empty() || line[0] == '#')
                    continue;

                if(re->match(line))
                {
                    std::size_t size = boost::lexical_cast<std::size_t>(
                        static_cast<std::string>(re->what(1)));

                    ex_file entry;

                    entry.m_name  = re->what(2);
                    entry.m_start = 0;
                    entry.m_end   = 0;
                    entry.m_size  = size;

                    m_index.push_back(entry);

                    offset += size;
                }
            }
        }

        void load_chunk()
        {
            std::size_t csize;
            m_packed.read(reinterpret_cast<char*>(&csize), sizeof(csize));
            if(m_packed.gcount() != sizeof(csize))
                throw std::runtime_error("Unable to load compressed chunk header");

            std::string compressed;
            compressed.resize(csize);
            m_packed.read(&compressed[0], csize);
            if(static_cast<std::size_t>(m_packed.gcount()) != csize)
                throw std::runtime_error("Unable to load compressed chunk data");

            snappy_uncompress(compressed, m_chunk);
            m_chunk_offset = 0;
        }

        bfs::ifstream        m_packed;
        std::vector<ex_file> m_index;
        std::string          m_chunk;
        std::size_t          m_chunk_offset;
        std::size_t          m_file_index;
    };
}

database_ptr open_database(const bfs::path& blob, const bfs::path& index, const config& cfg)
{
    database_ptr res;

    if(cfg.get_value("compression") == "on")
        res.reset(new compressed_database(blob, index));
    else
        res.reset(new basic_database(blob, index));

    return res;
}
