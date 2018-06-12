#include "gui.h"

#include <algorithm>
#include <cstdio>
#include <iostream>

#include <args.hxx>

#define CUTE_FILES_IMPLEMENTATION
#include <cute_files.h>

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

constexpr uint RES_X = 256;
constexpr uint RES_Y = 240;

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

  const int window_w = RES_X * 2;
  const int window_h = RES_Y * 2;

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

  // -------------------------- NES Initialization -------------------------- //

  // pass controllers to this->fm2_record
  this->fm2_record.set_joy(0, FM2_Controller::SI_GAMEPAD, &joy_1);
  this->fm2_record.set_joy(1, FM2_Controller::SI_GAMEPAD, &joy_2);

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

  // Load ROM if one has been passed as param
  if (this->args.rom != "") {
    this->in_menu = false;
    int error = this->load_rom(this->args.rom.c_str());
    if (error) return error;
  }

  return 0;
}

int SDL_GUI::load_rom(const char* rompath) {
  delete this->cart;

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
    strcpy(this->current_rom_file, rompath);
    this->cart = cart;
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
      this->cart->get_mapper()->setBatterySave(sav);
    } else {
      fprintf(stderr, "[Savegame][Load] No save data found.\n");
    }
    delete data;
  }

  // Slap a cartridge in!
  this->nes.loadCartridge(this->cart->get_mapper());

  // Power-cycle the NES
  this->nes.power_cycle();

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

      const char* sav_file_name = (std::string(this->current_rom_file) + ".sav").c_str();

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

  this->nes.removeCartridge();

  return 0;
}

void SDL_GUI::update_input_nes_joypads(const SDL_Event& event) {
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
    // switch (event.cbutton.button) {
    // case SDL_CONTROLLER_BUTTON_A:          this->joy_2.set_button(A,      new_state); break;
    // case SDL_CONTROLLER_BUTTON_X:          this->joy_2.set_button(B,      new_state); break;
    // case SDL_CONTROLLER_BUTTON_START:      this->joy_2.set_button(Start,  new_state); break;
    // case SDL_CONTROLLER_BUTTON_BACK:       this->joy_2.set_button(Select, new_state); break;
    // case SDL_CONTROLLER_BUTTON_DPAD_UP:    this->joy_2.set_button(Up,     new_state); break;
    // case SDL_CONTROLLER_BUTTON_DPAD_DOWN:  this->joy_2.set_button(Down,   new_state); break;
    // case SDL_CONTROLLER_BUTTON_DPAD_LEFT:  this->joy_2.set_button(Left,   new_state); break;
    // case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: this->joy_2.set_button(Right,  new_state); break;
    // }
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
    // switch (event.key.keysym.sym) {
    // case SDLK_z:      this->joy_2.set_button(A,      new_state); break;
    // case SDLK_x:      this->joy_2.set_button(B,      new_state); break;
    // case SDLK_RETURN: this->joy_2.set_button(Start,  new_state); break;
    // case SDLK_RSHIFT: this->joy_2.set_button(Select, new_state); break;
    // case SDLK_UP:     this->joy_2.set_button(Up,     new_state); break;
    // case SDLK_DOWN:   this->joy_2.set_button(Down,   new_state); break;
    // case SDLK_LEFT:   this->joy_2.set_button(Left,   new_state); break;
    // case SDLK_RIGHT:  this->joy_2.set_button(Right,  new_state); break;
    // }
  }
}

void SDL_GUI::update_input_global(const SDL_Event& event) {
  if (
    (event.type == SDL_QUIT) ||
    (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE)
  ) this->sdl_running = false;

  if (event.type == SDL_KEYDOWN) {
    switch (event.key.keysym.sym) {
      case SDLK_ESCAPE: this->in_menu = !this->in_menu; break;
    }
  }

  if (event.type == SDL_CONTROLLERBUTTONDOWN || event.type == SDL_CONTROLLERBUTTONUP) {
    switch (event.cbutton.button) {
    case SDL_CONTROLLER_BUTTON_LEFTSTICK: this->in_menu = !this->in_menu; break;
    }
  }
}

void SDL_GUI::update_input_nes_misc(const SDL_Event& event) {
  // Handle Key-events
  if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
    bool mod_shift = event.key.keysym.mod & KMOD_SHIFT;
    // Use CMD on macOS, and CTRL on windows / linux
    bool mod_ctrl = strcmp(SDL_GetPlatform(), "Mac OS X") == 0
      ? event.key.keysym.mod & (KMOD_LGUI | KMOD_RGUI)
      : mod_ctrl = event.key.keysym.mod & (KMOD_LCTRL | KMOD_RCTRL);

    // Regular 'ol keys
    switch (event.key.keysym.sym) {
      case SDLK_SPACE:
        // Fast-Forward
        this->speedup = (event.type == SDL_KEYDOWN) ? 200 : 100;
        this->speed_counter = 0;
        this->nes.set_speed(this->speedup);
        break;
    }

    // Controller
    if (event.type == SDL_CONTROLLERBUTTONDOWN || event.type == SDL_CONTROLLERBUTTONUP) {
      switch (event.cbutton.button) {
      case SDL_CONTROLLER_BUTTON_RIGHTSTICK:
        this->speedup = (event.type == SDL_CONTROLLERBUTTONDOWN) ? 200 : 100;
        this->speed_counter = 0;
        this->nes.set_speed(this->speedup);
        break;
      }
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

void SDL_GUI::step_nes() {
  // Calculate the number of frames to render
  // (speedup values that are not clean multiples of 100 imply that some the
  //  number of nes frames / real frame will vary)
  uint numframes = 0;
  this->speed_counter += speedup;
  while (this->speed_counter > 0) {
    this->speed_counter -= 100;
    numframes++;
  }

  // Run ANESE core for some number of frames
  for (uint i = 0; i < numframes; i++) {
    // log frame to fm2
    if (this->fm2_record.is_enabled())
      this->fm2_record.step_frame();

    // set input from fm2
    if (this->fm2_replay.is_enabled())
      this->fm2_replay.step_frame();

    // run the NES for a frame
    this->nes.step_frame();
  }

  if (this->nes.isRunning() == false) {
    // this->sdl_running = true;
  }
}

void SDL_GUI::output_nes() {
  // output audio!
  short* samples = nullptr;
  uint   count = 0;
  this->nes.getAudiobuff(samples, count);
  if (count) this->sdl.nes_sound_queue.write(samples, count);

  // output video!
  const u8* framebuffer = this->nes.getFramebuff();
  SDL_UpdateTexture(this->sdl.nes_texture, nullptr, framebuffer, RES_X * 4);
  SDL_RenderCopy(this->sdl.renderer, this->sdl.nes_texture, nullptr, &this->sdl.nes_screen);
}


void SDL_GUI::update_input_ui(const SDL_Event& event) {
  if (event.type == SDL_KEYDOWN) {
    switch (event.key.keysym.sym) {
    case SDLK_RETURN: this->menu.hit.enter = true; break;
    case SDLK_DOWN:   this->menu.hit.down  = true; break;
    case SDLK_UP:     this->menu.hit.up    = true; break;
    case SDLK_LEFT:   this->menu.hit.left  = true; break;
    case SDLK_RIGHT:  this->menu.hit.right = true; break;
    }
    // basic "fast skipping" though file-names
    const char* key_pressed = SDL_GetKeyName(event.key.keysym.sym);
    if (key_pressed[1] == '\0' && (isalnum(key_pressed[0]) || key_pressed[0] == '.'))
      this->menu.hit.last_ascii = key_pressed[0];
  }

  if (event.type == SDL_CONTROLLERBUTTONDOWN || event.type == SDL_CONTROLLERBUTTONUP) {
    switch (event.cbutton.button) {
    case SDL_CONTROLLER_BUTTON_A:         this->menu.hit.enter = true; break;
    case SDL_CONTROLLER_BUTTON_DPAD_UP:   this->menu.hit.up    = true; break;
    case SDL_CONTROLLER_BUTTON_DPAD_DOWN: this->menu.hit.down  = true; break;
    case SDL_CONTROLLER_BUTTON_DPAD_LEFT: this->menu.hit.left  = true; break;
    }
  }

  std::vector<cf_file_t>& files = this->menu.files;

  if (this->menu.hit.enter || this->menu.hit.right) {
    this->menu.hit.enter = false;
    this->menu.hit.right = false;
    const cf_file_t& file = files[this->menu.selected_i];
    if (file.is_dir) {
      strcpy(this->menu.directory, file.path);
      this->menu.selected_i = 0;
      this->menu.should_update_dir = true;
    } else {
      fprintf(stderr, "[Menu] Selected '%s'\n", file.name);
      this->unload_rom(this->cart);
      this->load_rom(file.path);
      this->in_menu = false;
    }
  }

  if (this->menu.should_update_dir) {
    this->menu.should_update_dir = false;
    files.clear();

    // Get file-listing
    cf_dir_t dir;
    cf_dir_open(&dir, this->menu.directory);
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

  // Handle input

  if (this->menu.hit.up) {
    this->menu.hit.up = false;
    this->menu.selected_i -= this->menu.selected_i ? 1 : 0;
  }

  if (this->menu.hit.down) {
    this->menu.hit.down = false;
    this->menu.selected_i += this->menu.selected_i < files.size() - 1 ? 1 : 0;
  }

  if (this->menu.hit.left) {
    this->menu.hit.left = false;
    strcpy(this->menu.directory, files[0].path);
    this->menu.selected_i = 0;
  }

  if (this->menu.hit.last_ascii) {
    uint new_selection = std::distance(
      files.begin(),
      std::find_if(files.begin(), files.end(),
        [=](const cf_file_t& f){
          return f.name[0] == this->menu.hit.last_ascii
              || f.name[0] == tolower(this->menu.hit.last_ascii);
        })
    );
    if (new_selection < files.size())
      this->menu.selected_i = new_selection;

    this->menu.hit.last_ascii = '\0';
  }
}

void SDL_GUI::output_menu() {
  // Paint transparent bg
  this->sdl.bg.x = this->sdl.bg.y = 0;
  SDL_RenderGetLogicalSize(this->sdl.renderer, &this->sdl.bg.w, &this->sdl.bg.h);
  SDL_SetRenderDrawColor(this->sdl.renderer, 0, 0, 0, 200);
  SDL_RenderFillRect(this->sdl.renderer, &this->sdl.bg);

  // Paint menu
  for (uint i = 0; i < this->menu.files.size(); i++) {
    const cf_file_t& file = this->menu.files[i];

    u32 color;
    if (this->menu.selected_i == i)        color = 0xff0000; // red   - selected
    else if (strcmp("nes", file.ext) == 0) color = 0x00ff00; // green - .nes
    else if (strcmp("zip", file.ext) == 0) color = 0x00ffff; // cyan  - .zip
    else                                   color = 0xffffff; // white - folder

    SDL2_inprint::incolor(color, /* unused */ 0);
    SDL2_inprint::inprint(this->sdl.renderer, file.name,
      10, this->sdl.bg.h / 2 + (i - this->menu.selected_i) * 12
    );
  }
}

int SDL_GUI::run() {
  fprintf(stderr, "[SDL2] Running SDL2 GUI\n");

  // ------------------------------ Main Loop! ------------------------------ //
  double past_fups [20] = {60.0}; // more samples == less value jitter
  uint past_fups_i = 0;

  while (this->sdl_running) {
    typedef uint time_ms;
    time_ms frame_start_time = SDL_GetTicks();
    past_fups_i++;

    // Check for new events
    SDL_Event event;
    while (SDL_PollEvent(&event) != 0) {
      this->update_input_global(event);

      if (!this->in_menu) {
        this->update_input_nes_misc(event);
        this->update_input_nes_joypads(event);
      } else {
        this->update_input_ui(event);
      }
    }

    // Update the NES when not in menu
    if (!this->in_menu)
      this->step_nes();

    // Render something
    this->output_nes();
    if (this->in_menu) {
      this->output_menu();
    }

    // SHOW ME WHAT YOU GOT
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

  this->unload_rom(this->cart);
  delete this->cart;

  // SDL Cleanup
  SDL_GameControllerClose(this->sdl.controller);
  SDL_DestroyTexture(this->sdl.nes_texture);
  SDL_DestroyRenderer(this->sdl.renderer);
  SDL_DestroyWindow(this->sdl.window);
  SDL_Quit();

  SDL2_inprint::kill_inline_font();

  printf("\nANESE closed successfully\n");
}
