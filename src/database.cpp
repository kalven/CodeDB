// CodeDB - public domain - 2010 Daniel Andersson

#include "database.hpp"
#include "profiler.hpp"
#include "config.hpp"

#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>

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
          : m_mapping(packed.string().c_str(), bip::read_only)
          , m_region(m_mapping, bip::read_only)
          , m_data(static_cast<const char*>(m_region.get_address()))
          , m_data_end(m_data + m_region.get_size())
          , m_load_profiler(make_profiler("load"))
        {
            if(m_data + 8 > m_data_end || m_data[0] != 'C' || m_data[1] != 'D' || m_data[2] != 'B' || m_data[3] != '1')
                throw std::runtime_error("Blob " + packed.string() + " is not valid");

            m_data += 4;
        }

      private:

        void rewind()
        {
            m_data = static_cast<const char*>(m_region.get_address()) + 4;
        }

        bool next_chunk(std::pair<const char*, const char*>& data)
        {
            profile_scope prof(m_load_profiler);

            if(m_data + 4 > m_data_end)
                return false;

            std::uint32_t chunk_size;
            std::memcpy(&chunk_size, m_data, 4);

            const char* chunk_end = m_data + 4 + chunk_size;

            if(chunk_end > m_data_end)
                return false;

            data.first = m_data + 4;
            data.second = chunk_end;

            m_data = chunk_end;

            return true;
        }

        bip::file_mapping  m_mapping;
        bip::mapped_region m_region;
        const char*        m_data;
        const char*        m_data_end;
        profiler&          m_load_profiler;
    };
}

database_ptr open_database(const bfs::path& blob)
{
    return database_ptr(new compressed_database(blob));
}
