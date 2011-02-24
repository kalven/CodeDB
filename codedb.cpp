// CodeDB - public domain - 2010 Daniel Andersson

#include "nsalias.hpp"
#include "options.hpp"
#include "config.hpp"
#include "build.hpp"
#include "regex.hpp"
#include "serve.hpp"
#include "init.hpp"
#include "find.hpp"
#include "help.hpp"

#include <boost/filesystem.hpp>
#include <boost/xpressive/regex_error.hpp>

#include <iostream>
#include <fstream>
#include <string>
#include <cctype>

bfs::path make_absolute(const std::string& path_str)
{
    return bfs::system_complete(bfs::path(path_str));
}

bfs::path find_codedb_path(const bfs::path& p)
{
    if(p.empty())
        return p;

    if(bfs::exists(p / ".codedb"))
        return p / ".codedb";

    return find_codedb_path(p.parent_path());
}

bfs::path require_codedb_path(const bfs::path& p)
{
    bfs::path cdb_path = find_codedb_path(p);
    if(cdb_path.empty())
        throw std::runtime_error("CodeDB not found (see: help init)");
    return cdb_path;
}

bfs::path require_codedb_path(const options& opt)
{
    auto it = opt.m_options.find("-d");
    return it == opt.m_options.end()
        ? require_codedb_path(bfs::initial_path())
        : require_codedb_path(it->second);
}

int main(int argc, char** argv)
{
    try
    {
        options opt = parse_cmdline(argc, argv);
        const bfs::path cdb_path = require_codedb_path(opt);

        switch(opt.m_mode)
        {
            case options::init:
                init();
                break;
            case options::config:
                run_config(cdb_path, opt);
                break;
            case options::build:
                build(cdb_path, opt);
                break;
            case options::find:
                find(cdb_path, opt);
                break;
            case options::serve:
                serve(cdb_path, opt);
                break;
            case options::undefined:
                std::cout << "cdb: '" << opt.m_args[0] << "' is not a cdb-command. See 'cdb --help'.\n";
                break;
            case options::help:
                help(opt);
                break;
            default:
                std::cout << "Not implemented '" << opt.m_args.at(0) << "'\n";
                break;
        }
    }
    catch(const bxp::regex_error& error)
    {
        std::string desc = error.what();
        if(!desc.empty())
            desc[0] = std::toupper(desc[0]);

        std::cerr << "Error: invalid regex";
        if(const std::string* re = boost::get_error_info<errinfo_regex_string>(error))
            std::cerr << " '" << *re << "'";
        std::cerr << "\n  " << desc << std::endl;
    }
    catch(const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}
