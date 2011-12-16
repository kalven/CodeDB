// CodeDB - public domain - 2010 Daniel Andersson

#ifndef CODEDB_MVAR_HPP
#define CODEDB_MVAR_HPP

#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>

template<class T>
class mvar
{
  public:
    mvar()
      : m_filled(false)
    {
    }

    void put(T& var)
    {
        boost::mutex::scoped_lock lock(m_mutex);

        while(m_filled)
            m_free.wait(lock);

        std::swap(m_var, var);
        m_filled = true;

        m_full.notify_one();
    }

    void get(T& other)
    {
        boost::mutex::scoped_lock lock(m_mutex);

        while(!m_filled)
            m_full.wait(lock);

        std::swap(m_var, other);

        m_filled = false;
        m_free.notify_one();
    }

  private:

    T                m_var;
    bool             m_filled;
    boost::mutex     m_mutex;
    boost::condition m_free;
    boost::condition m_full;
};

#endif
