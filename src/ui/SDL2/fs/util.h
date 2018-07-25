#pragma once

namespace ANESE_fs {
  namespace util {
    void get_abs_path(char* abs_path, const char* path, unsigned int n);

    int create_directory(const char* path);
  }
}
