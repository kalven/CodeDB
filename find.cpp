// CodeDB - public domain - 2010 Daniel Andersson

#include "find.hpp"
#include "options.hpp"
#include "regex.hpp"
#include "config.hpp"

#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/xpressive/xpressive_dynamic.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/lexical_cast.hpp>

#include <iostream>

namespace bip = boost::interprocess;
namespace bfs = boost::filesystem;
namespace bxp = boost::xpressive;

namespace
{
    const bxp::regex_constants::syntax_option_type default_regex_options =
        bxp::regex_constants::ECMAScript|
        bxp::regex_constants::not_dot_newline|
        bxp::regex_constants::optimize;

    std::string escape_regex(std::string text)
    {
        static const bxp::sregex escape_re =
            bxp::sregex::compile("([\\^\\.\\$\\|\\(\\)\\[\\]\\*\\+\\?\\/\\\\])");
        return bxp::regex_replace(text, escape_re, std::string("\\$1"));
    }

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

    struct match_info
    {
        const char* m_file;
        const char* m_full_file;
        const char* m_file_start;
        const char* m_file_end;
        const char* m_line_start;
        const char* m_line_end;
        const char* m_position;
        std::size_t m_line;
    };

    class match_receiver
    {
      public:
        virtual ~match_receiver() {}
        virtual const char* on_match(const match_info&) = 0;
    };

    class default_receiver : public match_receiver
    {
        const char* on_match(const match_info& match)
        {
            std::cout << match.m_file << ':' << match.m_line << ':';
            std::cout.write(match.m_line_start, match.m_line_end - match.m_line_start);

            return match.m_line_end;
        }
    };

    class finder
    {
      public:
        finder(const bfs::path& packed, const bfs::path& index)
          : m_mapping(packed.string().c_str(), bip::read_only)
          , m_region(m_mapping, bip::read_only)
          , m_data(static_cast<const char*>(m_region.get_address()))
          , m_data_end(m_data + m_region.get_size())
        {
            load_index(index);
        }

        void search(std::size_t prefix_size, const bxp::cregex& re, const bxp::cregex& file_re, match_receiver& receiver)
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

      private:

        void load_index(const bfs::path& path)
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

        typedef std::vector<std::pair<std::size_t, std::string> > index_t;

        bip::file_mapping  m_mapping;
        bip::mapped_region m_region;
        index_t            m_index;
        const char*        m_data;
        const char*        m_data_end;
    };
}

void find(const bfs::path& cdb_path, const options& opt)
{
    config cfg = load_config(cdb_path / "config");
    finder f(cdb_path / "blob", cdb_path / "index");

    bxp::regex_constants::syntax_option_type find_regex_options = default_regex_options;
    bxp::regex_constants::syntax_option_type file_regex_options = default_regex_options;

    if(opt.m_options.count("-i"))
        find_regex_options = find_regex_options|bxp::regex_constants::icase;
    if(cfg.get_value("nocase-file-match") == "on")
        file_regex_options = file_regex_options|bxp::regex_constants::icase;

    const bfs::path cdb_root = cdb_path.parent_path();
    const bfs::path search_root = bfs::initial_path();

    std::size_t prefix_size = 0;
    std::string file_match;
    if(opt.m_options.count("-a") == 0 && search_root != cdb_root)
    {
        file_match = "^" + escape_regex(
            search_root.string().substr(cdb_root.string().size() + 1) +
            bfs::slash<bfs::path>::value) + ".*";
        prefix_size = search_root.string().size() - cdb_root.string().size();
    }

    default_receiver receiver;

    for(unsigned i = 0; i != opt.m_args.size(); ++i)
    {
        std::string pattern = opt.m_args[i];
        if(opt.m_options.count("-v"))
            pattern = escape_regex(pattern);

        f.search(prefix_size,
                 compile_cregex(pattern, find_regex_options),
                 compile_cregex(file_match, file_regex_options),
                 receiver);
    }
}

