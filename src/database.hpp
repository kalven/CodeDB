// CodeDB - public domain - 2010 Daniel Andersson

#ifndef CODEDB_DATABASE_HPP
#define CODEDB_DATABASE_HPP

#include "nsalias.hpp"

#include <boost/filesystem/path.hpp>
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>

#include <vector>

class database
{
  public:
    struct file
    {
        std::string m_name;
        const char* m_start;
        const char* m_end;
    };

    database(const bfs::path& packed, const bfs::path& index);

    std::size_t size() const
    {
        return m_index.size();
    }

    const file& operator[](std::size_t i) const
    {
        return m_index[i];
    }

    const file* find_by_name(const std::string& name) const
    {
        for(std::size_t i = 0; i != m_index.size(); ++i)
            if(m_index[i].m_name == name)
                return &m_index[i];
        return 0;
    }

  private:

    void load_index(const bfs::path& path);

    bip::file_mapping  m_mapping;
    bip::mapped_region m_region;
    std::vector<file>  m_index;
};

#endif
