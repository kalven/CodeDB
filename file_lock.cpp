// CodeDB - public domain - 2010 Daniel Andersson

#include "file_lock.hpp"

#include <boost/filesystem/fstream.hpp>

#include <fstream>
#include <stdexcept>

file_lock::file_lock(const bfs::path& lock_path)
  : m_path(lock_path)
  , m_state(unlocked)
{
}

file_lock::~file_lock()
{
    if(m_state == shared)
        m_lock.unlock_sharable();
    else if(m_state == exclusive)
        m_lock.unlock();
}

void file_lock::lock_exclusive()
{
    create_lock();

    m_lock.lock();
    m_state = exclusive;
}

void file_lock::lock_sharable()
{
    create_lock();

    m_lock.lock_sharable();
    m_state = shared;
}

void file_lock::create_lock()
{
    // Touch the lockfile
    {
        bfs::ofstream lockfile(m_path);
        if(!lockfile.is_open())
            throw std::runtime_error("Unable to create lock file " + m_path.string());
    }

    bip::file_lock lock(m_path.string().c_str());

    m_lock.swap(lock);
}
