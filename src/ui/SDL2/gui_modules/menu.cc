#include "menu.h"

#include <algorithm>

#define CUTE_FILES_IMPLEMENTATION
#include <cute_files.h>
#include <SDL2_inprint.h>

#include "../fs/load.h"
#include "../fs/util.h"

MenuModule::MenuModule(SharedState& gui, EmuModule& emu)
: GUIModule(gui)
, emu(emu)
{
  // Update from config
  strcpy(this->nav.directory, this->gui.config.roms_dir);

  // Setup SDL2_inprint font
  SDL2_inprint::inrenderer(this->emu.sdl.renderer); // use NES's renderer
  SDL2_inprint::prepare_inline_font();
}

MenuModule::~MenuModule() {
  // Update config
  ANESE_fs::util::get_abs_path(this->gui.config.roms_dir, this->nav.directory, 260);

  SDL2_inprint::kill_inline_font();

}

void MenuModule::input(const SDL_Event& event) {
  // First, check if actions should even be recorded...
  if (event.type == SDL_KEYDOWN) {
    switch (event.key.keysym.sym) {
    case SDLK_ESCAPE: this->in_menu = !this->in_menu; break;
    }
  }
  if (event.type == SDL_CONTROLLERBUTTONDOWN ||
      event.type == SDL_CONTROLLERBUTTONUP) {
    switch (event.cbutton.button) {
    case SDL_CONTROLLER_BUTTON_LEFTSTICK: this->in_menu = !this->in_menu; break;
    }
  }

  if (!this->in_menu) return;

  if (event.type == SDL_KEYDOWN) {
    switch (event.key.keysym.sym) {
    case SDLK_RETURN: this->hit.enter = true; break;
    case SDLK_DOWN:   this->hit.down  = true; break;
    case SDLK_UP:     this->hit.up    = true; break;
    case SDLK_LEFT:   this->hit.left  = true; break;
    case SDLK_RIGHT:  this->hit.right = true; break;
    }
    // support for basic "fast skipping" though file-names
    const char* key_pressed = SDL_GetKeyName(event.key.keysym.sym);
    bool is_valid_char = isalnum(key_pressed[0])
                      || key_pressed[0] == '.';
    if (is_valid_char && key_pressed[1] == '\0')
      this->hit.last_ascii = key_pressed[0];
    // Special case: space
    if (event.key.keysym.sym == SDLK_SPACE)
      this->hit.last_ascii = ' ';
  }

  if (event.type == SDL_CONTROLLERBUTTONDOWN ||
      event.type == SDL_CONTROLLERBUTTONUP) {
    switch (event.cbutton.button) {
    case SDL_CONTROLLER_BUTTON_A:         this->hit.enter = true; break;
    case SDL_CONTROLLER_BUTTON_DPAD_UP:   this->hit.up    = true; break;
    case SDL_CONTROLLER_BUTTON_DPAD_DOWN: this->hit.down  = true; break;
    case SDL_CONTROLLER_BUTTON_DPAD_LEFT: this->hit.left  = true; break;
    }
  }
}

void MenuModule::update() {
  std::vector<cf_file_t>& files = this->nav.files; // helpful alias

  // First, check if the user wants to navigate / load a rom
  if (this->hit.enter || this->hit.right) {
    this->hit.enter = false;
    this->hit.right = false;
    const cf_file_t& file = files[this->nav.selected_i];
    if (file.is_dir) {
      // Navigate into directory
      strcpy(this->nav.directory, file.path);
      this->nav.selected_i = 0;
      this->nav.should_update_dir = true;
    } else {
      // Load-up ROM (and close menu)
      fprintf(stderr, "[Menu] Selected '%s'\n", file.name);
      this->unload_rom();
      this->load_rom(file.path);
      this->in_menu = false;
    }
  }

  // Potentially update directory listing
  if (this->nav.should_update_dir) {
    this->nav.should_update_dir = false;
    files.clear();

    // Get file-listing
    cf_dir_t dir;
    cf_dir_open(&dir, this->nav.directory);
    bool skip_first = true;
    while (dir.has_next) {
      cf_file_t file;
      cf_read_file(&dir, &file);
      if (!skip_first)
        files.push_back(file);
      cf_dir_next(&dir);
      skip_first = false;
    }
    cf_dir_close(&dir);

    // Remove all file-types we don't care about
    files.erase(
      std::remove_if(files.begin(), files.end(), [](const cf_file_t& file) {
        return file.is_dir == false &&
          !(strcmp("nes", file.ext) == 0 || strcmp("zip", file.ext) == 0);
      }),
      files.end()
    );

    // Sort the directory by filename
    std::sort(files.begin(), files.end(),
      [](const cf_file_t& a, const cf_file_t& b){
        return strcmp(a.name, b.name) < 0;
      });
  }

  // Handle navigation...

  if (this->hit.up) {
    this->hit.up = false;
    this->nav.selected_i -= this->nav.selected_i ? 1 : 0;
  }

  if (this->hit.down) {
    this->hit.down = false;
    this->nav.selected_i += (this->nav.selected_i < (files.size() - 1));
  }

  if (this->hit.left) {
    this->hit.left = false;
    strcpy(this->nav.directory, files[0].path);
    this->nav.selected_i = 0;
  }

  if (this->nav.quicksel.timeout) { this->nav.quicksel.timeout--; }
  else {
    this->nav.quicksel.i = 0;
    memset(this->nav.quicksel.buf, '\0', 16);
  }

  if (this->hit.last_ascii) {
    // clear buffer in ~1s from last keypress
    this->nav.quicksel.timeout = 30;

    // buf[15] is always '\0'
    this->nav.quicksel.buf[this->nav.quicksel.i++ % 15] = ::tolower(this->hit.last_ascii);

    uint new_selection = std::distance(
      files.begin(),
      std::find_if(files.begin(), files.end(),
        [=](const cf_file_t& f){
          std::string fname = std::string(f.name).substr(0, this->nav.quicksel.i);
          std::transform(fname.begin(), fname.end(), fname.begin(), ::tolower);
          return strcmp(fname.c_str(), this->nav.quicksel.buf) == 0;
        })
    );
    if (new_selection < files.size())
      this->nav.selected_i = new_selection;

    this->hit.last_ascii = '\0';
  }
}

void MenuModule::output() {
  if (!this->in_menu) return;
  // menu uses the EmuModule's rendering context

  // Paint transparent bg
  this->bg.x = this->bg.y = 0;
  SDL_GetWindowSize(this->emu.sdl.window, &this->bg.w, &this->bg.h);
  SDL_SetRenderDrawColor(this->emu.sdl.renderer, 0, 0, 0, 200);
  SDL_RenderFillRect(this->emu.sdl.renderer, &this->bg);

  // Paint menu
  for (uint i = 0; i < this->nav.files.size(); i++) {
    const cf_file_t& file = this->nav.files[i];

    u32 color;
    if (this->nav.selected_i == i)         color = 0xff0000; // red   - selected
    else if (strcmp("nes", file.ext) == 0) color = 0x00ff00; // green - .nes
    else if (strcmp("zip", file.ext) == 0) color = 0x00ffff; // cyan  - .zip
    else                                   color = 0xffffff; // white - folder

    SDL2_inprint::incolor(color, /* unused */ 0);
    SDL2_inprint::inprint(this->emu.sdl.renderer, file.name,
      10, this->bg.h / 2 + (i - this->nav.selected_i) * 12
    );
  }
}

int MenuModule::load_rom(const char* rompath) {
  // cleanup previous cart
  delete this->gui.cart;
  for (uint i = 0; i < 4; i++) {
    delete this->emu.savestate[i];
    this->emu.savestate[i] = nullptr;
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
    strcpy(this->current_rom_file, rompath);
    this->gui.cart = cart;
    break;
  }

  // Try to load battery-backed save
  if (!this->gui.config.cli.no_sav) {
    u8* data = nullptr;
    uint len = 0;
    ANESE_fs::load::load_file((std::string(rompath) + ".sav").c_str(), data, len);

    if (!data) fprintf(stderr, "[Savegame][Load] No save data found.\n");
    else {
      fprintf(stderr, "[Savegame][Load] Found save data.\n");
      const Serializable::Chunk* sav = Serializable::Chunk::parse(data, len);
      this->gui.cart->get_mapper()->setBatterySave(sav);
    }

    delete data;
  }

  // Try to load savestate
  // kinda jank lol
  if (!this->gui.config.cli.no_sav) {
    u8* data = nullptr;
    uint len = 0;
    ANESE_fs::load::load_file((std::string(rompath) + ".state").c_str(), data, len);

    u8* og_data = data;

    if (!data) fprintf(stderr, "[Savegame][Load] No savestate data found.\n");
    else {
      fprintf(stderr, "[Savegame][Load] Found savestate data.\n");
      for (const Serializable::Chunk*& savestate : this->emu.savestate) {
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
  this->gui.nes.loadCartridge(this->gui.cart->get_mapper());

  // Power-cycle the NES
  this->gui.nes.power_cycle();

  return 0;
}

int MenuModule::unload_rom() {
  if (!this->gui.cart) return 0;
  fprintf(stderr, "[UnLoad] Unloading cart...\n");

  // Save Battey-Backed RAM
  if (this->gui.cart != nullptr && !this->gui.config.cli.no_sav) {
    const Serializable::Chunk* sav = this->gui.cart->get_mapper()->getBatterySave();
    if (sav) {
      const u8* data;
      uint len;
      Serializable::Chunk::collate(data, len, sav);

      char buf [256];
      sprintf(buf, "%s.sav", this->current_rom_file);

      FILE* sav_file = fopen(buf, "wb");
      if (!sav_file) {
        fprintf(stderr, "[Savegame][Save] Failed to open save file!\n");
        return 1;
      }

      fwrite(data, 1, len, sav_file);
      fclose(sav_file);
      fprintf(stderr, "[Savegame][Save] Game saved to '%s'!\n", buf);

      delete sav;
    }
  }

  // Save Savestates
  if (this->gui.cart != nullptr && !this->gui.config.cli.no_sav) {
    char buf [256];
    sprintf(buf, "%s.state", this->current_rom_file);
    FILE* state_file = fopen(buf, "wb");
    if (!state_file) {
      fprintf(stderr, "[Savegame][Save] Failed to open savestate file!\n");
      return 1;
    }

    // kinda jank lol
    for (const Serializable::Chunk* savestate : this->emu.savestate) {
      const u8* data;
      uint len;
      Serializable::Chunk::collate(data, len, savestate);

      fwrite(&len, sizeof(uint), 1, state_file);
      if (data) fwrite(data, 1, len, state_file);
    }

    fclose(state_file);
    fprintf(stderr, "[Savegame][Save] Savestates saved to '%s'!\n", buf);
  }

  this->gui.nes.removeCartridge();

  return 0;
}
