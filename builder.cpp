#include <boost/filesystem.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/xpressive/xpressive_dynamic.hpp>
#include <boost/program_options.hpp>

#include <string>
#include <fstream>
#include <iostream>

namespace bfs = boost::filesystem;
namespace bxp = boost::xpressive;
namespace bpo = boost::program_options;

class builder
{
  public:
    builder(std::string packed,
            std::string index)
      : m_packed(packed.c_str(), std::ofstream::binary)
      , m_index(index.c_str())
    {
        if(!m_packed.is_open())
            throw std::runtime_error("Unable to open " + packed + " for writing");
        if(!m_index.is_open())
            throw std::runtime_error("Unable to open " + index + " for writing");
    }

    // Index format: byte_size:path

    void process_file(std::string path)
    {
        std::ifstream input(path.c_str());
        if(!input.is_open())
            throw std::runtime_error("Unable to open " + path + " for reading");

        std::string line;
        std::size_t bytes = 0;
        while(getline(input, line))
        {
            process_input_line(line);

            line  += char(10);
            bytes += line.size();

            m_packed.write(line.c_str(), line.size());
        }

        m_index << bytes << ':' << path << '\n';
    }

  private:

    void process_input_line(std::string& line)
    {
        boost::algorithm::trim(line);
    }

    std::ofstream m_packed;
    std::ofstream m_index;
};

struct options
{
    bxp::sregex m_file_inc_re;
    bxp::sregex m_dir_excl_re;
};

void process_directory(builder& b, options& o, const bfs::path& path)
{
    if(!bfs::exists(path))
    {
        std::cerr << path << " does not exist" << std::endl;
        return;
    }

    for(bfs::directory_iterator i(path), end; i != end; ++i)
    {
        const bfs::path& f = *i;
        if(bfs::is_directory(f))
        {
            if(!regex_match(f.filename(), o.m_dir_excl_re))
                process_directory(b, o, f);
        }
        else
        {
            if(regex_match(f.filename(), o.m_file_inc_re))
                b.process_file(f.file_string());
        }
    }
}

bfs::path make_absolute(const std::string& path_str)
{
    return bfs::system_complete(bfs::path(path_str));
}

int main(int argc, char** argv)
{
    try
    {
        bpo::options_description desc("Options");
        desc.add_options()
            ("help,h", "show this help message")
            ("append,a", "append to an existing db (default: overwrite)")
            ("dir-exclude,d",  bpo::value<std::string>()->default_value(""), "directories to exclude (default: all)")
            ("file-include,i", bpo::value<std::string>()->default_value(".*"), "files to include (default: all)")
            ("output,o",       bpo::value<std::string>(), "output filename");

        bpo::variables_map vm;
        bpo::parsed_options parsed =
            bpo::command_line_parser(argc, argv).options(desc).allow_unregistered().run();
        bpo::store(parsed, vm);
        bpo::notify(vm);

        if(vm.count("help") || !vm.count("output"))
        {
            std::cerr << "Usage: builder -f BASE [OPTION]... DIRECTORY...\n"
                      << "Build db recursively from files in each DIRECTORY.\n"
                      << "Creates files BASE.dat and BASE.idx.\n\n"
                      << "Example: builder -ocode -d '\\.svn' -i /usr/include\n\n"
                      << desc << std::endl;
            return 0;
        }

        std::string output       = vm["output"].as<std::string>();
        std::string dir_exclude  = vm["dir-exclude"].as<std::string>();
        std::string file_include = vm["file-include"].as<std::string>();

        std::vector<std::string> dirs =
            bpo::collect_unrecognized(parsed.options, bpo::include_positional);

        bxp::regex_constants::syntax_option_type regex_options =
            bxp::regex_constants::ECMAScript|bxp::regex_constants::optimize;
        
        options opt;

        opt.m_file_inc_re = bxp::sregex::compile(file_include, regex_options);
        opt.m_dir_excl_re = bxp::sregex::compile(dir_exclude, regex_options);

        builder b(output + ".dat", output + ".idx");
        for(unsigned i = 0; i != dirs.size(); ++i)
            process_directory(b, opt, make_absolute(dirs[i]));
    }
    catch(const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}
