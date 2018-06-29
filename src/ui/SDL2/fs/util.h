#pragma once

#ifdef WIN32
  #include <Windows.h>
#else
  #include <stdlib.h>
#endif

namespace ANESE_fs { namespace util {

// this is pretty jank
static inline void get_abs_path(const char* path, char* out, unsigned int n) {
#ifdef WIN32
  GetFullPathName(path, n, out, nullptr);
#else
  (void)n;
  realpath(path, out);
#endif
}

}}
