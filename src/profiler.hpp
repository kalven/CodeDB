#pragma once

#include "ticks.hpp"

#include <cstdint>

struct profiler
{
    const char*   m_name;
    std::uint32_t m_count;
    std::uint64_t m_start;
    std::uint64_t m_total;
};

inline void profile_start(profiler& p)
{
    p.m_start = getticks();
}

inline void profile_stop(profiler& p)
{
    auto now = getticks();
    p.m_total += (now - p.m_start);
    ++p.m_count;
}

profiler& make_profiler(const char* name);

void profiler_report();

class profile_scope
{
  public:
    profile_scope(profiler& p)
      : m_p(p)
    {
        profile_start(m_p);
    }

    profile_scope(const char* name)
      : m_p(make_profiler(name))
    {
        profile_start(m_p);
    }

    ~profile_scope()
    {
        profile_stop(m_p);
    }

  private:
    profiler& m_p;
};
