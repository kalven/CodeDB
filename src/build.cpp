// CodeDB - public domain - 2010 Daniel Andersson

#include "build.hpp"
#include "regex.hpp"
#include "config.hpp"
#include "options.hpp"
#include "compress.hpp"
#include "file_lock.hpp"
#include "profiler.hpp"

#include <boost/filesystem/fstream.hpp>
#include <boost/algorithm/string/trim.hpp>

#include <iostream>
#include <fstream>

namespace
{
    const std::size_t max_chunk_size = 512*1024;

    class builder
    {
      public:
        builder(const bfs::path& packed, const bfs::path& index, bool trim)
          : m_packed(packed, bfs::ofstream::binary)
          , m_index(index)
          , m_trim(trim)
          , m_process_file_prof(make_profiler("process_file"))
          , m_compress_prof(make_profiler("compress"))
        {
            if(!m_packed.is_open())
                throw std::runtime_error("Unable to open " + packed.string() + " for writing");
            if(!m_index.is_open())
                throw std::runtime_error("Unable to open " + index.string() + " for writing");

            m_packed.write("CDBZ", 4);
        }

        // Index format: byte_size:path

        void process_file(const bfs::path& path, std::size_t prefix_size)
        {
            profile_scope prof(m_process_file_prof);

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

                m_chunk += line;
            }

            if(m_chunk.size() > max_chunk_size)
                write_chunk();

            m_index << bytes << ':' << path.generic_string().substr(prefix_size) << '\n';
        }

        void finalize()
        {
            write_chunk();
        }

      private:

        void process_input_line(std::string& line)
        {
            if(m_trim)
                boost::algorithm::trim(line);
        }

        void write_chunk()
        {
            profile_scope prof(m_compress_prof);

            std::string compressed;
            snappy_compress(m_chunk, compressed);
            m_chunk.clear();

            std::uint32_t csize =
                static_cast<std::uint32_t>(compressed.size());
            m_packed.write(reinterpret_cast<const char*>(&csize), sizeof(csize));
            m_packed.write(compressed.c_str(), compressed.size());
        }

        std::string   m_chunk;
        bfs::ofstream m_packed;
        bfs::ofstream m_index;
        bool          m_trim;
        profiler&     m_process_file_prof;
        profiler&     m_compress_prof;
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

    builder b(cdb_path / "blob",
              cdb_path / "index",
              cfg.get_value("build-trim-ws") == "on");

    process_directory(b, bo, cdb_path.parent_path());
    b.finalize();
}
