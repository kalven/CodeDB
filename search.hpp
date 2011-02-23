// CodeDB - public domain - 2010 Daniel Andersson

#ifndef CODEDB_SEARCH_HPP
#define CODEDB_SEARCH_HPP

#include "nsalias.hpp"

#include <boost/xpressive/xpressive_fwd.hpp>
#include <boost/xpressive/regex_constants.hpp>

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

// Default regex options for searches
const bxp::regex_constants::syntax_option_type default_regex_options =
    bxp::regex_constants::ECMAScript|
    bxp::regex_constants::not_dot_newline|
    bxp::regex_constants::optimize;

// Search the memory between begin and end for all matches. If a match is found,
// the passed match_object is updated with line and position information before
// being passed to the receiver.
void search(const char*        begin,
            const char*        end,
            const bxp::cregex& re,
            match_info&        minfo,
            match_receiver&    receiver);

// Search an entire db. Only files that match the file_re will be considered.
void search_db(const database&    db,
               const bxp::cregex& re,
               const bxp::cregex& file_re,
               std::size_t        prefix_size,
               match_receiver&    receiver);

#endif

