// CodeDB - public domain - 2010 Daniel Andersson

#include "database.hpp"
#include "regex.hpp"

#include <boost/lexical_cast.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>

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
}

database_ptr open_database(const bfs::path& blob, const bfs::path& index)
{
    return database_ptr(new basic_database(blob, index));
}
