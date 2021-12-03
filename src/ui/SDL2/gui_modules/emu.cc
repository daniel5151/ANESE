#include "emu.h"

#include <cstdio>

#include "../fs/load.h"
#include "../fs/util.h"

EmuModule::EmuModule(SharedState& gui)
: GUIModule(gui)
{
  /*-------------------------------  SDL init  -------------------------------*/

  fprintf(stderr, "[GUI][Emu] Initializing...\n");

  // make window
  this->sdl.window = SDL_CreateWindow(
    "anese",
    SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
    256 * this->gui.config.window_scale,
    240 * this->gui.config.window_scale,
    SDL_WINDOW_RESIZABLE
  );

  // make renderer
  this->sdl.renderer = SDL_CreateRenderer(
    this->sdl.window,
    -1,
    SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
  );

  // make screen rect (to render texture onto)
  this->sdl.screen_rect.x = 0;
  this->sdl.screen_rect.y = 0;
  this->sdl.screen_rect.w = 256 * this->gui.config.window_scale;
  this->sdl.screen_rect.h = 240 * this->gui.config.window_scale;

  // Letterbox the screen in the window
  SDL_RenderSetLogicalSize(this->sdl.renderer,
    256 * this->gui.config.window_scale,
    240 * this->gui.config.window_scale);

  // Allow opacity
  SDL_SetRenderDrawBlendMode(this->sdl.renderer, SDL_BLENDMODE_BLEND);

  // NES screen texture
  this->sdl.screen_texture = SDL_CreateTexture(
    this->sdl.renderer,
    SDL_PIXELFORMAT_ARGB8888,
    SDL_TEXTUREACCESS_STREAMING,
    256, 240
  );

  // SDL_AudioSpec as, have;
  // as.freq = SDL_GUI::SAMPLE_RATE;
  // as.format = AUDIO_F32SYS;
  // as.channels = 1;
  // as.samples = 4096;
  // as.callback = nullptr; // use SDL_QueueAudio
  // this->sdl_common.nes_audiodev = SDL_OpenAudioDevice(NULL, 0, &as, &have, 0);
  // SDL_PauseAudioDevice(this->sdl_common.nes_audiodev, 0);

  this->sdl.sound_queue.init(this->gui.nes_params.apu_sample_rate);

  /*----------  Submodule Init  ----------*/

  this->menu_submodule = new MenuSubModule(gui, this->sdl.window, this->sdl.renderer);

  /*----------  NES Init  ----------*/

  this->gui.nes.attach_joy(0, &this->joy_1);
  this->gui.nes.attach_joy(1, &this->zap_2);

  // ---------------------------- Movie Support ----------------------------- //

  if (this->gui.config.cli.replay_fm2_path != "") {
    bool did_load = this->fm2_replay.init(this->gui.config.cli.replay_fm2_path.c_str());
    if (!did_load)
      fprintf(stderr, "[Replay][fm2] Movie loading failed!\n");
    else fprintf(stderr, "[Replay][fm2] Movie successfully loaded!\n");
  }

  if (this->gui.config.cli.record_fm2_path != "") {
    bool did_load = this->fm2_record.init(this->gui.config.cli.record_fm2_path.c_str());
    if (!did_load)
      fprintf(stderr, "[Record][fm2] Failed to setup Movie recording!\n");
    else fprintf(stderr, "[Record][fm2] Movie recording is setup!\n");
  }

  // pass controllers to this->fm2_record
  if (this->fm2_record.is_enabled()) {
    this->fm2_record.set_joy(0, FM2_Controller::SI_GAMEPAD, &this->joy_1);
    this->fm2_record.set_joy(1, FM2_Controller::SI_GAMEPAD, &this->joy_2);
  }

  // attach fm2_replay controllers (if needed)
  if (this->fm2_replay.is_enabled()) {
    this->gui.nes.attach_joy(0, this->fm2_replay.get_joy(0));
    this->gui.nes.attach_joy(1, this->fm2_replay.get_joy(1));
  }
}

EmuModule::~EmuModule() {
  fprintf(stderr, "[GUI][Emu] Shutting down...\n");

  delete this->menu_submodule;

  /*------------------------------  SDL Cleanup  -----------------------------*/

  SDL_DestroyTexture(this->sdl.screen_texture);
  SDL_DestroyRenderer(this->sdl.renderer);
  SDL_DestroyWindow(this->sdl.window);
}

void EmuModule::input(const SDL_Event& event) {
  this->menu_submodule->input(event);
  if (this->gui.status.in_menu) return;

  // Update from Controllers
  if (event.type == SDL_CONTROLLERBUTTONDOWN ||
      event.type == SDL_CONTROLLERBUTTONUP) {
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
  if (event.type == SDL_KEYDOWN ||
      event.type == SDL_KEYUP) {
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

  // Zapper

  // if (event.type == SDL_MOUSEMOTION) {
  //   // getting the light from the screen is a bit trickier...
  //   const u8* screen;
  //   this->gui.nes.getFramebuff(screen);
  //   const uint offset = (256 * 4 * (event.motion.y / this->gui.config.window_scale))
  //                     + (      4 * (event.motion.x / this->gui.config.window_scale));
  //   const bool new_light = screen[offset+ 0]  // R
  //                        | screen[offset+ 1]  // G
  //                        | screen[offset+ 2]; // B
  //   this->zap_2.set_light(new_light);
  // }

  // if (event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP) {
  //   bool new_state = (event.type == SDL_MOUSEBUTTONDOWN) ? true : false;
  //   this->zap_2.set_trigger(new_state);
  // }

  // Handle Key-events
  if (event.type == SDL_KEYDOWN ||
      event.type == SDL_KEYUP) {
    bool mod_shift = event.key.keysym.mod & KMOD_SHIFT;
    // Use CMD on macOS, and CTRL on windows / linux
    bool mod_ctrl = strcmp(SDL_GetPlatform(), "Mac OS X") == 0
      ? event.key.keysym.mod & (KMOD_LGUI | KMOD_RGUI)
      : event.key.keysym.mod & (KMOD_LCTRL | KMOD_RCTRL);

    // Regular 'ol keys
    switch (event.key.keysym.sym) {
      case SDLK_SPACE:
        // Fast-Forward
        this->speed_counter = 0;
        this->gui.nes_params.speed = (event.type == SDL_KEYDOWN) ? 200 : 100;
        this->gui.nes.updated_params();
        break;
    }

    // Controller
    if (event.type == SDL_CONTROLLERBUTTONDOWN ||
        event.type == SDL_CONTROLLERBUTTONUP) {
      switch (event.cbutton.button) {
      case SDL_CONTROLLER_BUTTON_RIGHTSTICK:
        this->speed_counter = 0;
        this->gui.nes_params.speed = (event.type == SDL_CONTROLLERBUTTONDOWN) ? 200 : 100;
        this->gui.nes.updated_params();
        break;
      }
    }

    // Meta Modified keys
    if (event.type == SDL_KEYDOWN && mod_ctrl) {
      #define SAVESTATE(i) do {                                   \
        if (mod_shift) {                                          \
          delete this->gui.savestate[i];                          \
          this->gui.savestate[i] = this->gui.nes.serialize();     \
        } else this->gui.nes.deserialize(this->gui.savestate[i]); \
      } while(0);

      switch (event.key.keysym.sym) {
      case SDLK_1: SAVESTATE(0); break; // Savestate Slot 1
      case SDLK_2: SAVESTATE(1); break; // Savestate Slot 2
      case SDLK_3: SAVESTATE(2); break; // Savestate Slot 3
      case SDLK_4: SAVESTATE(3); break; // Savestate Slot 4
      case SDLK_r: this->gui.nes.reset();       break; // Reset
      case SDLK_p: this->gui.nes.power_cycle(); break; // Power-Cycle
      case SDLK_EQUALS:
        // Speed up
        this->speed_counter = 0;
        this->gui.nes_params.speed += 25;
        this->gui.nes.updated_params();
        break;
      case SDLK_MINUS:
        // Speed down
        if (this->gui.nes_params.speed - 25 != 0) {
          this->speed_counter = 0;
          this->gui.nes_params.speed -= 25;
          this->gui.nes.updated_params();
        }
        break;
      case SDLK_c: {
        // Toggle CPU trace
        bool log = this->gui.nes_params.log_cpu = !this->gui.nes_params.log_cpu;
        this->gui.nes.updated_params();
        fprintf(stderr, "NESTEST CPU logging: %s\n", log ? "ON" : "OFF");
      } break;
      default: break;
      }
    }
  }
}

void EmuModule::update() {
  this->menu_submodule->update();
  if (this->gui.status.in_menu) return;

  // log frame to fm2
  if (this->fm2_record.is_enabled())
    this->fm2_record.step_frame();

  // set input from fm2
  if (this->fm2_replay.is_enabled())
    this->fm2_replay.step_frame();
}

void EmuModule::output() {
  // output audio!
  float* samples = nullptr;
  uint   count = 0;
  this->gui.nes.getAudiobuff(&samples, &count);
  // SDL_QueueAudio(this->gui.sdl.nes_audiodev, samples, count * sizeof(float));
  if (count) this->sdl.sound_queue.write(samples, count);

  // output video!
  const u8* framebuffer;
  this->gui.nes.getFramebuff(&framebuffer);
  SDL_UpdateTexture(this->sdl.screen_texture, nullptr, framebuffer, 256 * 4);

  // actual NES screen
  SDL_SetRenderDrawColor(this->sdl.renderer, 0, 0, 0, 0xff);
  SDL_RenderClear(this->sdl.renderer);
  SDL_RenderCopy(this->sdl.renderer, this->sdl.screen_texture, nullptr, &this->sdl.screen_rect);

  this->menu_submodule->output();

  SDL_RenderPresent(this->sdl.renderer);

  // Present fups though the title of the main window
  char window_title [64];
  sprintf(window_title, "anese - %u fups - %u%% speed",
    uint(this->gui.status.avg_fps), this->gui.nes_params.speed);
  SDL_SetWindowTitle(this->sdl.window, window_title);
}
