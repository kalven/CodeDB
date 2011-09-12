// CodeDB - public domain - 2010 Daniel Andersson

#ifndef CODEDB_BUILD_HPP
#define CODEDB_BUILD_HPP

#include "nsalias.hpp"

#include <boost/filesystem.hpp>

struct options;

void build(const bfs::path& cdb_path, const options& opt);

#endif
