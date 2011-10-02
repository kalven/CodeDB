// CodeDB - public domain - 2010 Daniel Andersson

#ifndef CODEDB_SEARCH_HPP
#define CODEDB_SEARCH_HPP

#include "nsalias.hpp"
#include "regex.hpp"

class database;

struct match_info
{
    const char* m_file;
    const char* m_full_file;
    const char* m_file_start;
    const char* m_file_end;
    const char* m_line_start;
    const char* m_line_end;
    const char* m_position;
    std::size_t m_line;
};

class match_receiver
{
  public:
    virtual ~match_receiver() {}
    virtual const char* on_match(const match_info&) = 0;
};

// Search the memory between begin and end for all matches. If a match is found,
// the passed match_object is updated with line and position information before
// being passed to the receiver.
void search(const char*     begin,
            const char*     end,
            regex&          re,
            match_info&     minfo,
            match_receiver& receiver);

// Search an entire db. Only files that match the file_re will be considered.
void search_db(const database& db,
               regex&          re,
               regex&          file_re,
               std::size_t     prefix_size,
               match_receiver& receiver);

#endif

