// CodeDB - public domain - 2010 Daniel Andersson

#ifndef CODEDB_FIND_HPP
#define CODEDB_FIND_HPP

#include <boost/filesystem.hpp>

struct options;

void find(const boost::filesystem::path& cdb_path, const options& opt);

#endif
