#include "shared_state.h"

#include "fs/load.h"

int SharedState::load_rom(const char* rompath) {
  // cleanup previous cart
  for (uint i = 0; i < 4; i++) {
    delete this->savestate[i];
    this->savestate[i] = nullptr;
  }

  fprintf(stderr, "[Load] Loading '%s'\n", rompath);
  Cartridge* cart = new Cartridge (ANESE_fs::load::load_rom_file(rompath));

  switch (cart->status()) {
  case Cartridge::Status::CART_BAD_DATA:
    fprintf(stderr, "[Cart] ROM file could not be parsed!\n");
    delete cart;
    return 1;
  case Cartridge::Status::CART_BAD_MAPPER:
    fprintf(stderr, "[Cart] Mapper %u has not been implemented yet!\n",
      cart->get_rom_file()->meta.mapper);
    delete cart;
    return 1;
  case Cartridge::Status::CART_NO_ERROR:
    fprintf(stderr, "[Cart] ROM file loaded successfully!\n");
    this->current_rom_file = rompath;
    this->cart = cart;
    break;
  }

  // Try to load battery-backed save
  if (!this->config.cli.no_sav) {
    u8* data = nullptr;
    uint len = 0;
    ANESE_fs::load::load_file((this->current_rom_file + ".sav").c_str(), data, len);

    if (!data) fprintf(stderr, "[Savegame][Load] No save data found.\n");
    else {
      fprintf(stderr, "[Savegame][Load] Found save data.\n");
      const Serializable::Chunk* sav = Serializable::Chunk::parse(data, len);
      this->cart->get_mapper()->setBatterySave(sav);
    }

    delete data;
  }

  // Try to load savestate
  // kinda jank lol
  if (!this->config.cli.no_sav) {
    u8* data = nullptr;
    uint len = 0;
    ANESE_fs::load::load_file((this->current_rom_file + ".state").c_str(), data, len);

    u8* og_data = data;

    if (!data) fprintf(stderr, "[Savegame][Load] No savestate data found.\n");
    else {
      fprintf(stderr, "[Savegame][Load] Found savestate data.\n");
      for (const Serializable::Chunk*& savestate : this->savestate) {
        uint sav_len = ((uint*)data)[0];
        data += sizeof(uint);
        if (!sav_len) savestate = nullptr;
        else {
          savestate = Serializable::Chunk::parse(data, sav_len);
          data += sav_len;
        }
      }
    }

    delete og_data;
  }

  // Slap a cartridge in!
  this->nes.loadCartridge(this->cart->get_mapper());

  // Power-cycle the NES
  this->nes.power_cycle();

  return 0;
}

int SharedState::unload_rom() {
  if (!this->cart) return 0;
  fprintf(stderr, "[UnLoad] Unloading cart...\n");

  // Save Battey-Backed RAM
  if (this->cart != nullptr && !this->config.cli.no_sav) {
    const Serializable::Chunk* sav = this->cart->get_mapper()->getBatterySave();
    if (sav) {
      const u8* data;
      uint len;
      Serializable::Chunk::collate(data, len, sav);

      FILE* sav_file = fopen((this->current_rom_file + ".sav").c_str(), "wb");
      if (!sav_file) {
        fprintf(stderr, "[Savegame][Save] Failed to open save file!\n");
        return 1;
      }

      fwrite(data, 1, len, sav_file);
      fclose(sav_file);
      fprintf(stderr, "[Savegame][Save] Game saved to '%s.sav'!\n",
        this->current_rom_file.c_str());

      delete sav;
    }
  }

  // Save Savestates
  if (this->cart != nullptr && !this->config.cli.no_sav) {
    FILE* state_file = fopen((this->current_rom_file + ".state").c_str(), "wb");
    if (!state_file) {
      fprintf(stderr, "[Savegame][Save] Failed to open savestate file!\n");
      return 1;
    }

    // kinda jank lol
    for (const Serializable::Chunk* savestate : this->savestate) {
      const u8* data;
      uint len;
      Serializable::Chunk::collate(data, len, savestate);

      fwrite(&len, sizeof(uint), 1, state_file);
      if (data) fwrite(data, 1, len, state_file);
    }

    fclose(state_file);
    fprintf(stderr, "[Savegame][Save] Savestates saved to '%s.state'!\n",
      this->current_rom_file.c_str());
  }

  this->nes.removeCartridge();

  delete this->cart;

  return 0;
}
