// CodeDB - public domain - 2010 Daniel Andersson

#ifndef CODEDB_SERVE_UTIL_HPP
#define CODEDB_SERVE_UTIL_HPP

#include "nsalias.hpp"

#include <boost/filesystem/path.hpp>

#include <vector>
#include <string>
#include <map>

struct http_query
{
    std::string                        m_action;
    std::map<std::string, std::string> m_args;
};

http_query parse_http_query(const std::string& str);

std::string html_escape(const std::string& str);

std::string urldecode(const std::string& str);
std::string urlencode(const std::string& str);

std::vector<std::string> serve_page(std::string content);
std::vector<std::string> serve_file(const bfs::path& path);

#endif
