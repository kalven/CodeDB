#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/xpressive/xpressive_dynamic.hpp>
#include <boost/program_options.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/next_prior.hpp>

#include <string>
#include <fstream>
#include <iostream>
#include <stdexcept>

#include <map>

namespace bxp = boost::xpressive;
namespace bip = boost::interprocess;
namespace bpo = boost::program_options;

bool isprefix(const std::string& big, const std::string& prefix)
{
    return big.size() >= prefix.size() && big.compare(0, prefix.size(), prefix) == 0;
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
           std::string index,
           std::string prefix)
      : m_mapping(packed.c_str(), bip::read_only)
      , m_region(m_mapping, bip::read_only)
      , m_data(static_cast<const char*>(m_region.get_address()))
      , m_slice_start(0)
      , m_slice_end(0)
    {
        load_index(index, prefix);
    }

    void search(const bxp::cregex& re)
    {
        bxp::cmatch what;

        if(m_slice_start == m_slice_end)
            return;

        const char* search_start = m_slice_start;
        while(regex_search(search_start, m_slice_end, what, re))
        {
            std::size_t offset = (search_start - m_data) +
                static_cast<std::size_t>(what.position());

            file_range range = find_file_range(m_data + offset);

            const char* line_start;
            std::size_t line = count_lines(range.m_start,
                                           m_data + offset,
                                           line_start);
            const char* line_end = std::strchr(line_start, 10);

            std::cout << range.m_path << ':' << (line+1) << ':';
            std::cout.write(line_start, line_end - line_start);
            std::cout << '\n';

            search_start = line_end + 1;

            if(search_start >= m_slice_end)
                break;
        }
    }

  private:

    void load_index(std::string path, std::string prefix)
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

                if(isprefix(file, prefix))
                {
                    if(m_slice_start == 0)
                        m_slice_start = m_data + offset - size;
                    m_slice_end = m_data + offset;

                    m_index.insert(std::make_pair(offset, file));
                }
            }
        }
    }

    struct file_range
    {
        std::string m_path;
        const char* m_start;
        const char* m_end;
    };

    file_range find_file_range(const char* location)
    {
        std::size_t offset = location - m_data;

        index_t::const_iterator i = m_index.lower_bound(offset);
        if(i == m_index.end())
            throw std::runtime_error("Location is out of range");

        index_t::const_iterator next = boost::next(i);
        if(next != m_index.end() && i->first == offset)
            i = next;

        std::size_t range_start =
            i == m_index.begin() ? 0 : boost::prior(i)->first;

        file_range result;
        result.m_path  = i->second;
        result.m_start = m_data + range_start;
        result.m_end   = m_data + i->first;

        return result;
    }

    typedef std::map<std::size_t, std::string> index_t;

    bip::file_mapping  m_mapping;
    bip::mapped_region m_region;
    index_t            m_index;
    const char*        m_data;
    const char*        m_slice_start;
    const char*        m_slice_end;
};

int main(int argc, char** argv)
{
    try
    {
        std::cout.sync_with_stdio(false);

        bpo::options_description desc("Options");
        desc.add_options()
            ("help,h", "show this help message")
            ("ignore-case,i", "ignore case distinctions")
            ("subdir,d", bpo::value<std::string>()->default_value(""), "limit search to a specific sub directory")
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

        bxp::regex_constants::syntax_option_type regex_options =
            bxp::regex_constants::ECMAScript|
            bxp::regex_constants::not_dot_newline|
            bxp::regex_constants::optimize;
        if(vm.count("ignore-case"))
            regex_options = regex_options|bxp::regex_constants::icase;

        std::string subdir = vm["subdir"].as<std::string>();
        std::string input  = vm["input"].as<std::string>();

        std::vector<std::string> patterns =
            bpo::collect_unrecognized(parsed.options, bpo::include_positional);

        finder f(input + ".dat", input + ".idx", subdir);
        for(unsigned i = 0; i != patterns.size(); ++i)
            f.search(bxp::cregex::compile(patterns[i].c_str(), regex_options));
    }
    catch(const std::exception& e)
    {
        std::cerr << "error: " << e.what() << std::endl;
    }
}
