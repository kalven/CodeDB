// CodeDB - public domain - 2010 Daniel Andersson

#include "build.hpp"
#include "regex.hpp"
#include "config.hpp"
#include "options.hpp"
#include "compress.hpp"
#include "file_lock.hpp"
#include "profiler.hpp"

#include <boost/filesystem/fstream.hpp>

#include <type_traits>
#include <iostream>
#include <fstream>
#include <cassert>

namespace
{
    const std::size_t max_chunk_size = 512*1024;

    void trim(const char*& b, const char*& e)
    {
        while(b != e && (*b == ' ' || *b == '\t' || *b == '\r'))
            ++b;
        while(e != b && (e[-1] == ' ' || e[-1] == '\t' || e[-1] == '\r'))
            --e;
    }

    template<class T>
    void write_binary(std::ostream& dest, const T* array, std::size_t count)
    {
        static_assert(std::is_pointer<T>::value == false, "T can't be pointer");
        static_assert(std::is_pod<T>::value, "T must be POD");
        dest.write(reinterpret_cast<const char*>(array), sizeof(T) * count);
    }

    template<class T>
    void write_binary(std::ostream& dest, const T& value)
    {
        write_binary(dest, &value, 1);
    }

    template<class T>
    void append_binary(std::string& dest, const T& value)
    {
        static_assert(std::is_pointer<T>::value == false, "T can't be pointer");
        static_assert(std::is_pod<T>::value, "T must be POD");

        const char* ptr = reinterpret_cast<const char*>(&value);
        dest.append(ptr, ptr + sizeof(value));
    }

    template<class T>
    void write_binary_at(std::string& dest, std::size_t index, const T& value)
    {
        assert(index + sizeof(value) <= dest.size());
        std::memcpy(&dest[index], &value, sizeof(value));
    }

    class builder
    {
      public:
        builder(const bfs::path& packed, bool trim)
          : m_packed(packed, bfs::ofstream::binary)
          , m_trim(trim)
          , m_process_file_prof(make_profiler("process_file"))
          , m_compress_prof(make_profiler("compress"))
        {
            if(!m_packed.is_open())
                throw std::runtime_error("Unable to open " + packed.string() + " for writing");

            // Database magic
            m_packed.write("CDB1", 4);
        }

        ~builder()
        {
            if(!m_chunk_files.empty())
                compress_chunk();
        }

        void process_file(const bfs::path& path, std::size_t prefix_size)
        {
            profile_start(m_process_file_prof);

            bfs::ifstream input(path);
            if(!input.is_open())
                throw std::runtime_error("Unable to open " + path.string() + " for reading");

            std::string line;
            std::size_t bytes = 0;
            while(getline(input, line))
            {
                const char* b = line.c_str();
                const char* e = b + line.size();

                if(m_trim)
                    trim(b, e);

                m_chunk_data.append(b, e - b);
                m_chunk_data += '\n';

                bytes += (e - b) + 1;
            }

            file_entry fe;
            fe.m_size = static_cast<std::uint32_t>(bytes);
            fe.m_name = path.generic_string().substr(prefix_size);

            m_chunk_files.push_back(fe);

            profile_stop(m_process_file_prof);

            if(m_chunk_data.size() > max_chunk_size)
                compress_chunk();
        }

      private:

        void compress_chunk()
        {
            profile_scope prof(m_compress_prof);

            std::string chunk;

            // file content offset (placeholder)
            append_binary(chunk, std::uint32_t(0));

            // file count
            append_binary(chunk, static_cast<std::uint32_t>(m_chunk_files.size()));

            // [{file-size, name-offset}]
            std::uint32_t name_offset = 0;
            for(auto i = m_chunk_files.begin(); i != m_chunk_files.end(); ++i)
            {
                append_binary(chunk, i->m_size);
                append_binary(chunk, name_offset);

                name_offset += i->m_name.size() + 1;
            }

            // [{filename, 0}]
            for(auto i = m_chunk_files.begin(); i != m_chunk_files.end(); ++i)
            {
                chunk += i->m_name;
                chunk += '\0';
            }

            // now we can write the file content offset
            write_binary_at(chunk, 0, static_cast<std::uint32_t>(chunk.size()));

            // finally, the actual file contents
            chunk += m_chunk_data;

            std::string compressed_chunk;
            snappy_compress(chunk, compressed_chunk);

            write_binary(m_packed, static_cast<std::uint32_t>(compressed_chunk.size()));
            m_packed.write(compressed_chunk.c_str(), compressed_chunk.size());

            m_chunk_data.clear();
            m_chunk_files.clear();
        }

        struct file_entry
        {
            std::uint32_t m_size;
            std::string   m_name;
        };

        std::string             m_chunk_data;
        bfs::ofstream           m_packed;
        bool                    m_trim;
        profiler&               m_process_file_prof;
        profiler&               m_compress_prof;
        std::vector<file_entry> m_chunk_files;
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

    builder b(cdb_path / "db", cfg.get_value("build-trim-ws") == "on");

    process_directory(b, bo, cdb_path.parent_path());
}
