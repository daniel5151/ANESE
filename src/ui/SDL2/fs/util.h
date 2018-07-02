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
static inline void get_abs_path(char* abs_path, const char* path, unsigned int n) {
#ifdef WIN32
  GetFullPathName(path, n, abs_path, nullptr);
#else
  (void)n;
  (void)realpath(path, abs_path);
#endif
}

}}
