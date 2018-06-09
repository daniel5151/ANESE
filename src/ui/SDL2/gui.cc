#include "gui.h"

#include <iostream>
#include <cstdio>

#include <args.hxx>
#include <tinyfiledialogs.h>

#include "common/util.h"
#include "common/serializable.h"
#include "common/debug.h"

#include "nes/cartridge/cartridge.h"
#include "nes/joy/controllers/standard.h"
#include "nes/nes.h"

#include "fs/load.h"

constexpr uint RES_X = 256;
constexpr uint RES_Y = 240;

SDL_GUI::SDL_GUI(int argc, char* argv[]) {
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
    init_error_val = 1;
    return;
  } catch (args::ParseError& e) {
    std::cerr << e.what() << std::endl;
    std::cerr << parser;
    init_error_val = 1;
    return;
  } catch (args::ValidationError& e) {
    std::cerr << e.what() << std::endl;
    std::cerr << parser;
    init_error_val = 1;
    return;
  }

  this->args.log_cpu = log_cpu;
  this->args.reset_sav = reset_sav;
  this->args.ppu_timing_hack = ppu_timing_hack;
  this->args.record_fm2_path = args::get(record_fm2_path);
  this->args.replay_fm2_path = args::get(replay_fm2_path);
  this->args.rom = args::get(rom);

  // ----------------------------- Get ROM Path ----------------------------- //
  // Until I implement proper file-browsing in the SDL GUI, i'm using the nifty
  // `tinyfiledialogs` library to open a thin file-select dialog in the case
  // that no ROM file was provided

  if (args.rom == "") {
    const char* rom_formats[] = { "*.nes", "*.zip" };
    const char* file = tinyfd_openFileDialog(
      "Select ROM",
      nullptr,
      2, rom_formats,
      "NES roms",
      0
    );

    if (!file) {
      printf("[File Select] Canceled File Select. Closing...\n");
      init_error_val = 1;
      return;
    }

    this->args.rom = std::string(file);
  }

  // ------------------------------ Init SDL2 ------------------------------- //

  int window_w = RES_X * 2;
  int window_h = RES_Y * 2;

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

  this->sdl.sound_queue.init(44100);

  // ---------------------------- Debug Switches ---------------------------- //

  if (this->args.log_cpu)         { DEBUG_VARS::Get()->print_nestest = true; }
  if (this->args.ppu_timing_hack) { DEBUG_VARS::Get()->fogleman_hack = true; }

  // ---------------------------- Movie Support ----------------------------- //

  if (this->args.replay_fm2_path != "") {
    bool did_load = this->fm2_replay.init(this->args.replay_fm2_path.c_str());
    if (!did_load)
      fprintf(stderr, "[Replay][fm2] Movie loading failed!\n");
    fprintf(stderr, "[Replay][fm2] Movie successfully loaded!\n");
  }

  if (this->args.record_fm2_path != "") {
    bool did_load = this->fm2_record.init(this->args.record_fm2_path.c_str());
    if (!did_load)
      fprintf(stderr, "[Record][fm2] Failed to setup Movie recording!\n");
    fprintf(stderr, "[Record][fm2] Movie recording is setup!\n");
  }
}

int SDL_GUI::set_up_cartridge(const char* rompath) {
  delete this->cart;

  fprintf(stderr, "[Load] Loading '%s'\n", rompath);
  this->cart = new Cartridge (load_rom_file(rompath));

  switch (this->cart->status()) {
  case Cartridge::Status::BAD_DATA:
    fprintf(stderr, "[Cart] ROM file could not be parsed!\n");
    return 1;
  case Cartridge::Status::BAD_MAPPER:
    fprintf(stderr, "[Cart] Mapper %u has not been implemented yet!\n",
      this->cart->get_rom_file()->meta.mapper);
    return 1;
  case Cartridge::Status::NO_ERROR:
    fprintf(stderr, "[Cart] ROM file loaded successfully!\n");
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
    } else {
      fprintf(stderr, "[Savegame][Load] No save data found.\n");
    }
    delete data;
  }

  // Inject Battery-Backed RAM Save
  this->cart->get_mapper()->setBatterySave(sav);

  return 0;
}

void SDL_GUI::update_joypads(const SDL_Event& event) {
  // Update from Controllers
  if (event.type == SDL_CONTROLLERBUTTONDOWN || event.type == SDL_CONTROLLERBUTTONUP) {
    bool new_state = (event.type == SDL_CONTROLLERBUTTONDOWN) ? true : false;
    using namespace JOY_Standard_Button;

    // Player 1
    switch (event.cbutton.button) {
    case SDL_CONTROLLER_BUTTON_A:          this->joy_1.set_button(A,      new_state); break;
    case SDL_CONTROLLER_BUTTON_X:          this->joy_1.set_button(B,      new_state); break;
    case SDL_CONTROLLER_BUTTON_START:      this->joy_1.set_button(Start,  new_state); break;
    case SDL_CONTROLLER_BUTTON_BACK:       this->joy_1.set_button(Select, new_state); break;
    case SDL_CONTROLLER_BUTTON_DPAD_UP:    this->joy_1.set_button(Up,     new_state); break;
    case SDL_CONTROLLER_BUTTON_DPAD_DOWN:  this->joy_1.set_button(Down,   new_state); break;
    case SDL_CONTROLLER_BUTTON_DPAD_LEFT:  this->joy_1.set_button(Left,   new_state); break;
    case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: this->joy_1.set_button(Right,  new_state); break;
    }

    // Player 2
    switch (event.cbutton.button) {
    case SDL_CONTROLLER_BUTTON_A:          this->joy_2.set_button(A,      new_state); break;
    case SDL_CONTROLLER_BUTTON_X:          this->joy_2.set_button(B,      new_state); break;
    case SDL_CONTROLLER_BUTTON_START:      this->joy_2.set_button(Start,  new_state); break;
    case SDL_CONTROLLER_BUTTON_BACK:       this->joy_2.set_button(Select, new_state); break;
    case SDL_CONTROLLER_BUTTON_DPAD_UP:    this->joy_2.set_button(Up,     new_state); break;
    case SDL_CONTROLLER_BUTTON_DPAD_DOWN:  this->joy_2.set_button(Down,   new_state); break;
    case SDL_CONTROLLER_BUTTON_DPAD_LEFT:  this->joy_2.set_button(Left,   new_state); break;
    case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: this->joy_2.set_button(Right,  new_state); break;
    }
  }

  // Update from Keyboard
  if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
    // ------ Keyboard controls ------ //
    bool new_state = (event.type == SDL_KEYDOWN) ? true : false;
    using namespace JOY_Standard_Button;

    // Player 1
    switch (event.key.keysym.sym) {
    case SDLK_z:      this->joy_1.set_button(A,      new_state); break;
    case SDLK_x:      this->joy_1.set_button(B,      new_state); break;
    case SDLK_RETURN: this->joy_1.set_button(Start,  new_state); break;
    case SDLK_RSHIFT: this->joy_1.set_button(Select, new_state); break;
    case SDLK_UP:     this->joy_1.set_button(Up,     new_state); break;
    case SDLK_DOWN:   this->joy_1.set_button(Down,   new_state); break;
    case SDLK_LEFT:   this->joy_1.set_button(Left,   new_state); break;
    case SDLK_RIGHT:  this->joy_1.set_button(Right,  new_state); break;
    }

    // Player 2
    // For now: mapped to player 1
    switch (event.key.keysym.sym) {
    case SDLK_z:      this->joy_2.set_button(A,      new_state); break;
    case SDLK_x:      this->joy_2.set_button(B,      new_state); break;
    case SDLK_RETURN: this->joy_2.set_button(Start,  new_state); break;
    case SDLK_RSHIFT: this->joy_2.set_button(Select, new_state); break;
    case SDLK_UP:     this->joy_2.set_button(Up,     new_state); break;
    case SDLK_DOWN:   this->joy_2.set_button(Down,   new_state); break;
    case SDLK_LEFT:   this->joy_2.set_button(Left,   new_state); break;
    case SDLK_RIGHT:  this->joy_2.set_button(Right,  new_state); break;
    }
  }
}

void SDL_GUI::check_misc_actions(const SDL_Event& event) {
  if (
    (event.type == SDL_QUIT) ||
    (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE)
  ) this->running = false;

  // Handle Key-events
  if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
    bool mod_shift = event.key.keysym.mod & KMOD_SHIFT;
    // Use CMD on macOS, and CTRL on windows / linux
    bool mod_ctrl = strcmp(SDL_GetPlatform(), "Mac OS X") == 0
      ? event.key.keysym.mod & (KMOD_LGUI | KMOD_RGUI)
      : mod_ctrl = event.key.keysym.mod & (KMOD_LCTRL | KMOD_RCTRL);

    // Regular 'ol keys
    switch (event.key.keysym.sym) {
      case SDLK_ESCAPE:
        // Quit
        this->running = false;
        break;
      case SDLK_SPACE:
        // Fast-Forward
        this->speedup = (event.type == SDL_KEYDOWN) ? 200 : 100;
        this->speed_counter = 0;
        this->nes.set_speed(speedup);
        break;
    }

    // Meta Modified keys
    if (event.type == SDL_KEYDOWN && mod_ctrl) {
      #define SAVESTATE(i) do {                     \
        if (mod_shift) {                            \
          delete this->savestate[i];                \
          this->savestate[i] = nes.serialize();     \
        } else this->nes.deserialize(savestate[i]); \
      } while(0);

      switch (event.key.keysym.sym) {
      case SDLK_1: SAVESTATE(0); break; // Savestate Slot 1
      case SDLK_2: SAVESTATE(1); break; // Savestate Slot 2
      case SDLK_3: SAVESTATE(2); break; // Savestate Slot 3
      case SDLK_4: SAVESTATE(3); break; // Savestate Slot 4
      case SDLK_r: this->nes.reset();       break; // Reset
      case SDLK_p: this->nes.power_cycle(); break; // Power-Cycle
      case SDLK_EQUALS:
        // Speed up
        this->nes.set_speed(speedup += 25);
        this->speed_counter = 0;
        break;
      case SDLK_MINUS:
        // Speed down
        if (speedup - 25 != 0) {
          this->nes.set_speed(speedup -= 25);
          this->speed_counter = 0;
        }
        break;
      case SDLK_c: {
        // Toggle CPU trace
        bool log = DEBUG_VARS::Get()->print_nestest ^= 1;
        fprintf(stderr, "NESTEST CPU logging: %s\n", log ? "ON" : "OFF");
      } break;
      default: break;
      }
    }
  }
}

int SDL_GUI::run() {
  fprintf(stderr, "[SDL2] Running SDL2 GUI\n");

  // -------------------------- Construct Cartridge ------------------------- //

  if (this->args.rom != "") {
    int error = this->set_up_cartridge(this->args.rom.c_str());
    if (error) return error;
  }

  // -------------------------- NES Initialization -------------------------- //

  // pass controllers to this->fm2_record if needed
  if (this->fm2_record.is_enabled()) {
    this->fm2_record.set_joy(0, FM2_Controller::SI_GAMEPAD, &joy_1);
    this->fm2_record.set_joy(1, FM2_Controller::SI_GAMEPAD, &joy_2);
  }

  // Check if there is fm2 to replay
  if (this->fm2_replay.is_enabled()) {
    // plug in fm2 controllers
    this->nes.attach_joy(0, this->fm2_replay.get_joy(0));
    this->nes.attach_joy(1, this->fm2_replay.get_joy(1));
  } else {
    // plug in physical nes controllers
    this->nes.attach_joy(0, &joy_1);
    this->nes.attach_joy(1, &joy_2);
  }

  // Slap a cartridge in!
  this->nes.loadCartridge(this->cart->get_mapper());

  // Power up the NES
  this->nes.power_cycle();

  // ------------------------------ Main Loop! ------------------------------ //
  double past_fups [20] = {60.0}; // more samples == less value jitter
  uint past_fups_i = 0;

  while (this->running) {
    typedef uint time_ms;
    time_ms frame_start_time = SDL_GetTicks();
    past_fups_i++;

    // Check for new events
    SDL_Event event;
    while (SDL_PollEvent(&event) != 0) {
      this->check_misc_actions(event);
      this->update_joypads(event);
    }

    // Calculate the number of frames to render
    // (speedup values that are not clean multiples of 100 imply that some the
    //  number of nes frames / real frame will vary)
    uint numframes = 0;
    speed_counter += speedup;
    while (speed_counter > 0) {
      speed_counter -= 100;
      numframes++;
    }

    // Run ANESE core for some number of frames
    for (uint i = 0; i < numframes; i++) {
      if (fm2_record.is_enabled()) fm2_record.step_frame(); // log frame to fm2
      if (fm2_replay.is_enabled()) fm2_replay.step_frame(); // set input from fm2

      // run the NES for a frame
      nes.step_frame();
    }

    if (nes.isRunning() == false) {
      // this->running = true;
    }

    // output audio!
    short* samples = nullptr;
    uint   count = 0;
    nes.getAudiobuff(samples, count);
    if (count) this->sdl.sound_queue.write(samples, count);

    // output video!
    const u8* framebuffer = nes.getFramebuff();
    SDL_UpdateTexture(this->sdl.nes_texture, nullptr, framebuffer, RES_X * 4);
    SDL_RenderCopy(this->sdl.renderer, this->sdl.nes_texture, nullptr, &this->sdl.nes_screen);
    SDL_RenderPresent(this->sdl.renderer);

    // time how long all-that took :)
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
      uint(avg_fps), this->speedup);
    SDL_SetWindowTitle(this->sdl.window, window_title);
  }

  return 0;
}


SDL_GUI::~SDL_GUI() {
  fprintf(stderr, "[SDL2] Stopping SDL2 GUI\n");

  // Save Battey-Backed RAM
  const Serializable::Chunk* sav = this->cart->get_mapper()->getBatterySave();
  if (sav) {
    const u8* data;
    uint len;
    Serializable::Chunk::collate(data, len, sav);

    FILE* sav_file = fopen((args.rom + ".sav").c_str(), "wb");
    if (!sav_file)
      fprintf(stderr, "[Savegame][Save] Failed to open save file!\n");

    fwrite(data, 1, len, sav_file);
    fclose(sav_file);
    fprintf(stderr, "[Savegame][Save] Game successfully saved!\n");

    delete sav;
  }

  // NES Cleanup
  delete this->cart;

  // SDL Cleanup
  SDL_GameControllerClose(this->sdl.controller);
  SDL_DestroyTexture(this->sdl.nes_texture);
  SDL_DestroyRenderer(this->sdl.renderer);
  SDL_DestroyWindow(this->sdl.window);
  SDL_Quit();

  printf("\nANESE closed successfully\n");
}
