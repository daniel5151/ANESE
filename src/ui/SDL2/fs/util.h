#pragma once

// this is pretty jank
#ifdef WIN32
#include <Windows.h>
static inline void get_abs_path(const char* path, char* out, unsigned int n) {
  GetFullPathName(path, n, out, nullptr);
}
#else
#include <stdlib.h>
static inline void get_abs_path(const char* path, char* out, unsigned int n) {
  (void)n;
  realpath(path, out);
}
#endif
