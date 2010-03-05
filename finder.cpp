// CodeDB - public domain - 2010 Daniel Andersson

#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/xpressive/xpressive_dynamic.hpp>
#include <boost/program_options.hpp>
#include <boost/lexical_cast.hpp>

#include <string>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <utility>
#include <vector>

namespace bxp = boost::xpressive;
namespace bip = boost::interprocess;
namespace bpo = boost::program_options;

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
    finder(std::string packed,
           std::string index)
      : m_mapping(packed.c_str(), bip::read_only)
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

        const char* file_start = m_data;
        for(index_t::const_iterator i = m_index.begin(); i != m_index.end(); ++i)
        {
            const std::string& file = i->second;
            const char* file_end = m_data + i->first;
            const char* search_start = file_start;
            std::size_t line = 0;

            if(regex_search(file.c_str(), file.c_str() + file.size(), file_what, file_re))
            {
                while(regex_search(search_start, file_end, what, re))
                {
                    std::size_t offset = (search_start - m_data) +
                        static_cast<std::size_t>(what.position());

                    const char* line_start;
                    line += count_lines(search_start, m_data + offset, line_start);
                    const char* line_end = std::strchr(line_start, 10);

                    std::cout << file << ':' << (line+1) << ':';
                    std::cout.write(line_start, line_end - line_start);
                    std::cout << '\n';

                    search_start = line_end + 1;
                    line++;

                    if(search_start >= file_end)
                        break;
                }
            }

            file_start = file_end;
        }
    }

  private:

    void load_index(std::string path)
    {
        std::ifstream input(path.c_str());
        if(!input.is_open())
            throw std::runtime_error("Unable to open index " + path + " for reading");

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

int main(int argc, char** argv)
{
    try
    {
        bpo::options_description desc("Options");
        desc.add_options()
            ("help,h", "show this help message")
            ("ignore-case,i", "ignore case distinctions")
            ("file-prefix,f", bpo::value<std::string>(), "limit search to files that match a prefix")
            ("file-regex,F", bpo::value<std::string>(), "limit search to files that match a regex")
            ("input,b",  bpo::value<std::string>(), "input base name");

        bpo::variables_map vm;
        bpo::parsed_options parsed =
            bpo::command_line_parser(argc, argv).options(desc).allow_unregistered().run();
        bpo::store(parsed, vm);
        bpo::notify(vm);

        if(vm.count("help") || !vm.count("input"))
        {
            std::cerr << "Usage: finder -b BASE [OPTION]... PATTERN...\n"
                      << "Search for each PATTERN in the db\n"
                      << "Loads db from BASE.dat and BASE.idx\n\n"
                      << desc << std::endl;
            return 0;
        }

        if(vm.count("file-prefix") && vm.count("file-regex"))
            throw std::runtime_error("file-prefix and file-regex are mutually exclusive");

        bxp::regex_constants::syntax_option_type regex_options =
            bxp::regex_constants::ECMAScript|
            bxp::regex_constants::not_dot_newline|
            bxp::regex_constants::optimize;
        if(vm.count("ignore-case"))
            regex_options = regex_options|bxp::regex_constants::icase;

        std::string file_match;
        if(vm.count("file-prefix"))
            file_match = "^" + escape_regex(vm["file-prefix"].as<std::string>()) + ".*";
        else if(vm.count("file-regex"))
            file_match = vm["file-regex"].as<std::string>();

        std::string input  = vm["input"].as<std::string>();

        std::vector<std::string> patterns =
            bpo::collect_unrecognized(parsed.options, bpo::include_positional);

        finder f(input + ".dat", input + ".idx");
        for(unsigned i = 0; i != patterns.size(); ++i)
            f.search(bxp::cregex::compile(patterns[i].c_str(), regex_options),
                     bxp::cregex::compile(file_match, regex_options));
    }
    catch(const std::exception& e)
    {
        std::cerr << "error: " << e.what() << std::endl;
    }
}
