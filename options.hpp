// CodeDB - public domain - 2010 Daniel Andersson

#ifndef CODEDB_OPTIONS_HPP
#define CODEDB_OPTIONS_HPP

#include <string>
#include <vector>
#include <map>

struct options
{
    enum mode { undefined, help, init, config, build, find };
    
    mode                               m_mode;
    std::map<std::string, std::string> m_options;
    std::vector<std::string>           m_args;
};

options parse_cmdline(int argc, char** argv);

#endif
