#include <boost/filesystem.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/xpressive/xpressive_dynamic.hpp>

#include <string>
#include <fstream>
#include <iostream>

namespace bfs = boost::filesystem;
namespace bxp = boost::xpressive;

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

            if(!line.empty())
            {
                line  += char(10);
                bytes += line.size();

                m_packed.write(line.c_str(), line.size());
            }
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

void process_directory(builder& b, const bfs::path& path)
{
    static const bxp::sregex file_re(bxp::sregex::compile(".*?\\.(h|hpp|c|cpp)$",
                                                          bxp::regex_constants::ECMAScript|
                                                          bxp::regex_constants::icase|
                                                          bxp::regex_constants::optimize));

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
            if(f.filename() != ".svn")
                process_directory(b, f);
        }
        else
        {
            if(regex_match(f.filename(), file_re))
                b.process_file(f.file_string());
        }
    }
}

bfs::path make_absolute(const char* path_str)
{
    return bfs::system_complete(bfs::path(path_str));
}

int main(int argc, char** argv)
{
    try
    {
        builder b("code.dat", "code.idx");

        for(char** i = argv+1; i != (argv+argc); ++i)
            process_directory(b, make_absolute(*i));
    }
    catch(const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}
