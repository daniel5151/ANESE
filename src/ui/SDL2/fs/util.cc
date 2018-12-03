#include "util.h"

#include <stdlib.h>
#include <errno.h>

#ifdef WIN32
  #include <Windows.h>
  #include <direct.h>
#else
  #include <sys/stat.h>
#endif

namespace ANESE_fs {
  namespace util {

// this is pretty jank
void get_abs_path(char* abs_path, const char* path, unsigned int n) {
#ifdef WIN32
  GetFullPathName(path, n, abs_path, nullptr);
#else
  const char* _ = realpath(path, abs_path);
  (void)n;
  (void)_;
#endif
}

int create_directory(const char* path) {
#ifdef WIN32
  int status = _mkdir(path);
#else
  int status = mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
#endif
  if (status == -1)
    if (errno == EEXIST)
      return 0;
  return status;
}

}}
