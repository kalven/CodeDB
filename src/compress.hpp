// CodeDB - public domain - 2010 Daniel Andersson

#ifndef CODEDB_COMPRESS_HPP
#define CODEDB_COMPRESS_HPP

#include <string>

void snappy_compress(const std::string& input, std::string& output);
void snappy_uncompress(const std::string& input, std::string& output);

#endif
