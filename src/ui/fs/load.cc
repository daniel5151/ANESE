#include "load.h"

#include "nes/cartridge/rom_file.h"

#include "parse_rom.h"

#include <fstream>
#include <iostream>
#include <string>

#include <miniz_zip.h>

/*----------  Utlities  ----------*/

#include <algorithm>
static inline std::string get_file_ext(const char* filename) {
  std::string filename_str (filename);
  std::string ext = filename_str.substr(filename_str.find_last_of("."));
  std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
  return ext;
}

/*----------  File Data Loaders  ----------*/

// Loads file directly into memory
bool load_file(const char* filepath, u8*& data, uint& data_len) {
  if (!filepath) {
    fprintf(stderr, "[Load] filepath == nullptr in load_file!\n");
    assert(false);
  }

  std::ifstream rom_file (std::string(filepath), std::ios::binary);

  if (!rom_file.is_open()) {
    fprintf(stderr, "[Load] Could not open '%s'\n", filepath);
    return false;
  }

  // get length of file
  rom_file.seekg(0, rom_file.end);
  std::streamoff rom_file_size = rom_file.tellg();
  rom_file.seekg(0, rom_file.beg);

  if (rom_file_size == -1) {
    fprintf(stderr, "[Load] Could not read '%s'\n", filepath);
    return false;
  }

  data_len = rom_file_size;
  data = new u8 [data_len];
  rom_file.read((char*) data, data_len);

  fprintf(stderr, "[Load] Successfully read '%s'\n", filepath);

  return true;
}

// Searches for valid roms inside .zip files, and loads them into memory
static bool load_zip_file_data(const char* filepath, u8*& data, uint& data_len) {
  if (!filepath) {
    fprintf(stderr, "[Load] filepath == nullptr in load_zip_file_data!\n");
    assert(false);
  }

  mz_zip_archive zip_archive;
  memset(&zip_archive, 0, sizeof zip_archive);
  mz_bool status = mz_zip_reader_init_file(
    &zip_archive,
    filepath,
    0
  );
  if (!status) {
    fprintf(stderr, "[Load][.zip] Could not read '%s'\n", filepath);
    mz_zip_reader_end(&zip_archive);
    return false;
  }

  // Try to find a .nes file in the archive
  for (uint i = 0; i < mz_zip_reader_get_num_files(&zip_archive); i++) {
    mz_zip_archive_file_stat file_stat;
    mz_zip_reader_file_stat(&zip_archive, i, &file_stat);

    std::string file_ext = get_file_ext(file_stat.m_filename);

    if (file_ext == ".nes") {
      printf("[Load][.zip][UnZip] Found .nes file in archive: '%s'\n", file_stat.m_filename);

      size_t uncomp_size;
      void* p = mz_zip_reader_extract_file_to_heap(
        &zip_archive,
        file_stat.m_filename,
        &uncomp_size,
        0
      );

      if (!p) {
        printf("[Load][.zip][UnZip] Could not decompress '%s'\n", filepath);
        mz_free(p);
        mz_zip_reader_end(&zip_archive);
        return false;
      }

      // Nice! We got data!
      data_len = uncomp_size;
      data = new u8 [data_len];
      memcpy(data, p, data_len);

      fprintf(stderr, "[Load][.zip] Successfully read '%s'\n", filepath);

      // Free the redundant decompressed file mem
      mz_free(p);

      // Close the archive, freeing any resources it was using
      mz_zip_reader_end(&zip_archive);
    }
  }

  return true;
}

/*----------  Public Interface  ----------*/

// Given a filepath, tries to open and parse it as a NES ROM.
// Returns a valid ROM_File, or a nullptr if something went wrong
ROM_File* load_rom_file(const char* filepath) {
  if (!filepath) {
    fprintf(stderr, "[Load] filepath == nullptr in load_rom_file!\n");
    assert(false);
  }

  // Be lazy, and just load the entire file into memory
  u8*  data = nullptr;
  uint data_len = 0;

  std::string rom_ext = get_file_ext(filepath);
  /**/ if (rom_ext == ".nes") load_file(filepath, data, data_len);
  else if (rom_ext == ".zip") load_zip_file_data(filepath, data, data_len);
  else {
    fprintf(stderr, "[Load] Invalid file extension.\n");
    return nullptr;
  }

  // Determine ROM type, and parse it
  switch(rom_type(data, data_len)) {
  using namespace FileFormat;
  case iNES: return parse_iNES(data, data_len);
  case NES2: return parse_iNES(data, data_len); // ignore NES2.0 headers
  case INVALID: return nullptr;
  }
}
