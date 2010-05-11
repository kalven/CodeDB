// CodeDB - public domain - 2010 Daniel Andersson

#include "options.hpp"
#include "config.hpp"
#include "build.hpp"
#include "init.hpp"
#include "find.hpp"

#include <boost/filesystem.hpp>

#include <string>
#include <fstream>
#include <iostream>

namespace bfs = boost::filesystem;

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

int main(int argc, char** argv)
{
    try
    {
        options opt = parse_cmdline(argc, argv);

        std::cout << "mode: " << opt.m_mode << std::endl;
        std::cout << "options: " << std::endl;
        for(auto i = opt.m_options.begin(); i != opt.m_options.end(); ++i)
            std::cout << "  " << i->first << " => \"" << i->second << "\"" << std::endl;
        std::cout << "args: " << std::endl;
        for(auto i = opt.m_args.begin(); i != opt.m_args.end(); ++i)
            std::cout << "  " << *i << std::endl;
        std::cout << "-----------------------------------" << std::endl;

        switch(opt.m_mode)
        {
            case options::init:
                init();
                break;
            case options::build:
                build(require_codedb_path(bfs::initial_path()));
                break;
            case options::find:
                find(require_codedb_path(bfs::initial_path()));
                break;
            default:
                std::cerr << "Not implemented" << std::endl;
        }
    }
    catch(const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}
