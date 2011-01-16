// CodeDB - public domain - 2010 Daniel Andersson

#ifndef CODEDB_FINDER_HPP
#define CODEDB_FINDER_HPP

#include <boost/xpressive/xpressive_fwd.hpp>
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/filesystem/path.hpp>

#include <vector>

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

class finder
{
  public:
    finder(const boost::filesystem::path& packed,
           const boost::filesystem::path& index);

    void search(std::size_t                     prefix_size,
                const boost::xpressive::cregex& re,
                const boost::xpressive::cregex& file_re,
                match_receiver&                 receiver);

  private:

    void load_index(const boost::filesystem::path& path);

    typedef std::vector<std::pair<std::size_t, std::string> > index_t;

    boost::interprocess::file_mapping  m_mapping;
    boost::interprocess::mapped_region m_region;
    index_t                            m_index;
    const char*                        m_data;
    const char*                        m_data_end;
};

#endif
