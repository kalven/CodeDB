// CodeDB - public domain - 2010 Daniel Andersson

#include "options.hpp"

#include <deque>
#include <stdexcept>

namespace
{
    void add_single(std::map<std::string, std::string>& options, const std::string& opt, const std::string value = "")
    {
        if(options.count(opt))
            throw std::runtime_error("Option " + opt + " already specified");
        options[opt] = value;
    }
}

options parse_cmdline(int argc, char** argv)
{
    options result;

    std::deque<std::string> args(argv+1, argv+argc);

    if(args.empty() || args[0] == "-h" || args[0] == "--help" || args[0] == "help")
    {
        result.m_mode = options::help;
        if(args.size() == 2)
            result.m_args.push_back(args[1]);
        if(args.size() > 2)
            throw std::runtime_error("Invalid argument");
    }
    else if(args[0] == "init")
    {
        result.m_mode = options::init;
        if(args.size() > 1)
            throw std::runtime_error("Invalid argument");
    }
    else if(args[0] == "build")
    {
        result.m_mode = options::build;
        if(args.size() > 1)
            throw std::runtime_error("Invalid argument");
    }
    else if(args[0] == "config")
    {
        result.m_mode = options::config;
        if(args.size() > 3)
            throw std::runtime_error("Invalid argument");
        result.m_args.insert(result.m_args.end(), args.begin()+1, args.end());
    }
    else if(args[0] == "find")
    {
        result.m_mode = options::find;
        auto i = args.begin()+1;
        for(;i != args.end() && i->at(0) == '-'; ++i)
        {
            if(*i == "--")
            {
                ++i;
                break;
            }

            if(*i == "-a" || *i == "-i" || *i == "-v")
                add_single(result.m_options, *i);
            if(*i == "-f")
            {
                if(++i == args.end())
                    throw std::runtime_error("-f requires an argument");
                add_single(result.m_options, "-f", *i);
            }
            if(*i == "-F")
            {
                if(++i == args.end())
                    throw std::runtime_error("-F requires an argument");
                add_single(result.m_options, "-F", *i);
            }
        }

        result.m_args.insert(result.m_args.end(), i, args.end());
        if(result.m_args.empty())
            throw std::runtime_error("find requires a query");
    }
    else
    {
        result.m_mode = options::undefined;
        result.m_args.push_back(args[0]);
    }

    return result;
}
