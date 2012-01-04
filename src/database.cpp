// CodeDB - public domain - 2010 Daniel Andersson

#include "database.hpp"
#include "serialization.hpp"
#include "profiler.hpp"
#include "config.hpp"

#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>

database::~database()
{
}

db_chunk::db_chunk(const std::string& data)
  : m_data(data)
{
    // A chunk starts with an offset to the file data and the file count.
    m_data_offset = read_binary(m_data, 0);
    m_count       = read_binary(m_data, sizeof(db_uint));
    m_current     = 0;
}

bool db_chunk::next_file(db_file& file)
{
    static const auto header_size = sizeof(db_uint) * 2;
    static const auto entry_size = sizeof(db_uint) * 2;

    if(m_current == m_count)
        return false;

    // Each file entry is represented by two db_uints that tell us the size of
    // the file and an offset where the null terminated filename is stored.
    auto entry_offset = header_size + m_current * entry_size;

    db_uint file_size   = read_binary(m_data, entry_offset);
    db_uint name_offset = read_binary(m_data, entry_offset + sizeof(db_uint));

    // Filenames come after all the file variables.
    auto names_base = header_size + m_count * entry_size;

    file.m_name_start = m_data.c_str() + names_base + name_offset;
    file.m_name_end   = file.m_name_start + std::strlen(file.m_name_start);
    file.m_start      = m_data.c_str() + m_data_offset;
    file.m_end        = file.m_start + file_size;

    // Advance the data offset so that it points to the next file.
    m_data_offset += file_size;
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
            if(m_data + 4 > m_data_end || m_data[0] != 'C' || m_data[1] != 'D' || m_data[2] != 'B' || m_data[3] != '1')
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

            if(m_data + sizeof(db_uint) > m_data_end)
                return false;

            db_uint chunk_size;
            std::memcpy(&chunk_size, m_data, sizeof(db_uint));

            const char* chunk_end = m_data + sizeof(db_uint) + chunk_size;

            if(chunk_end > m_data_end)
                return false;

            data.first = m_data + sizeof(db_uint);
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
