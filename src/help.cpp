// CodeDB - public domain - 2010 Daniel Andersson

#include "help.hpp"
#include "options.hpp"

#include <iostream>

void help(const options& opt) {
  if (opt.m_args.empty()) {
    std::cout << "usage: cdb [--help] COMMAND [ARGS]\n\n"
              << "The available cdb commands are:\n"
              << "  build    Create the code db index\n"
              << "  config   Get and set db options\n"
              << "  find     Search the code db\n"
              << "  init     Create an empty db\n"
              << "  serve    Starts a local HTTP server\n\n"
              << "See 'cdb help COMMAND' for more information on a specific "
                 "command.\n";
    return;
  }

  const std::string& topic = opt.m_args[0];

  if (topic == "build") {
    std::cout
        << "build: Build the code db index. Recursively indexes files that "
           "match\n"
        << "the regular expression specified by the 'file-include' "
           "configuration.\n"
        << "Directories that match the 'dir-exclude' regex are ignored.\n\n"
        << "usage: cdb build\n\n"
        << "Valid options:\n"
        << "  -v : Verbose\n";
  } else if (topic == "find") {
    std::cout << "find: Search the code db. Only files found in and below the "
                 "current\n"
              << "working directory are considered by default.\n\n"
              << "usage: cdb find PATTERN...\n\n"
              << "Valid options:\n"
              << "  -v : Treat patterns as verbatim strings\n"
              << "  -i : Case insensitive search\n"
              << "  -a : Search the entire code db\n";
  } else if (topic == "init") {
    std::cout << "init: Initialize a code db in the current directory.\n\n"
              << "usage: cdb init\n\n";
  } else if (topic == "config") {
    std::cout << "config: Get and set code db options.\n\n"
              << "usage: cdb config             : show all config keys\n"
              << "usage: cdb config KEY         : show configuration for KEY\n"
              << "usage: cdb config KEY VALUE   : update KEY to a new VALUE\n";
  } else if (topic == "serve") {
    std::cout << "serve: Starts a local HTTP server that allows searching and "
                 "browsing\n"
              << "the code db.\n\n"
              << "usage: cdb serve\n\n"
              << "Valid options:\n"
              << "  -d ARG : Location of the code db root to serve\n";
  }
}
