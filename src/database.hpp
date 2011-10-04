// CodeDB - public domain - 2010 Daniel Andersson

#ifndef CODEDB_DATABASE_HPP
#define CODEDB_DATABASE_HPP

#include "nsalias.hpp"

#include <boost/filesystem/path.hpp>

#include <string>
#include <memory>

class database
{
  public:
    struct file
    {
        std::string m_name;
        const char* m_start;
        const char* m_end;
    };

    virtual ~database();

    virtual void restart() = 0;
    virtual const file* next_file() = 0;
};

typedef std::unique_ptr<database> database_ptr;

database_ptr open_database(const bfs::path& blob, const bfs::path& index);

#endif
