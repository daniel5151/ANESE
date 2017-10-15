#include "common/util.h"
#include "nes/cartridge/cartridge.h"
#include "nes/nes.h"

#include <iostream>
#include <fstream>

int main(int argc, const char* argv[]) {
  // Parse args
  if (argc < 2) {
    std::cerr << "usage: anese [.nes]\n";
    return -1;
  }

  // open ROM from file
  std::ifstream rom_file (argv[1]);

  if (!rom_file.is_open()) {
    std::cerr << "could not open '" << argv[1] << "'\n";
    return -1;
  }

  // get length of file
  rom_file.seekg(0, rom_file.end);
  u32 data_len = rom_file.tellg();
  rom_file.seekg(0, rom_file.beg);

  u8* data = new u8 [data_len];

  rom_file.read((char*) data, data_len);

  // Generate cartridge from data
  Cartridge* rom_cart = new Cartridge (data, data_len);

  if (rom_cart->isValid()) {
    std::cout << "iNES file loaded successfully!\n";
  } else {
    std::cerr << "Given file was not an iNES file!\n";
    return -1;
  }

  // Init NES
  NES nes = NES ();

  // Slap in a cartridge
  // (don't forget to blow on it)
  rom_cart->blowOnContacts();
  rom_cart->blowOnContacts();
  nes.loadCartridge(rom_cart);

  /* play vidya */
  nes.start();

  // Throw away the Cartridge :(
  delete rom_cart;

  return 0;
}
