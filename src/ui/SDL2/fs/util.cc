#include "util.h"

#ifdef WIN32
  #include <Windows.h>
#else
  #include <stdlib.h>
#endif

namespace ANESE_fs {
  namespace util {

// this is pretty jank
void get_abs_path(char* abs_path, const char* path, unsigned int n) {
#ifdef WIN32
  GetFullPathName(path, n, abs_path, nullptr);
#else
  (void)n;
  (void)realpath(path, abs_path);
#endif
}

}}
