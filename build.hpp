// CodeDB - public domain - 2010 Daniel Andersson

#ifndef CODEDB_BUILD_HPP
#define CODEDB_BUILD_HPP

#include <boost/filesystem.hpp>

struct options;

void build(const boost::filesystem::path& cdb_path, const options& opt);

#endif
