// CodeDB - public domain - 2010 Daniel Andersson
#pragma once

#include <cstdint>
#include <cstring>
#include <ostream>
#include <string>

typedef std::uint32_t db_uint;

inline db_uint read_binary(const std::string& src, std::size_t index)
{
    db_uint result;
    std::memcpy(&result, &src[index], sizeof(result));
    return result;
}

inline void write_binary(std::ostream& dest, db_uint value)
{
    dest.write(reinterpret_cast<const char*>(&value), sizeof(value));
}

inline void write_binary(std::string& dest, db_uint value)
{
    const char* ptr = reinterpret_cast<const char*>(&value);
    dest.append(ptr, ptr + sizeof(value));
}

inline void write_binary_at(std::string& dest, std::size_t index, db_uint value)
{
    std::memcpy(&dest[index], &value, sizeof(value));
}
