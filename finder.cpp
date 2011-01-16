// CodeDB - public domain - 2010 Daniel Andersson

#include "finder.hpp"

#include <boost/xpressive/xpressive_dynamic.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/lexical_cast.hpp>

namespace bfs = boost::filesystem;
namespace bxp = boost::xpressive;

namespace
{
    std::size_t count_lines(const char* begin, const char* end, const char*& last_line)
    {
        std::size_t count = 0;
        const char* last_break = begin;

        while(begin != end)
        {
            if(*begin == char(10))
            {
                ++count;
                last_break = begin;
            }
            ++begin;
        }

        if(last_break != end && count != 0)
            ++last_break;
        last_line = last_break;

        return count;
    }
}

finder::finder(const bfs::path& packed, const bfs::path& index)
  : m_mapping(packed.string().c_str(), boost::interprocess::read_only)
  , m_region(m_mapping, boost::interprocess::read_only)
  , m_data(static_cast<const char*>(m_region.get_address()))
  , m_data_end(m_data + m_region.get_size())
{
    load_index(index);
}

void finder::search(std::size_t prefix_size, const bxp::cregex& re, const bxp::cregex& file_re, match_receiver& receiver)
{
    bxp::cmatch what, file_what;

    if(m_data == m_data_end)
        return;

    match_info minfo;

    const char* file_start = m_data;
    for(std::size_t i = 0; i != m_index.size(); ++i)
    {
        const std::string& file = m_index[i].second;
        const char* file_end = m_data + m_index[i].first;
        const char* search_start = file_start;

        minfo.m_full_file  = m_index[i].second.c_str();
        minfo.m_file       = minfo.m_full_file + prefix_size;
        minfo.m_file_start = file_start;
        minfo.m_file_end   = file_end;
        minfo.m_line       = 1;

        if(regex_search(file.c_str(), file.c_str() + file.size(), file_what, file_re))
        {
            while(regex_search(search_start, file_end, what, re))
            {
                std::size_t offset = (search_start - m_data) +
                    static_cast<std::size_t>(what.position());

                minfo.m_line += count_lines(search_start, m_data + offset, minfo.m_line_start);
                minfo.m_line_end = std::strchr(minfo.m_line_start, 10) + 1;
                minfo.m_position = m_data + offset;

                search_start = receiver.on_match(minfo);
                minfo.m_line++;

                if(search_start >= file_end)
                    break;
            }
        }

        file_start = file_end;
    }
}

void finder::load_index(const bfs::path& path)
{
    bfs::ifstream input(path);
    if(!input.is_open())
        throw std::runtime_error("Unable to open index " + path.string() + " for reading");

    bxp::sregex re = bxp::sregex::compile("(\\d+):(.*)");
    bxp::smatch what;

    std::size_t offset = 0;

    std::string line;
    while(getline(input, line))
    {
        if(line.empty() || line[0] == '#')
            continue;

        if(regex_match(line, what, re))
        {
            std::size_t size = boost::lexical_cast<std::size_t>(what[1]);
            std::string file = what[2];

            offset += size;
            m_index.push_back(std::make_pair(offset, file));
        }
    }
}
