// CodeDB - public domain - 2010 Daniel Andersson

#include "serve.hpp"
#include "serve_init.hpp"
#include "serve_util.hpp"
#include "regex.hpp"
#include "file_lock.hpp"
#include "database.hpp"
#include "search.hpp"
#include "httpd.hpp"
#include "nsalias.hpp"

#include <boost/xpressive/xpressive_dynamic.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/lexical_cast.hpp>

#include <iostream>
#include <sstream>
#include <map>

namespace
{
    const bxp::regex_constants::syntax_option_type default_regex_options =
        bxp::regex_constants::ECMAScript|
        bxp::regex_constants::not_dot_newline|
        bxp::regex_constants::optimize;

    class collecting_receiver : public match_receiver
    {
      public:
        struct file
        {
            std::vector<std::size_t> m_line_numbers;
            std::vector<std::string> m_lines;
        };

        std::map<std::string, file> m_hits;

      private:
        const char* on_match(const match_info& match)
        {
            file& f = m_hits[match.m_file];
            f.m_line_numbers.push_back(match.m_line);
            f.m_lines.push_back(std::string(match.m_line_start, match.m_line_end - 1));

            return match.m_line_end;
        }
    };

    class line_receiver : public match_receiver
    {
      public:
        std::vector<std::size_t> m_line_numbers;

      private:
        const char* on_match(const match_info& match)
        {
            m_line_numbers.push_back(match.m_line);
            return match.m_line_end;
        }
    };

    std::vector<std::string> handler(database& db, const bfs::path& doc_root, const http_request& req)
    {
        std::cout << "New request: \"" << req.m_resource << "\"\n";

        http_query q = parse_http_query(req.m_resource);

        if(q.m_action == "/view")
        {
            std::cout << "View" << std::endl;

            const database::file* f = db.find_by_name(q.m_args["f"]);
            if(!f)
                return serve_page("error...");

            match_info minfo;
            minfo.m_file_start = f->m_start;
            
            line_receiver lines;

            search(f->m_start, f->m_end,
                   compile_cregex(q.m_args["q"], default_regex_options),
                   minfo,
                   lines);

            std::ostringstream os;
            os <<
                "<!DOCTYPE html>\n"
                "<meta charset=utf-8>\n"
                "<link href=\"style.css\" rel=\"stylesheet\" type=\"text/css\"/>\n"
                "<div class=\"file\">\n"
                "<table class=\"code\" width=\"100%\">\n"
                "<caption>" << html_escape(q.m_args["f"]) << "</caption>"
                "<tr><td class=\"lines\"><pre>";

            std::size_t linecount = std::count(f->m_start, f->m_end, char(10));
            for(std::size_t i = 1; i <= linecount; ++i)
                os << i << '\n';

            os << "</pre></td><td class=\"text\"><pre>";

            std::size_t line = 1, linei = 0;
            for(const char* cursor = f->m_start; cursor != f->m_end; ++line)
            {
                const char* eol = std::find(cursor, f->m_end, char(10));
                if(linei < lines.m_line_numbers.size() && line == lines.m_line_numbers[linei])
                {
                    os << "<span class=hl>"
                       << html_escape(std::string(cursor, eol))
                       << ' '
                       << "</span>\n";

                    ++linei;
                }
                else
                {
                    os << html_escape(std::string(cursor, eol+1));
                }

                cursor = eol + 1;
            }

            os << "</pre></td></tr></table></div><br>\n";

            return serve_page(os.str());
        }

        if(q.m_action == "/search")
        {
            std::cout << "Search" << std::endl;
            try
            {
                collecting_receiver recevier;

                search_db(db,
                          compile_cregex(q.m_args["q"], default_regex_options),
                          compile_cregex("", default_regex_options),
                          0,
                          recevier);

                std::string page;
                page +=
                    "<!DOCTYPE html>\n"
                    "<meta charset=utf-8>\n"
                    "<link href=\"style.css\" rel=\"stylesheet\" type=\"text/css\"/>\n";

                for(auto i = recevier.m_hits.begin(); i != recevier.m_hits.end(); ++i)
                {
                    page += "<div class=\"file\">\n";
                    page += "<table class=\"code\" width=\"100%\">\n";
                    page += "<caption><a href=\"/view?f=" + urlencode(i->first) + "&q=" + urlencode(q.m_args["q"]) + "\">" + i->first + "</a></caption>";

                    page += "<tr><td class=\"lines\"><pre>";

                    auto& f = i->second;

                    for(auto j = f.m_line_numbers.begin(); j != f.m_line_numbers.end(); ++j)
                        page += boost::lexical_cast<std::string>(*j) + "\n";

                    page += "</pre></td><td class=\"text\"><pre>";

                    for(auto j = f.m_lines.begin(); j != f.m_lines.end(); ++j)
                        page += html_escape(*j) + "\n";

                    page += "</pre></td></tr></table></div><br>\n";
                }

                return serve_page(page);
            }
            catch(const std::exception& e)
            {
                return serve_page("error...");
            }
        }

        if(q.m_action == "/")
            q.m_action = "index.html";

        return serve_file(doc_root / q.m_action);
    }
}

void serve(const bfs::path& cdb_path, const options& opt)
{
    serve_init(cdb_path / "www");

    database db(cdb_path / "blob", cdb_path / "index");

    boost::asio::io_service iosvc;

    httpd server(iosvc, "0.0.0.0", "8080",
                 std::bind(&handler, std::ref(db), cdb_path / "www", std::placeholders::_1));

    std::cout << "CodeDB serving" << std::endl;
    iosvc.run();
}
