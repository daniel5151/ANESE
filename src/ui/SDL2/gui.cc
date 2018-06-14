#include "gui.h"

#include <cstdio>
#include <iostream>

#include <args.hxx>

namespace SDL2_inprint {
extern "C" {
#include <SDL2_inprint.h>
}
}

#include "common/util.h"
#include "common/serializable.h"
#include "common/debug.h"

#include "nes/cartridge/cartridge.h"
#include "nes/joy/controllers/standard.h"
#include "nes/nes.h"

#include "fs/load.h"

int SDL_GUI::init(int argc, char* argv[]) {
  fprintf(stderr, "[SDL2] Starting SDL2 GUI\n");
  // --------------------------- Argument Parsing --------------------------- //

  // use the `args` library to parse arguments
  args::ArgumentParser parser("ANESE - A Nintendo Entertainment System Emulator", "");
  args::HelpFlag help(parser, "help", "Display this help menu", {'h', "help"});

  // Debug
  args::Flag log_cpu(parser, "log-cpu",
    "Output CPU execution over STDOUT",
    {'c', "log-cpu"});

  args::Flag reset_sav(parser, "reset-sav",
    "Overwrite any existing save files",
    {"reset-sav"});

  // Hacks
  args::Flag ppu_timing_hack(parser, "alt-nmi-timing",
    "Enable NMI timing fix \n"
    "(needed to boot some games, eg: Bad Dudes, Solomon's Key)",
    {"alt-nmi-timing"});

  // Movies
  args::ValueFlag<std::string> record_fm2_path(parser, "path",
    "Record a movie in the fm2 format",
    {"record-fm2"});
  args::ValueFlag<std::string> replay_fm2_path(parser, "path",
    "Replay a movie in the fm2 format",
    {"replay-fm2"});

  // Essential
  args::Positional<std::string> rom(parser, "rom",
    "Path to a iNES rom");

  try {
    parser.ParseCLI(argc, argv);
  } catch (args::Help) {
    std::cout << parser;
    return 0;
  } catch (args::ParseError& e) {
    std::cerr << e.what() << std::endl;
    std::cerr << parser;
    return 1;
  } catch (args::ValidationError& e) {
    std::cerr << e.what() << std::endl;
    std::cerr << parser;
    return 1;
  }

  this->args.log_cpu = log_cpu;
  this->args.reset_sav = reset_sav;
  this->args.ppu_timing_hack = ppu_timing_hack;
  this->args.record_fm2_path = args::get(record_fm2_path);
  this->args.replay_fm2_path = args::get(replay_fm2_path);
  this->args.rom = args::get(rom);

  // ------------------------------ Init SDL2 ------------------------------- //

  const int window_w = RES_X * SCREEN_SCALE;
  const int window_h = RES_Y * SCREEN_SCALE;

  SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK);

  this->sdl.window = SDL_CreateWindow(
    "anese",
    SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
    window_w, window_h,
    SDL_WINDOW_RESIZABLE
  );

  this->sdl.renderer = SDL_CreateRenderer(
    this->sdl.window,
    -1,
    SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
  );

  // nes screen texture
  this->sdl.nes_texture = SDL_CreateTexture(
    this->sdl.renderer,
    SDL_PIXELFORMAT_ARGB8888,
    SDL_TEXTUREACCESS_STREAMING,
    RES_X, RES_Y
  );

  // The rectangle that the nes screen texture is slapped onto
  this->sdl.nes_screen.h = window_h;
  this->sdl.nes_screen.w = window_w;
  this->sdl.nes_screen.x = 0;
  this->sdl.nes_screen.y = 0;

  // Letterbox the screen in the window
  SDL_RenderSetLogicalSize(this->sdl.renderer, window_w, window_h);
  // Allow opacity when drawing menu
  SDL_SetRenderDrawBlendMode(this->sdl.renderer, SDL_BLENDMODE_BLEND);

  /* Open the first available controller. */
  for (int i = 0; i < SDL_NumJoysticks(); i++) {
    if (SDL_IsGameController(i)) {
      this->sdl.controller = SDL_GameControllerOpen(i);
      if (this->sdl.controller) break;
      else {
        fprintf(stderr, "[SDL] Could not open game controller %i: %s\n",
          i, SDL_GetError());
      }
    }
  }

  this->sdl.nes_sound_queue.init(44100);

  // Setup SDL2_inprint font
  SDL2_inprint::inrenderer(this->sdl.renderer);
  SDL2_inprint::prepare_inline_font();

  // ---------------------------- Debug Switches ---------------------------- //

  if (this->args.log_cpu)         { DEBUG_VARS::Get()->print_nestest = true; }
  if (this->args.ppu_timing_hack) { DEBUG_VARS::Get()->fogleman_hack = true; }

  // ---------------------------- Movie Support ----------------------------- //

  if (this->args.replay_fm2_path != "") {
    bool did_load = this->nes.fm2_replay.init(this->args.replay_fm2_path.c_str());
    if (!did_load)
      fprintf(stderr, "[Replay][fm2] Movie loading failed!\n");
    fprintf(stderr, "[Replay][fm2] Movie successfully loaded!\n");
  }

  if (this->args.record_fm2_path != "") {
    bool did_load = this->nes.fm2_record.init(this->args.record_fm2_path.c_str());
    if (!did_load)
      fprintf(stderr, "[Record][fm2] Failed to setup Movie recording!\n");
    fprintf(stderr, "[Record][fm2] Movie recording is setup!\n");
  }

  // -------------------------- NES Initialization -------------------------- //

  // pass controllers to this->fm2_record
  this->nes.fm2_record.set_joy(0, FM2_Controller::SI_GAMEPAD, &this->nes.joy_1);
  this->nes.fm2_record.set_joy(1, FM2_Controller::SI_GAMEPAD, &this->nes.joy_2);

  // Check if there is fm2 to replay
  if (this->nes.fm2_replay.is_enabled()) {
    // plug in fm2 controllers
    this->nes.console.attach_joy(0, this->nes.fm2_replay.get_joy(0));
    this->nes.console.attach_joy(1, this->nes.fm2_replay.get_joy(1));
  } else {
    // plug in physical nes controllers
    this->nes.console.attach_joy(0, &this->nes.joy_1);
    this->nes.console.attach_joy(1, &this->nes.zap_2);
  }

  // Load ROM if one has been passed as param
  if (this->args.rom != "") {
    this->ui.in_menu = false;
    int error = this->load_rom(this->args.rom.c_str());
    if (error) return error;
  }

  return 0;
}

int SDL_GUI::load_rom(const char* rompath) {
  delete this->nes.cart;

  fprintf(stderr, "[Load] Loading '%s'\n", rompath);
  Cartridge* cart = new Cartridge (load_rom_file(rompath));

  switch (cart->status()) {
  case Cartridge::Status::BAD_DATA:
    fprintf(stderr, "[Cart] ROM file could not be parsed!\n");
    delete cart;
    return 1;
  case Cartridge::Status::BAD_MAPPER:
    fprintf(stderr, "[Cart] Mapper %u has not been implemented yet!\n",
      cart->get_rom_file()->meta.mapper);
    delete cart;
    return 1;
  case Cartridge::Status::NO_ERROR:
    fprintf(stderr, "[Cart] ROM file loaded successfully!\n");
    strcpy(this->ui.current_rom_file, rompath);
    this->nes.cart = cart;
    break;
  }

  // Try to load battery-backed save
  const Serializable::Chunk* sav = nullptr;

  if (!this->args.reset_sav) {
    u8* data = nullptr;
    uint len = 0;
    load_file((std::string(rompath) + ".sav").c_str(), data, len);
    if (data) {
      fprintf(stderr, "[Savegame][Load] Found save data.\n");
      sav = Serializable::Chunk::parse(data, len);
      this->nes.cart->get_mapper()->setBatterySave(sav);
    } else {
      fprintf(stderr, "[Savegame][Load] No save data found.\n");
    }
    delete data;
  }

  // Slap a cartridge in!
  this->nes.console.loadCartridge(this->nes.cart->get_mapper());

  // Power-cycle the NES
  this->nes.console.power_cycle();

  return 0;
}

int SDL_GUI::unload_rom(Cartridge* cart) {
  if (!cart) return 0;

  fprintf(stderr, "[UnLoad] Unloading cart...\n");
  // Save Battey-Backed RAM
  if (cart != nullptr) {
    const Serializable::Chunk* sav = cart->get_mapper()->getBatterySave();
    if (sav) {
      const u8* data;
      uint len;
      Serializable::Chunk::collate(data, len, sav);

      const char* sav_file_name = (std::string(this->ui.current_rom_file) + ".sav").c_str();

      FILE* sav_file = fopen(sav_file_name, "wb");
      if (!sav_file) {
        fprintf(stderr, "[Savegame][Save] Failed to open save file!\n");
        return 1;
      }

      fwrite(data, 1, len, sav_file);
      fclose(sav_file);
      fprintf(stderr, "[Savegame][Save] Game successfully saved to '%s'!\n",
        sav_file_name);

      delete sav;
    }
  }

  this->nes.console.removeCartridge();

  return 0;
}

SDL_GUI::~SDL_GUI() {
  fprintf(stderr, "[SDL2] Stopping SDL2 GUI\n");

  this->unload_rom(this->nes.cart);
  delete this->nes.cart;

  // SDL Cleanup
  SDL_GameControllerClose(this->sdl.controller);
  SDL_DestroyTexture(this->sdl.nes_texture);
  SDL_DestroyRenderer(this->sdl.renderer);
  SDL_DestroyWindow(this->sdl.window);
  SDL_Quit();

  SDL2_inprint::kill_inline_font();

  printf("\nANESE closed successfully\n");
}
