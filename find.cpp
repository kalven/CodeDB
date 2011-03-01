// CodeDB - public domain - 2010 Daniel Andersson

#include "find.hpp"
#include "options.hpp"
#include "regex.hpp"
#include "config.hpp"
#include "file_lock.hpp"
#include "database.hpp"
#include "search.hpp"

#include <boost/xpressive/xpressive_dynamic.hpp>

#include <iostream>

namespace
{
    std::string escape_regex(std::string text)
    {
        static const bxp::sregex escape_re =
            bxp::sregex::compile("([\\^\\.\\$\\|\\(\\)\\[\\]\\*\\+\\?\\/\\\\])");
        return bxp::regex_replace(text, escape_re, std::string("\\$1"));
    }

    class default_receiver : public match_receiver
    {
      public:
        default_receiver(bool trim)
          : m_trim(trim)
        {
        }

      private:

        const char* on_match(const match_info& match)
        {
            const char* line_start = match.m_line_start;
            if(m_trim)
            {
                while(*line_start == ' ' || *line_start == '\t')
                    ++line_start;
            }

            std::cout << match.m_file << ':' << match.m_line << ':';
            std::cout.write(line_start, match.m_line_end - line_start);

            return match.m_line_end;
        }

        bool m_trim;
    };
}

void find(const bfs::path& cdb_path, const options& opt)
{
    config cfg = load_config(cdb_path / "config");

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

    default_receiver receiver(cfg.get_value("find-trim-ws") == "on");

    file_lock lock(cdb_path / "lock");
    lock.lock_sharable();

    database db(cdb_path / "blob", cdb_path / "index");

    for(unsigned i = 0; i != opt.m_args.size(); ++i)
    {
        std::string pattern = opt.m_args[i];
        if(opt.m_options.count("-v"))
            pattern = escape_regex(pattern);

        search_db(db,
                  compile_cregex(pattern, find_regex_options),
                  compile_cregex(file_match, file_regex_options),
                  prefix_size,
                  receiver);
    }
}
