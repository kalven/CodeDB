// CodeDB - public domain - 2010 Daniel Andersson
#include "ticks.hpp"

#ifdef _MSC_VER

#include <windows.h>

int cpu_mhz() {
  HKEY key = NULL;
  LONG result = RegOpenKeyEx(
      HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
      0, KEY_READ, &key);
  if (result != ERROR_SUCCESS) return -1;

  DWORD mhz;
  DWORD mhz_size = sizeof(mhz);
  result = RegQueryValueEx(key, "~MHz", 0, 0, reinterpret_cast<BYTE*>(&mhz),
                           &mhz_size);

  RegCloseKey(key);

  if (result != ERROR_SUCCESS) return -1;

  return static_cast<int>(mhz);
}

#else

#include <cstdio>

int cpu_mhz() {
  int mhz = -1;

  if (std::FILE* f = std::fopen("/proc/cpuinfo", "r")) {
    double rate;

    for (char line[40]; std::fgets(line, sizeof(line), f);) {
      if (std::sscanf(line, "cpu MHz : %lf", &rate) == 1) {
        mhz = static_cast<int>(rate);
        break;
      }
    }

    std::fclose(f);
  }

  return mhz;
}

#endif
