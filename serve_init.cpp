// CodeDB - public domain - 2010 Daniel Andersson

#include "serve_init.hpp"

#include <boost/filesystem/fstream.hpp>

#include <stdexcept>

const char file_index_html[] =
    "<!DOCTYPE html>\n<meta charset=utf-8>\n<title>CodeDB</title>\n<form action=sear"
    "ch method=get>\n<input name=q type=search placeholder=\"Type a regex to search\""
    " autofocus>\n<input type=submit value=Search>\n</form>\n";

const char file_style_css[] =
    "body\n{\n    font-family: 'Bitstream Vera Sans Mono','Courier',monospace;\n    "
    "font-size: 12px;\n}\n\npre\n{\n    margin: 0px;\n}\n\ndiv.file\n{\n    border: "
    "1px solid #bbb;\n}\n\ntable.code\n{\n    border-spacing: 0px;\n}\n\ntable.code "
    "caption\n{\n    background: -webkit-gradient(linear, left top, left bottom, fro"
    "m(#FFF), to(#DDD));\n    background: -moz-linear-gradient(top,  #FFF,  #DDD);\n"
    "    border-bottom: 1px solid #AAA;\n    color: #333;\n    text-align: left;\n  "
    "  padding-left: 0.5em;\n    padding-top: 0.3em;\n    padding-bottom: 0.3em;\n}\n"
    "\ntd.lines\n{\n    background-color: #EEE;\n    border-right: 1px solid #CCC;\n"
    "    color: #888;\n    text-align: right;\n    padding-right: 0.5em;\n    paddin"
    "g-left: 0.5em;\n    width: 3em;\n}\n\ntd.text\n{\n    padding-left: 1em;\n}\n\n"
    "span.hl\n{\n    background-color: #fbf;\n    display: inline-block;\n    width:"
    " 100%;\n}\n";

namespace
{
    void write_file(const bfs::path& dest, const char* content, std::size_t size)
    {
        if(exists(dest))
            return;

        bfs::ofstream out(dest, bfs::ofstream::binary);
        if(!out.is_open())
            throw std::runtime_error("Unable to open " + dest.string() + " for writing");

        out.write(content, size);
    }
}

void serve_init(const bfs::path& serve_path)
{
    // Make sure the directory exists, or create it
    if(exists(serve_path))
    {
        if(!is_directory(serve_path))
            throw std::runtime_error(serve_path.string() + " already exists");
    }
    else
    {
        create_directory(serve_path);
    }

    write_file(serve_path / "index.html", file_index_html, sizeof(file_index_html) - 1);
    write_file(serve_path / "style.css", file_style_css, sizeof(file_style_css) - 1);
}
