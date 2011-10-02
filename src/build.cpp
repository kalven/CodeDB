// CodeDB - public domain - 2010 Daniel Andersson

#include "build.hpp"
#include "regex.hpp"
#include "config.hpp"
#include "options.hpp"
#include "file_lock.hpp"

#include <boost/filesystem/fstream.hpp>
#include <boost/algorithm/string/trim.hpp>

#include <iostream>
#include <fstream>

namespace
{
    class builder
    {
      public:
        builder(const bfs::path& packed, const bfs::path& index, bool trim)
          : m_packed(packed, bfs::ofstream::binary)
          , m_index(index)
          , m_trim(trim)
        {
            if(!m_packed.is_open())
                throw std::runtime_error("Unable to open " + packed.string() + " for writing");
            if(!m_index.is_open())
                throw std::runtime_error("Unable to open " + index.string() + " for writing");
        }

        // Index format: byte_size:path

        void process_file(const bfs::path& path, std::size_t prefix_size)
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

            m_index << bytes << ':' << path.generic_string().substr(prefix_size) << '\n';
        }

      private:

        void process_input_line(std::string& line)
        {
            if(m_trim)
                boost::algorithm::trim(line);
        }

        bfs::ofstream m_packed;
        bfs::ofstream m_index;
        bool          m_trim;
    };

    struct build_options
    {
        regex_ptr m_file_inc_re;
        regex_ptr m_dir_excl_re;
        bool      m_verbose;
    };

    void process_directory(builder& b, build_options& o, const bfs::path& root)
    {
        if(!bfs::exists(root))
            throw std::runtime_error(root.string() + " does not exist");

        std::size_t prefix_size = root.string().size() + 1;

        for(bfs::recursive_directory_iterator i(root), end; i != end; ++i)
        {
            const bfs::path& f = *i;
            const bool is_dir = bfs::is_directory(f);

            if(is_dir && o.m_dir_excl_re->match(f.filename().string()))
            {
                i.no_push();
                continue;
            }

            if(!is_dir && o.m_file_inc_re->match(f.filename().string()))
            {
                b.process_file(f, prefix_size);
                if(o.m_verbose)
                    std::cout << f << std::endl;
            }
        }
    }
}

void build(const bfs::path& cdb_path, const options& opt)
{
    config cfg = load_config(cdb_path / "config");

    build_options bo;

    bo.m_file_inc_re = compile_regex(cfg.get_value("file-include"));
    bo.m_dir_excl_re = compile_regex(cfg.get_value("dir-exclude"));

    bo.m_verbose = opt.m_options.count("-v") == 1;

    file_lock lock(cdb_path / "lock");
    lock.lock_exclusive();

    builder b(cdb_path / "blob", cdb_path / "index", cfg.get_value("build-trim-ws") == "on");
    process_directory(b, bo, cdb_path.parent_path());
}
