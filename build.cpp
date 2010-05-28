// CodeDB - public domain - 2010 Daniel Andersson

#include "build.hpp"
#include "regex.hpp"
#include "config.hpp"
#include "options.hpp"

#include <boost/filesystem/fstream.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/xpressive/xpressive_dynamic.hpp>

#include <iostream>
#include <fstream>

namespace bfs = boost::filesystem;
namespace bxp = boost::xpressive;

namespace
{
    class builder
    {
      public:
        builder(const bfs::path& packed, const bfs::path& index)
          : m_packed(packed, std::ofstream::binary)
          , m_index(index)
        {
            if(!m_packed.is_open())
                throw std::runtime_error("Unable to open " + packed.string() + " for writing");
            if(!m_index.is_open())
                throw std::runtime_error("Unable to open " + index.string() + " for writing");
        }

        // Index format: byte_size:path

        void process_file(const bfs::path& path)
        {
            bfs::ifstream input(path);
            if(!input.is_open())
                throw std::runtime_error("Unable to open " + path.string() + " for reading");

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

        bfs::ofstream m_packed;
        bfs::ofstream m_index;
    };

    struct build_options
    {
        bxp::sregex m_file_inc_re;
        bxp::sregex m_dir_excl_re;
        bool        m_verbose;
    };

    void process_directory(builder& b, build_options& o, const bfs::path& path)
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
                {
                    b.process_file(f.file_string());
                    if(o.m_verbose)
                        std::cout << f << std::endl;
                }
            }
        }
    }
}

void build(const bfs::path& cdb_path, const options& opt)
{
    config cfg = load_config(cdb_path / "config");

    bxp::regex_constants::syntax_option_type regex_options =
        bxp::regex_constants::ECMAScript|bxp::regex_constants::optimize;

    build_options bo;

    bo.m_file_inc_re = compile_sregex(
        cfg.get_value("file-include"), regex_options);
    bo.m_dir_excl_re = compile_sregex(
        cfg.get_value("dir-exclude"), regex_options);

    bo.m_verbose = opt.m_options.count("-v") == 1;

    builder b(cdb_path / "blob", cdb_path / "index");
    process_directory(b, bo, cdb_path.parent_path());
}
