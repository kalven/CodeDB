// CodeDB - public domain - 2010 Daniel Andersson
#include "profiler.hpp"

#include <iostream>
#include <iomanip>
#include <cstdlib>

profiler s_profilers[20];
unsigned s_profcount = 0;

profiler& make_profiler(const char* name) {
  auto& p = s_profilers[s_profcount++];
  p.m_name = name;

  return p;
}

void profiler_report() {
  if (!std::getenv("CDB_PROFILE")) return;

  int mhz = cpu_mhz();
  if (mhz == -1) return;

  std::cerr << "Counter          Elapsed time      Hits\n"
            << "---------------------------------------\n";

  for (unsigned i = 0; i != s_profcount; ++i) {
    if (!s_profilers[i].m_total) continue;

    auto ms = s_profilers[i].m_total / (mhz * 1000.0);
    std::cerr << std::left << std::setw(15) << s_profilers[i].m_name
              << std::setw(10) << std::right << std::fixed
              << std::setprecision(3) << ms << " ms." << std::setw(10)
              << s_profilers[i].m_count << std::endl;
  }
}
