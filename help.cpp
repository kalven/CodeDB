// CodeDB - public domain - 2010 Daniel Andersson

#include "help.hpp"
#include "options.hpp"

#include <iostream>

void help(const options& opt)
{
    if(opt.m_args.empty())
    {
        std::cout << "usage: cdb [--help] COMMAND [ARGS]\n\n"
                  << "The available cdb commands are:\n"
                  << "  build    Create the code db index\n"
                  << "  config   Get and set db options\n"
                  << "  find     Search the code db\n"
                  << "  init     Create an empty db\n\n"
                  << "See 'cdb help COMMAND' for more information on a specific command.\n";
        return;
    }

    if(opt.m_args[0] == "build")
    {
        std::cout << "build: Build the code db index. Recursively indexes files that match\n"
                  << "the regular expression specified by the 'file-include' configuration.\n"
                  << "Directories that match the 'dir-exclude' regex are ignored.\n\n"
                  << "usage: cdb build\n\n"
                  << "Valid options:\n"
                  << "  -v  : Verbose\n";
    }
    else if(opt.m_args[0] == "find")
    {
        std::cout << "find: Search the code db. Only files found in and below the current\n"
                  << "working directory are considered by default.\n\n"
                  << "usage: cdb find PATTERN...\n\n"
                  << "Valid options:\n"
                  << "  -v : Treat patterns as verbatim strings\n"
                  << "  -i : Case insensitive search\n"
                  << "  -a : Search the entire code db\n";
    }
    else if(opt.m_args[0] == "init")
    {
        std::cout << "init: Initialize a code db in the current directory\n\n"
                  << "usage: cdb init\n\n";
    }
}
