// CodeDB - public domain - 2010 Daniel Andersson

#include "serve.hpp"
#include "serve_init.hpp"
#include "serve_util.hpp"
#include "compress.hpp"
#include "config.hpp"
#include "regex.hpp"
#include "file_lock.hpp"
#include "database.hpp"
#include "search.hpp"
#include "httpd.hpp"

#include <boost/algorithm/string/case_conv.hpp>

#include <iostream>
#include <sstream>
#include <map>

namespace {
std::string header(const std::string& search_string) {
  return "<!DOCTYPE html>"
         "<meta charset=utf-8>"
         "<link href=\"style.css\" rel=\"stylesheet\" type=\"text/css\"/>"
         "<script type=\"text/javascript\" src=\"logo.js\"></script>"
         "<title>CodeDB</title>"
         "<a href=\"/\"><canvas id=\"logo\" width=\"200\" "
         "height=\"60\">CodeDB</canvas></a>"
         "<form id=\"search\" action=search method=get>"
         "<input id=\"q\" name=q type=search placeholder=\"Type a regex to "
         "search\" autofocus value=\"" +
         quot_escape(search_string) +
         "\"><input id=\"s\" type=submit value=Search>"
         "</form>"
         "<div id=\"body\">";
}

std::string footer() { return "</div>"; }

bool find_by_name(database& db, db_file& result, const std::string& file_name,
                  std::string& storage) {
  std::pair<const char*, const char*> compressed;

  db.rewind();
  while (db.next_chunk(compressed)) {
    snappy_uncompress(compressed.first, compressed.second, storage);
    db_chunk chunk(storage);

    db_file file;
    while (chunk.next_file(file)) {
      if (file_name == file.m_name_start) {
        result = file;
        return true;
      }
    }
  }

  return false;
}

class collecting_receiver : public match_receiver {
 public:
  struct file {
    std::vector<std::size_t> m_line_numbers;
    std::vector<std::string> m_lines;
  };

  std::map<std::string, file> m_hits;

 private:
  const char* on_match(const match_info& match) {
    file& f = m_hits[match.m_file];
    f.m_line_numbers.push_back(match.m_line);
    f.m_lines.push_back(std::string(match.m_line_start, match.m_line_end - 1));

    return match.m_line_end;
  }
};

class line_receiver : public match_receiver {
 public:
  std::vector<std::size_t> m_line_numbers;

 private:
  const char* on_match(const match_info& match) {
    m_line_numbers.push_back(match.m_line);
    return match.m_line_end;
  }
};

std::vector<std::string> handle_view(database& db, const http_query& q) {
  std::string file_name = get_arg(q, "f");

  db_file f;
  std::string file_storage;
  if (!find_by_name(db, f, file_name, file_storage))
    throw std::runtime_error("File <b>" + html_escape(file_name) +
                             "</b> not found");

  std::string search_string = get_arg(q, "q");

  match_info minfo;
  minfo.m_file_start = f.m_start;

  line_receiver lines;

  regex_ptr re = compile_regex(search_string);
  search(f.m_start, f.m_end, *re, minfo, lines);

  std::ostringstream os;
  os << header(search_string) << "<div class=\"file\">\n"
                                 "<table class=\"code\" width=\"100%\">\n"
                                 "<caption>" << html_escape(file_name)
     << "</caption>"
        "<tr><td class=\"lines\"><pre>";

  std::size_t linecount = std::count(f.m_start, f.m_end, char(10));
  for (std::size_t i = 1; i <= linecount; ++i) os << i << '\n';

  os << "</pre></td><td class=\"text\"><pre>";

  std::size_t line = 1, linei = 0;
  for (const char* cursor = f.m_start; cursor != f.m_end; ++line) {
    const char* eol = std::find(cursor, f.m_end, char(10));
    if (linei < lines.m_line_numbers.size() &&
        line == lines.m_line_numbers[linei]) {
      os << "<span class=hl>" << html_escape(std::string(cursor, eol)) << ' '
         << "</span>\n";

      ++linei;
    } else {
      os << html_escape(std::string(cursor, eol + 1));
    }

    cursor = eol + 1;
  }

  os << "</pre></td></tr></table></div><br>" << footer();

  return serve_page(os.str());
}

std::vector<std::string> handle_search(database& db, const http_query& q) {
  collecting_receiver recevier;

  std::string search_string = get_arg(q, "q");

  regex_ptr re = compile_regex(search_string);
  regex_ptr file_re = compile_regex("");

  search_db(db, *re, *file_re, 0, recevier);

  std::ostringstream os;

  os << header(search_string);

  if (recevier.m_hits.empty()) {
    os << "Your search - <b>" << html_escape(search_string)
       << "</b> - did not match any documents.";
  }

  for (auto i = recevier.m_hits.begin(); i != recevier.m_hits.end(); ++i) {
    os << "<div class=\"file\"><table class=\"code\" width=\"100%\">"
       << "<caption><a href=\"/view?f=" << urlencode(i->first)
       << "&q=" << urlencode(search_string) << "\">" << i->first
       << "</a></caption><tr><td class=\"lines\"><pre>";

    auto& f = i->second;

    std::copy(f.m_line_numbers.begin(), f.m_line_numbers.end(),
              std::ostream_iterator<std::size_t>(os, "\n"));

    os << "</pre></td><td class=\"text\"><pre>";

    for (auto j = f.m_lines.begin(); j != f.m_lines.end(); ++j)
      os << html_escape(*j) << "\n";

    os << "</pre></td></tr></table></div><br>\n";
  }

  os << footer();

  return serve_page(os.str());
}

std::vector<std::string> handler(database& db, const bfs::path& doc_root,
                                 const http_request& req) {
  std::cout << "New request: \"" << req.m_resource << "\"\n";

  try {
    http_query q = parse_http_query(req.m_resource);

    if (q.m_action == "/") return serve_page(header(""));
    if (q.m_action == "/view") return handle_view(db, q);
    if (q.m_action == "/search") return handle_search(db, q);

    return serve_file(doc_root / q.m_action);
  }
  catch (const std::exception& e) {
    std::ostringstream os;
    os << header("") << "<div id=\"error\">" << e.what() << "</div>"
       << footer();

    return serve_page(os.str());
  }
}
}

void serve(const bfs::path& cdb_path, const options& opt) {
  config cfg = load_config(cdb_path / "config");

  serve_init(cdb_path / "www");

  file_lock lock(cdb_path / "lock");
  lock.lock_sharable();

  database_ptr db = open_database(cdb_path / "db");

  bas::io_service iosvc;

  bfs::path docroot = cdb_path / "www";
  httpd server(
      iosvc, "0.0.0.0", cfg.get_value("serve-port"),
      [&](const http_request& req) { return handler(*db, docroot, req); });

  std::cout << "CodeDB serving" << std::endl;
  iosvc.run();
}
