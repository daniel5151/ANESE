#pragma once

#ifdef WIN32
  #define NOMINMAX // breaks args library, #thankswindows
  #include <Windows.h>
  #undef NO_ERROR // breaks my own code, #thankswindows
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
  (void)realpath(path, out);
#endif
}

}}
