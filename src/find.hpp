// CodeDB - public domain - 2010 Daniel Andersson

#ifndef CODEDB_FIND_HPP
#define CODEDB_FIND_HPP

#include "nsalias.hpp"

#include <boost/filesystem.hpp>

struct options;

void find(const bfs::path& cdb_path, const options& opt);

#endif
