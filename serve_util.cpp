// CodeDB - public domain - 2010 Daniel Andersson

#include "serve_util.hpp"

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/fstream.hpp>

#include <sstream>
#include <cstring>
#include <cctype>

namespace
{
    inline int hexdigit(char c)
    {
        if(c >= '0' && c <= '9')
            return c - '0';
        if(c >= 'A' && c <= 'F')
            return c - 'A' + 10;
        if(c >= 'a' && c <= 'f')
            return c - 'a' + 10;

        return 0;
    }

    inline char hexchar(int x)
    {
        return x < 10 ? '0' + x : 'A' + (x - 10);
    }

    const char* content_type(const bfs::path& path)
    {
        const std::string& ext =
            boost::algorithm::to_lower_copy(path.extension());

        if(ext == ".html")
            return "text/html; charset=utf-8";
        if(ext == ".css")
            return "text/css; charset=utf-8";
        if(ext == ".png")
            return "image/png";
        if(ext == ".jpeg" || ext == ".jpg")
            return "image/jpeg";
        if(ext == ".js")
            return "text/javascript";

        return "text/plain; charset=utf-8";
    }
}

http_query parse_http_query(const std::string& in)
{
    http_query result;

    auto qm = in.find('?');

    result.m_action = in.substr(0, qm);

    if(qm != std::string::npos)
    {
        std::stringstream is(in.substr(qm + 1));

        std::string kv;
        while(std::getline(is, kv, '&'))
        {
            auto eq = kv.find('=');
            if(eq == std::string::npos)
            {
                result.m_args[urldecode(kv)];
            }
            else
            {
                result.m_args[urldecode(kv.substr(0, eq))] =
                    urldecode(kv.substr(eq + 1));
            }
        }
    }

    return result;
}

std::string get_arg(const http_query& query, const std::string& index)
{
    auto it = query.m_args.find(index);
    if(it == query.m_args.end())
        return std::string();

    return it->second;
}

std::string html_escape(const std::string& str)
{
    std::string result;
    result.reserve(str.size());

    for(auto i = str.begin(); i != str.end(); ++i)
    {
        switch(*i)
        {
            case '<':
                result += "&lt;";
                break;
            case '>':
                result += "&gt;";
                break;
            case '&':
                result += "&amp;";
                break;
            default:
                result += *i;
        }
    }

    return result;
}

std::string quot_escape(const std::string& str)
{
    std::string result;
    result.reserve(str.size());

    for(auto i = str.begin(); i != str.end(); ++i)
    {
        switch(*i)
        {
            case '"':
                result += "&quot;";
                break;
            default:
                result += *i;
        }
    }

    return result;
}

std::string urldecode(const std::string& str)
{
    if(str.empty())
        return str;

    std::string result = str;
    char* out = &result[0];

    for(const char* i = result.c_str(); *i; ++i)
    {
        switch(*i)
        {
            case '%':
                if(isxdigit(i[1]) && isxdigit(i[2]))
                {
                    *out++ = char(hexdigit(i[1]) * 16 + hexdigit(i[2]));
                    i += 2;
                }
                break;
            case '+':
                *out++ = ' ';
                break;
            default:
                *out++ = *i;
                break;
        }
    }

    result.resize(out - &result[0]);
    return result;
}

std::string urlencode(const std::string& str)
{
    std::string result;
    for(auto i = str.begin(); i != str.end(); ++i)
    {
        char c = *i;
        if((c >= 'A' && c <= 'Z') ||
           (c >= 'a' && c <= 'z') ||
           (c >= '0' && c <= '9') ||
           std::strchr("-_.~", c))
        {
            result += c;
        }
        else
        {
            unsigned char u = static_cast<unsigned char>(c);
            result += '%';
            result += hexchar(u >> 4);
            result += hexchar(u & 15);
        }
    }

    return result;
}

std::vector<std::string> serve_page(std::string content)
{
    std::vector<std::string> result;

    std::ostringstream header;
    header << 
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html; charset=utf-8\r\n"
        "Content-Length: " << content.size() << "\r\n"
        "Connection: close\r\n\r\n";

    result.push_back(header.str());
    result.push_back(std::move(content));

    return result;
}

std::vector<std::string> serve_file(const bfs::path& path)
{
    if(!bfs::exists(path))
        return serve_page("not found!");
    if(!bfs::is_regular(path))
        return serve_page("not regular!");

    bfs::ifstream file(path);
    if(!file.is_open())
        return serve_page("unable to open!");

    std::size_t size = static_cast<std::size_t>(bfs::file_size(path));

    std::string content(size, 0);
    file.read(&content[0], size);

    std::vector<std::string> result;

    std::ostringstream header;
    header <<
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: " << content_type(path) << "\r\n"
        "Content-Length: " << content.size() << "\r\n"
        "Connection: close\r\n\r\n";

    result.push_back(header.str());
    result.push_back(std::move(content));

    return result;
}
