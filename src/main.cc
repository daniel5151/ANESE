#include "util.h"
#include "ines.h"

#include <iostream>
#include <fstream>

int main(int argc, const char* argv[]) {
  // Parse args
  if (argc < 2) {
    std::cerr << "usage: anese [.nes]\n";
    return -1;
  }

  // open rom from file
  std::ifstream rom_file_stream(argv[1]);

  if (!rom_file_stream.is_open()) {
    std::cerr << "could not open '" << argv[1] << "'\n";
    return -1;
  }

  // Parse the ROM into a iNES object
  INES rom_file (rom_file_stream);
  // INES rom_file (std::cin);


  if (rom_file.is_valid) {
    std::cout << "iNES file loaded successfully!\n";
  } else {
    std::cerr << "Given file was not an iNES file!\n";
    return -1;
  }

  return 0;
}
