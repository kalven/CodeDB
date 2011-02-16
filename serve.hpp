// CodeDB - public domain - 2010 Daniel Andersson

#ifndef CODEDB_SERVE_HPP
#define CODEDB_SERVE_HPP

#include <boost/filesystem.hpp>

struct options;

void serve(const boost::filesystem::path& cdb_path, const options& opt);

#endif
