#include "gui.h"

#include <cstdio>
#include <iostream>

#include <cfgpath.h>
#include <clara.hpp>
#include <SDL2_inprint.h>
#include <SimpleIni.h>

#include "common/util.h"
#include "common/serializable.h"

#include "nes/cartridge/cartridge.h"
#include "nes/joy/controllers/standard.h"
#include "nes/nes.h"

#include "fs/load.h"
#include "fs/util.h"

int SDL_GUI::init(int argc, char* argv[]) {
  // --------------------------- Argument Parsing --------------------------- //

  bool show_help = false;
  auto cli
    = clara::Help(show_help)
    | clara::Opt(this->args.log_cpu)
        ["--log-cpu"]
        ("Output CPU execution over STDOUT")
    | clara::Opt(this->args.no_sav)
        ["--no-sav"]
        ("Don't load/create sav files")
    | clara::Opt(this->args.ppu_timing_hack)
        ["--alt-nmi-timing"]
        ("Enable NMI timing fix \n"
         "(fixes some games, eg: Bad Dudes, Solomon's Key)")
    | clara::Opt(this->args.record_fm2_path, "path")
        ["--record-fm2"]
        ("Record a movie in the fm2 format")
    | clara::Opt(this->args.replay_fm2_path, "path")
        ["--replay-fm2"]
        ("Replay a movie in the fm2 format")
    | clara::Opt(this->args.config_file, "path")
        ["--config"]
        ("Use custom config file")
    | clara::Arg(this->args.rom, "rom")
        ("an iNES rom");

  auto result = cli.parse(clara::Args(argc, argv));
  if(!result) {
    std::cerr << "Error: " << result.errorMessage() << "\n";
    std::cerr << cli;
    exit(1);
  }

  if (show_help) {
    std::cout << cli;
    exit(1);
  }

  // -------------------------- Config File Parsing ------------------------- //

  // Get cross-platform config path (if no custom path specified)
  if (this->args.config_file == "") {
    char config_f_path [256];
    cfgpath::get_user_config_file(config_f_path, 256, "anese");
    this->args.config_file = config_f_path;
  }

  this->config.load(this->args.config_file.c_str());

  // TODO: put this somewhere else...
  strcpy(this->ui.menu.directory, this->config.roms_dir);

  // ------------------------------ NES Params ------------------------------ //

  if (this->args.log_cpu)         { this->emu.params.log_cpu         = true; }
  if (this->args.ppu_timing_hack) { this->emu.params.ppu_timing_hack = true; }
  this->emu.nes.updated_params();

  // ------------------------------ Init SDL2 ------------------------------- //

  fprintf(stderr, "[SDL2] Initializing SDL2 GUI\n");

  SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK);

  this->sdl.window = SDL_CreateWindow(
    "anese",
    SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
    RES_X * this->config.window_scale, RES_Y * this->config.window_scale,
    SDL_WINDOW_RESIZABLE
  );

  this->sdl.renderer = SDL_CreateRenderer(
    this->sdl.window,
    -1,
    SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
  );

  // Letterbox the screen in the window
  SDL_RenderSetLogicalSize(this->sdl.renderer,
    RES_X * SCREEN_SCALE, RES_Y * SCREEN_SCALE);
  // Allow opacity when drawing menu
  SDL_SetRenderDrawBlendMode(this->sdl.renderer, SDL_BLENDMODE_BLEND);

  /* Open the first available controller. */
  for (int i = 0; i < SDL_NumJoysticks(); i++) {
    if (SDL_IsGameController(i)) {
      this->sdl.controller = SDL_GameControllerOpen(i);
    }
  }

  // SDL_AudioSpec as, have;
  // as.freq = SDL_GUI::SAMPLE_RATE;
  // as.format = AUDIO_F32SYS;
  // as.channels = 1;
  // as.samples = 4096;
  // as.callback = nullptr; // use SDL_QueueAudio
  // this->sdl.nes_audiodev = SDL_OpenAudioDevice(NULL, 0, &as, &have, 0);
  // SDL_PauseAudioDevice(this->sdl.nes_audiodev, 0);

  // Setup SDL2_inprint font
  SDL2_inprint::inrenderer(this->sdl.renderer);
  SDL2_inprint::prepare_inline_font();

  /*----------  Init GUI subsystems  ----------*/
  this->emu.init();
  this->ui.init();

  // ---------------------------- Movie Support ----------------------------- //

  if (this->args.replay_fm2_path != "") {
    bool did_load = this->emu.fm2_replay.init(this->args.replay_fm2_path.c_str());
    if (!did_load)
      fprintf(stderr, "[Replay][fm2] Movie loading failed!\n");
    fprintf(stderr, "[Replay][fm2] Movie successfully loaded!\n");
  }

  if (this->args.record_fm2_path != "") {
    bool did_load = this->emu.fm2_record.init(this->args.record_fm2_path.c_str());
    if (!did_load)
      fprintf(stderr, "[Record][fm2] Failed to setup Movie recording!\n");
    fprintf(stderr, "[Record][fm2] Movie recording is setup!\n");
  }

  // -------------------------- NES Initialization -------------------------- //

  // pass controllers to this->fm2_record
  this->emu.fm2_record.set_joy(0, FM2_Controller::SI_GAMEPAD, &this->emu.joy_1);
  this->emu.fm2_record.set_joy(1, FM2_Controller::SI_GAMEPAD, &this->emu.joy_2);

  // Check if there is fm2 to replay
  if (this->emu.fm2_replay.is_enabled()) {
    // plug in fm2 controllers
    this->emu.nes.attach_joy(0, this->emu.fm2_replay.get_joy(0));
    this->emu.nes.attach_joy(1, this->emu.fm2_replay.get_joy(1));
  } else {
    // plug in physical nes controllers
    this->emu.nes.attach_joy(0, &this->emu.joy_1);
    this->emu.nes.attach_joy(1, &this->emu.zap_2);
  }

  // Load ROM if one has been passed as param
  if (this->args.rom != "") {
    this->ui.in_menu = false;
    int error = this->load_rom(this->args.rom.c_str());
    if (error) return error;
  }

  return 0;
}

SDL_GUI::~SDL_GUI() {
  fprintf(stderr, "[SDL2] Stopping SDL2 GUI\n");

  // Cleanup ROM (unloading also creates savs)
  this->unload_rom(this->emu.cart);
  delete this->emu.cart;

  // Update config
  // TODO: put this somewhere else...
  char new_roms_dir [256];
  ANESE_fs::util::get_abs_path(this->ui.menu.directory, new_roms_dir, 256);
  strcpy(this->config.roms_dir, new_roms_dir);

  this->config.save(this->args.config_file.c_str());

  // SDL Cleanup
  // SDL_CloseAudioDevice(this->sdl.nes_audiodev);
  SDL_GameControllerClose(this->sdl.controller);
  SDL_DestroyRenderer(this->sdl.renderer);
  SDL_DestroyWindow(this->sdl.window);
  SDL_Quit();

  SDL2_inprint::kill_inline_font();

  printf("\nANESE closed successfully\n");
}

int SDL_GUI::load_rom(const char* rompath) {
  delete this->emu.cart;
  for (uint i = 0; i < 4; i++) {
    delete this->emu.savestate[i];
    this->emu.savestate[i] = nullptr;
  }

  fprintf(stderr, "[Load] Loading '%s'\n", rompath);
  Cartridge* cart = new Cartridge (ANESE_fs::load::load_rom_file(rompath));

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
    this->emu.cart = cart;
    break;
  }

  // Try to load battery-backed save
  const Serializable::Chunk* sav = nullptr;

  if (!this->args.no_sav) {
    u8* data = nullptr;
    uint len = 0;
    ANESE_fs::load::load_file((std::string(rompath) + ".sav").c_str(), data, len);
    if (data) {
      fprintf(stderr, "[Savegame][Load] Found save data.\n");
      sav = Serializable::Chunk::parse(data, len);
      this->emu.cart->get_mapper()->setBatterySave(sav);
    } else {
      fprintf(stderr, "[Savegame][Load] No save data found.\n");
    }
    delete data;
  }

  // Slap a cartridge in!
  this->emu.nes.loadCartridge(this->emu.cart->get_mapper());

  // Power-cycle the NES
  this->emu.nes.power_cycle();

  return 0;
}

int SDL_GUI::unload_rom(Cartridge* cart) {
  if (!cart) return 0;

  fprintf(stderr, "[UnLoad] Unloading cart...\n");
  // Save Battey-Backed RAM
  if (cart != nullptr && !this->args.no_sav) {
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

  this->emu.nes.removeCartridge();

  return 0;
}

void SDL_GUI::input_global(const SDL_Event& event) {
  if (
    (event.type == SDL_QUIT) ||
    (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE)
  ) this->sdl_running = false;

  if (event.type == SDL_KEYDOWN) {
    switch (event.key.keysym.sym) {
      case SDLK_ESCAPE:
        this->ui.in_menu = !this->ui.in_menu; break;
    }
  }

  if (event.type == SDL_CONTROLLERBUTTONDOWN ||
      event.type == SDL_CONTROLLERBUTTONUP) {
    switch (event.cbutton.button) {
    case SDL_CONTROLLER_BUTTON_LEFTSTICK:
      this->ui.in_menu = !this->ui.in_menu; break;
    }
  }
}

int SDL_GUI::run() {
  fprintf(stderr, "[SDL2] Running SDL2 GUI\n");

  double past_fups [20] = {60.0}; // more samples == less value jitter
  uint past_fups_i = 0;

  while (this->sdl_running) {
    typedef uint time_ms;
    time_ms frame_start_time = SDL_GetTicks();
    past_fups_i++;

    // Check for new events
    SDL_Event event;
    while (SDL_PollEvent(&event) != 0) {
      this->input_global(event);

      if (!this->ui.in_menu)
        this->emu.input(event);
      else
        this->ui.input(event);
    }

    // Update the NES when not in menu
    if (!this->ui.in_menu)
      this->emu.update();
    else
      this->ui.update();

    // Render something
    this->emu.output(); // keep showing NES in the background
    if (this->ui.in_menu) {
      this->ui.output();
    }

    // SHOW ME WHAT YOU GOT
    SDL_RenderPresent(this->sdl.renderer);

    // time how long all-that took
    time_ms frame_end_time = SDL_GetTicks();

    // ---- Count Framerate ---- //
    // Update fups for this frame
    past_fups[past_fups_i % 20] = 1000.0 / (frame_end_time - frame_start_time);

    // Get the average fups over the past 20 frames
    double avg_fps = 0;
    for(unsigned i = 0; i < 20; i++)
      avg_fps += past_fups[i];
    avg_fps /= 20;

    // Present fups though the title of the main window
    char window_title [64];
    sprintf(window_title, "anese - %u fups - %u%% speed",
      uint(avg_fps), this->emu.params.speed);
    SDL_SetWindowTitle(this->sdl.window, window_title);
  }

  return 0;
}
