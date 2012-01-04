// CodeDB - public domain - 2010 Daniel Andersson

#ifndef CODEDB_DATABASE_HPP
#define CODEDB_DATABASE_HPP

#include "nsalias.hpp"
#include "serialization.hpp"

#include <boost/filesystem/path.hpp>

#include <cstdint>
#include <string>
#include <memory>

class config;

struct db_file
{
    const char* m_name_start;
    const char* m_name_end;
    const char* m_start;
    const char* m_end;
};

class db_chunk
{
  public:
    db_chunk(const std::string& data);

    bool next_file(db_file&);

  private:
    const std::string& m_data;
    db_uint            m_data_offset;
    db_uint            m_count;
    db_uint            m_current;
};

class database
{
  public:
    virtual ~database();

    virtual void rewind() = 0;
    virtual bool next_chunk(std::pair<const char*, const char*>&) = 0;
};

typedef std::unique_ptr<database> database_ptr;

database_ptr open_database(const bfs::path& blob);

#endif
