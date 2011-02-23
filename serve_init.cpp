// CodeDB - public domain - 2010 Daniel Andersson

#include "serve_init.hpp"

#include <boost/filesystem/fstream.hpp>

#include <stdexcept>

const char file_style_css[] =
    "body\n{\n    margin: 0px;\n    background: #ddd;\n    border-top: 80px solid #2"
    "22;\n    font-family: Arial;\n    font-size: 12px;\n}\n\n#body\n{\n    padding:"
    " 2em;\n}\n\n#logo\n{\n    position: absolute;\n    left: 10px;\n    top: 8px;\n"
    "}\n\n#search\n{\n    position: absolute;\n    top: 24px;\n    left: 240px;\n   "
    " color: #999;\n}\n\ninput\n{\n    color: #fff;\n    border: 1px solid #666;\n  "
    "  background: #444;\n    outline: none;\n}\n\ninput#q\n{\n    padding: 6px 6px "
    "6px 8px;\n    min-width: 500px;\n    border-top-left-radius: 1em;\n    border-b"
    "ottom-left-radius: 1em;\n}\n\ninput#s\n{\n    padding: 6px 8px 6px 6px;\n    bo"
    "rder-top-right-radius: 1em;\n    border-bottom-right-radius: 1em;\n}\n\npre\n{\n"
    "    margin: 0px;\n}\n\ndiv.file\n{\n    border: 1px solid #bbb;\n}\n\ntable.cod"
    "e\n{\n    border-spacing: 0px;\n}\n\ntable.code caption\n{\n    background: -we"
    "bkit-gradient(linear, left top, left bottom, from(#FFF), to(#DDD));\n    backgr"
    "ound: -moz-linear-gradient(top,  #FFF,  #DDD);\n    border-bottom: 1px solid #A"
    "AA;\n    color: #333;\n    text-align: left;\n    padding-left: 0.5em;\n    pad"
    "ding-top: 0.3em;\n    padding-bottom: 0.3em;\n}\n\ntd.lines\n{\n    background-"
    "color: #EEE;\n    border-right: 1px solid #CCC;\n    color: #888;\n    text-ali"
    "gn: right;\n    padding-right: 0.5em;\n    padding-left: 0.5em;\n    width: 3em"
    ";\n}\n\ntd.text\n{\n    padding-left: 1em;\n    color: #000;\n}\n\nspan.hl\n{\n"
    "    background: #5cf;\n    display: inline-block;\n    width: 100%;\n}\n";

const char file_logo_js[] =
    "window.onload = function() {\nvar canvas = document.getElementById(\"logo\");\n"
    "if(!canvas.getContext) return;\nvar ctx = canvas.getContext(\"2d\");\nvar cdb ="
    " \"CodeDB\";\nvar trans = \"rgba(0,0,0,0)\";\nvar black = \"rgba(0,0,0,1)\";\nc"
    "tx.font = \"bold 50px Trebuchet MS\";\nctx.fillStyle = trans;\nctx.shadowOffset"
    "X = 3;\nctx.shadowOffsetY = 3;\nctx.shadowBlur = 8;\nctx.shadowColor = black;\n"
    "ctx.fillText(cdb, 10, 50);\nctx.shadowColor = trans;\nctx.shadowBlur = 0;\nctx."
    "shadowOffsetX = 0;\nctx.shadowOffsetY = 0;\nctx.strokeStyle = \"#fff\";\nctx.li"
    "neWidth = 3;\nctx.strokeText(cdb, 10, 50);\nvar grd = ctx.createLinearGradient("
    "0,20,0,40);\ngrd.addColorStop(0, \"#28f\");\ngrd.addColorStop(1, \"#3af\");\nct"
    "x.fillStyle = black;\nctx.fillStyle = grd;\nctx.fillText(cdb, 10, 50);\n}\n";

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

    write_file(serve_path / "style.css", file_style_css, sizeof(file_style_css) - 1);
    write_file(serve_path / "logo.js", file_logo_js, sizeof(file_logo_js) - 1);
}
