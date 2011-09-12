// CodeDB - public domain - 2010 Daniel Andersson

#include "database.hpp"

#include <boost/xpressive/xpressive_dynamic.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/lexical_cast.hpp>

database::database(const bfs::path& packed, const bfs::path& index)
  : m_mapping(packed.string().c_str(), bip::read_only)
  , m_region(m_mapping, bip::read_only)
{
    load_index(index);
}

void database::load_index(const bfs::path& path)
{
    bfs::ifstream input(path);
    if(!input.is_open())
        throw std::runtime_error("Unable to open index " + path.string() + " for reading");

    bxp::sregex re = bxp::sregex::compile("(\\d+):(.*)");
    bxp::smatch what;

    const char* data = static_cast<const char*>(m_region.get_address());

    std::string line;
    while(getline(input, line))
    {
        if(line.empty() || line[0] == '#')
            continue;

        if(regex_match(line, what, re))
        {
            std::size_t size = boost::lexical_cast<std::size_t>(what[1]);

            file entry = {what[2], data, data+size};
            m_index.push_back(entry);

            data += size;
        }
    }

    if(data > static_cast<const char*>(m_region.get_address()) + m_region.get_size())
        throw std::runtime_error("Index mismatch");
}
