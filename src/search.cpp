// CodeDB - public domain - 2010 Daniel Andersson

#include "search.hpp"
#include "database.hpp"

#include <stdexcept>
#include <cstring>

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

void search(const char*     begin,
            const char*     end,
            regex&          re,
            match_info&     minfo,
            match_receiver& receiver)
{
    if(begin == end)
        return;

    minfo.m_line = 1;

    const char* search_pos = begin;

    while(re.search(search_pos, end))
    {
        match_range hit = re.what(0);

        std::size_t offset = (search_pos - begin) +
            (hit.begin() - search_pos);

        if(begin + offset == end)
            break;

        minfo.m_line += count_lines(search_pos, begin + offset, minfo.m_line_start);
        minfo.m_line_end = std::strchr(minfo.m_line_start, 10) + 1;
        minfo.m_position = begin + offset;

        search_pos = receiver.on_match(minfo);
        minfo.m_line++;

        if(search_pos >= end)
            break;
    }
}

void search_db(const database& db,
               regex&          re,
               regex&          file_re,
               std::size_t     prefix_size,
               match_receiver& receiver)
{
    match_info minfo;

    for(std::size_t i = 0; i != db.size(); ++i)
    {
        const database::file& file = db[i];

        if(file_re.search(file.m_name))
        {
            minfo.m_full_file  = file.m_name.c_str();
            minfo.m_file       = minfo.m_full_file + prefix_size;
            minfo.m_file_start = file.m_start;
            minfo.m_file_end   = file.m_end;

            search(file.m_start, file.m_end, re, minfo, receiver);
        }
    }
}
