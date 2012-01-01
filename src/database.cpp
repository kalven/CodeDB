// CodeDB - public domain - 2010 Daniel Andersson

#include "database.hpp"
#include "profiler.hpp"
#include "config.hpp"

#include <boost/filesystem/fstream.hpp>

namespace
{
    template<class T>
    T read_binary(std::istream& source)
    {
        T result;
        source.read(reinterpret_cast<char*>(&result), sizeof(result));
        return result;
    }

    template<class T>
    T read_binary(const std::string& src, std::size_t index)
    {
        T result;
        std::memcpy(&result, &src[index], sizeof(result));
        return result;
    }
}

database::~database()
{
}

db_chunk::db_chunk(const std::string& data)
  : m_data(data)
{
    m_data_offset = read_binary<std::uint32_t>(m_data, 0);
    m_count       = read_binary<std::uint32_t>(m_data, 4);
    m_current     = 0;
}

bool db_chunk::next_file(db_file& file)
{
    if(m_current == m_count)
        return false;

    std::size_t fvars_offset = (m_current + 1) * 8;
    const char* fname_base = m_data.c_str() + ((2 + (m_count*2)) * sizeof(std::uint32_t));

    std::uint32_t fsize = read_binary<std::uint32_t>(m_data, fvars_offset);
    std::uint32_t fname_offset = read_binary<std::uint32_t>(m_data, fvars_offset + sizeof(std::uint32_t));

    file.m_name_start = fname_base + fname_offset;
    file.m_name_end = file.m_name_start + std::strlen(file.m_name_start);
    file.m_start = m_data.c_str() + m_data_offset;
    file.m_end = file.m_start + fsize;

    m_data_offset += fsize;
    m_current++;

    return true;
}

namespace
{
    class compressed_database : public database
    {
      public:
        compressed_database(const bfs::path& packed)
          : m_packed(packed, bfs::ifstream::binary)
          , m_load_profiler(make_profiler("load"))
        {
            if(!m_packed.is_open())
                throw std::runtime_error("Unable to open blob " + packed.string() + " for reading");

            char hd[4];
            m_packed.read(hd, sizeof(hd));
            if(hd[0] != 'C' || hd[1] != 'D' || hd[2] != 'B' || hd[3] != '1')
                throw std::runtime_error("Blob " + packed.string() + " is not valid");
        }

      private:

        void rewind()
        {
            m_packed.clear();
            m_packed.seekg(4);
        }

        bool next_chunk(std::string& data)
        {
            profile_scope prof(m_load_profiler);

            std::uint32_t chunk_size = read_binary<std::uint32_t>(m_packed);
            if(!m_packed)
                return false;

            data.resize(chunk_size);
            m_packed.read(&data[0], data.size());

            return m_packed;
        }

        bfs::ifstream m_packed;
        profiler&     m_load_profiler;
    };
}

database_ptr open_database(const bfs::path& blob)
{
    return database_ptr(new compressed_database(blob));
}
