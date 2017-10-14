#include "util.h"
#include "ines.h"
#include "cartridge.h"

#include <iostream>
#include <fstream>

int main(int argc, const char* argv[]) {
  // Parse args
  if (argc < 2) {
    std::cerr << "usage: anese [.nes]\n";
    return -1;
  }

  // open ROM from file
  std::ifstream rom_file_stream(argv[1]);

  if (!rom_file_stream.is_open()) {
    std::cerr << "could not open '" << argv[1] << "'\n";
    return -1;
  }

  // Read in data from data_stream of unknown size
  uint8* data = nullptr;
  uint32 data_len = 0;

  const uint CHUNK_SIZE = 0x4000;
  for (uint chunk = 0; rom_file_stream; chunk++) {
    // Allocate new data
    uint8* new_data = new uint8 [CHUNK_SIZE * (chunk + 1)];
    data_len = CHUNK_SIZE * (chunk + 1);

    // Copy old data into the new data
    for (uint i = 0; i < CHUNK_SIZE * chunk; i++)
      new_data[i] = data[i];

    delete data;
    data = new_data;

    // Read another chunk into the data
    rom_file_stream.read((char*) data + CHUNK_SIZE * chunk, CHUNK_SIZE);
  }

  // Generate cartridge from data
  Cartridge rom_cart (data, data_len);


  if (rom_cart.isValid()) {
    std::cout << "iNES file loaded successfully!\n";
  } else {
    std::cerr << "Given file was not an iNES file!\n";
    return -1;
  }

  return 0;
}
