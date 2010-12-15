// CodeDB - public domain - 2010 Daniel Andersson

#ifndef CODEDB_FILE_LOCK_HPP
#define CODEDB_FILE_LOCK_HPP

#include <boost/interprocess/sync/file_lock.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/noncopyable.hpp>

#include <string>

class file_lock : public boost::noncopyable
{
  public:
    file_lock(const boost::filesystem::path&);
    ~file_lock();

    void lock_exclusive();
    void lock_sharable();

  private:
    void create_lock();

    enum state
    {
        unlocked,
        shared,
        exclusive
    };

    boost::interprocess::file_lock m_lock;
    boost::filesystem::path        m_path;
    state                          m_state;
};

#endif
