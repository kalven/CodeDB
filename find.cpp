// CodeDB - public domain - 2010 Daniel Andersson

#include "find.hpp"
#include "options.hpp"

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

        if(last_break != end)
            ++last_break;
        last_line = last_break;

        return count;
    }

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

        void search(const bxp::cregex& re, const bxp::cregex& file_re)
        {
            bxp::cmatch what, file_what;

            if(m_data == m_data_end)
                return;

            char  linebuf[20];
            iovec buffers[3];

            buffers[1].iov_base = linebuf;

            const char* file_start = m_data;
            for(index_t::const_iterator i = m_index.begin(); i != m_index.end(); ++i)
            {
                const std::string& file = i->second;
                const char* file_end = m_data + i->first;
                const char* search_start = file_start;
                std::size_t line = 0;

                buffers[0].iov_base = const_cast<char*>(file.c_str());
                buffers[0].iov_len  = file.size();

                if(regex_search(file.c_str(), file.c_str() + file.size(), file_what, file_re))
                {
                    while(regex_search(search_start, file_end, what, re))
                    {
                        std::size_t offset = (search_start - m_data) +
                            static_cast<std::size_t>(what.position());

                        const char* line_start;
                        line += count_lines(search_start, m_data + offset, line_start);
                        const char* line_end = std::strchr(line_start, 10) + 1;

                        // buffers[1].iov_len = sprintf(linebuf, ":%d:", line+1);
                        // buffers[1].iov_len = 5;
                        // buffers[2].iov_base = const_cast<char*>(line_start);
                        // buffers[2].iov_len = line_end - line_start;

                        // writev(1, buffers, 3);

                        std::cout << file << ':' << (line+1) << ':';
                        std::cout.write(line_start, line_end - line_start);

                        search_start = line_end;
                        line++;

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

void find(const boost::filesystem::path& cdb_path, const options& opt)
{
    finder f(cdb_path / "blob", cdb_path / "index");

    bxp::regex_constants::syntax_option_type regex_options =
        bxp::regex_constants::ECMAScript|
        bxp::regex_constants::not_dot_newline|
        bxp::regex_constants::optimize;
    if(opt.m_options.count("-i"))
        regex_options = regex_options|bxp::regex_constants::icase;

    std::string file_match(".*");
    // if(vm.count("file-prefix"))
    //     file_match = "^" + escape_regex(vm["file-prefix"].as<std::string>()) + ".*";
    // else if(vm.count("file-regex"))
    //     file_match = vm["file-regex"].as<std::string>();

    for(unsigned i = 0; i != opt.m_args.size(); ++i)
        f.search(bxp::cregex::compile(opt.m_args[i].c_str(), regex_options),
                 bxp::cregex::compile(file_match, regex_options));
}
