#include "compress.hpp"
#include "snappy.h"

void snappy_compress(const std::string& input, std::string& output)
{
    snappy::Compress(input.c_str(), input.size(), &output);
}

void snappy_uncompress(const char* begin, const char* end, std::string& output)
{
    snappy::Uncompress(begin, end - begin, &output);
}
