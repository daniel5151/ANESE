#include "gui.h"

#include "common/debug.h"

/*------------------------------  Global Events  -----------------------------*/

/**
 * @brief Handle SDL_Events corresponding to global actions (e.g: quit)
 *
 * @param event SDL_Event
 */
void SDL_GUI::update_input_global(const SDL_Event& event) {
  if (
    (event.type == SDL_QUIT) ||
    (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE)
  ) this->sdl_running = false;

  if (event.type == SDL_KEYDOWN) {
    switch (event.key.keysym.sym) {
      case SDLK_ESCAPE: this->ui.in_menu = !this->ui.in_menu; break;
    }
  }

  if (event.type == SDL_CONTROLLERBUTTONDOWN || event.type == SDL_CONTROLLERBUTTONUP) {
    switch (event.cbutton.button) {
    case SDL_CONTROLLER_BUTTON_LEFTSTICK: this->ui.in_menu = !this->ui.in_menu; break;
    }
  }
}

/*-------------------------------  NES Events  -------------------------------*/

/**
 * @brief Handle SDL_Events corresponding to NES joypad/zapper input
 *
 * @param event SDL_Event
 */
void SDL_GUI::update_input_nes_joypads(const SDL_Event& event) {
  // Update from Controllers
  if (event.type == SDL_CONTROLLERBUTTONDOWN || event.type == SDL_CONTROLLERBUTTONUP) {
    bool new_state = (event.type == SDL_CONTROLLERBUTTONDOWN) ? true : false;
    using namespace JOY_Standard_Button;

    // Player 1
    switch (event.cbutton.button) {
    case SDL_CONTROLLER_BUTTON_A:          this->nes.joy_1.set_button(A,      new_state); break;
    case SDL_CONTROLLER_BUTTON_X:          this->nes.joy_1.set_button(B,      new_state); break;
    case SDL_CONTROLLER_BUTTON_START:      this->nes.joy_1.set_button(Start,  new_state); break;
    case SDL_CONTROLLER_BUTTON_BACK:       this->nes.joy_1.set_button(Select, new_state); break;
    case SDL_CONTROLLER_BUTTON_DPAD_UP:    this->nes.joy_1.set_button(Up,     new_state); break;
    case SDL_CONTROLLER_BUTTON_DPAD_DOWN:  this->nes.joy_1.set_button(Down,   new_state); break;
    case SDL_CONTROLLER_BUTTON_DPAD_LEFT:  this->nes.joy_1.set_button(Left,   new_state); break;
    case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: this->nes.joy_1.set_button(Right,  new_state); break;
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
    case SDLK_z:      this->nes.joy_1.set_button(A,      new_state); break;
    case SDLK_x:      this->nes.joy_1.set_button(B,      new_state); break;
    case SDLK_RETURN: this->nes.joy_1.set_button(Start,  new_state); break;
    case SDLK_RSHIFT: this->nes.joy_1.set_button(Select, new_state); break;
    case SDLK_UP:     this->nes.joy_1.set_button(Up,     new_state); break;
    case SDLK_DOWN:   this->nes.joy_1.set_button(Down,   new_state); break;
    case SDLK_LEFT:   this->nes.joy_1.set_button(Left,   new_state); break;
    case SDLK_RIGHT:  this->nes.joy_1.set_button(Right,  new_state); break;
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

  // Update from Mouse
  if (event.type == SDL_MOUSEMOTION) {
    // getting the light from the screen is a bit trickier...
    const u8* screen = this->nes.console.getFramebuff();
    const uint offset = (256 * 4 * (event.motion.y / SCREEN_SCALE))
                      + (event.motion.x / SCREEN_SCALE) * 4;
    const bool new_light = screen[offset+ 0]  // R
                         | screen[offset+ 1]  // G
                         | screen[offset+ 2]; // B
    this->nes.zap_2.set_light(new_light);
  }

  if (event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP) {
    bool new_state = (event.type == SDL_MOUSEBUTTONDOWN) ? true : false;
    this->nes.zap_2.set_trigger(new_state);
  }
}

/**
 * @brief Handle SDL_Events corresponding to NES emulator actions
 *
 * @param event SDL_Event
 */
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
        this->nes.speedup = (event.type == SDL_KEYDOWN) ? 200 : 100;
        this->nes.speed_counter = 0;
        this->nes.console.set_speed(this->nes.speedup);
        break;
    }

    // Controller
    if (event.type == SDL_CONTROLLERBUTTONDOWN || event.type == SDL_CONTROLLERBUTTONUP) {
      switch (event.cbutton.button) {
      case SDL_CONTROLLER_BUTTON_RIGHTSTICK:
        this->nes.speedup = (event.type == SDL_CONTROLLERBUTTONDOWN) ? 200 : 100;
        this->nes.speed_counter = 0;
        this->nes.console.set_speed(this->nes.speedup);
        break;
      }
    }

    // Meta Modified keys
    if (event.type == SDL_KEYDOWN && mod_ctrl) {
      #define SAVESTATE(i) do {                                       \
        if (mod_shift) {                                              \
          delete this->nes.savestate[i];                              \
          this->nes.savestate[i] = this->nes.console.serialize();     \
        } else this->nes.console.deserialize(this->nes.savestate[i]); \
      } while(0);

      switch (event.key.keysym.sym) {
      case SDLK_1: SAVESTATE(0); break; // Savestate Slot 1
      case SDLK_2: SAVESTATE(1); break; // Savestate Slot 2
      case SDLK_3: SAVESTATE(2); break; // Savestate Slot 3
      case SDLK_4: SAVESTATE(3); break; // Savestate Slot 4
      case SDLK_r: this->nes.console.reset();       break; // Reset
      case SDLK_p: this->nes.console.power_cycle(); break; // Power-Cycle
      case SDLK_EQUALS:
        // Speed up
        this->nes.console.set_speed(this->nes.speedup += 25);
        this->nes.speed_counter = 0;
        break;
      case SDLK_MINUS:
        // Speed down
        if (this->nes.speedup - 25 != 0) {
          this->nes.console.set_speed(this->nes.speedup -= 25);
          this->nes.speed_counter = 0;
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

/*-------------------------------  UI Events  --------------------------------*/

#include <algorithm>

#define CUTE_FILES_IMPLEMENTATION
#include <cute_files.h>

/**
 * @brief Handle SDL_Events corresponding to UI actions
 *
 * @param event SDL_Event
 */
void SDL_GUI::update_input_ui(const SDL_Event& event) {
  if (event.type == SDL_KEYDOWN) {
    switch (event.key.keysym.sym) {
    case SDLK_RETURN: this->ui.menu.hit.enter = true; break;
    case SDLK_DOWN:   this->ui.menu.hit.down  = true; break;
    case SDLK_UP:     this->ui.menu.hit.up    = true; break;
    case SDLK_LEFT:   this->ui.menu.hit.left  = true; break;
    case SDLK_RIGHT:  this->ui.menu.hit.right = true; break;
    }
    // basic "fast skipping" though file-names
    const char* key_pressed = SDL_GetKeyName(event.key.keysym.sym);
    if (key_pressed[1] == '\0' && (isalnum(key_pressed[0]) || key_pressed[0] == '.'))
      this->ui.menu.hit.last_ascii = key_pressed[0];
  }

  if (event.type == SDL_CONTROLLERBUTTONDOWN || event.type == SDL_CONTROLLERBUTTONUP) {
    switch (event.cbutton.button) {
    case SDL_CONTROLLER_BUTTON_A:         this->ui.menu.hit.enter = true; break;
    case SDL_CONTROLLER_BUTTON_DPAD_UP:   this->ui.menu.hit.up    = true; break;
    case SDL_CONTROLLER_BUTTON_DPAD_DOWN: this->ui.menu.hit.down  = true; break;
    case SDL_CONTROLLER_BUTTON_DPAD_LEFT: this->ui.menu.hit.left  = true; break;
    }
  }

  std::vector<cf_file_t>& files = this->ui.menu.files;

  if (this->ui.menu.hit.enter || this->ui.menu.hit.right) {
    this->ui.menu.hit.enter = false;
    this->ui.menu.hit.right = false;
    const cf_file_t& file = files[this->ui.menu.selected_i];
    if (file.is_dir) {
      strcpy(this->ui.menu.directory, file.path);
      this->ui.menu.selected_i = 0;
      this->ui.menu.should_update_dir = true;
    } else {
      fprintf(stderr, "[Menu] Selected '%s'\n", file.name);
      this->unload_rom(this->nes.cart);
      this->load_rom(file.path);
      this->ui.in_menu = false;
    }
  }

  if (this->ui.menu.should_update_dir) {
    this->ui.menu.should_update_dir = false;
    files.clear();

    // Get file-listing
    cf_dir_t dir;
    cf_dir_open(&dir, this->ui.menu.directory);
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

  if (this->ui.menu.hit.up) {
    this->ui.menu.hit.up = false;
    this->ui.menu.selected_i -= this->ui.menu.selected_i ? 1 : 0;
  }

  if (this->ui.menu.hit.down) {
    this->ui.menu.hit.down = false;
    this->ui.menu.selected_i += this->ui.menu.selected_i < files.size() - 1 ? 1 : 0;
  }

  if (this->ui.menu.hit.left) {
    this->ui.menu.hit.left = false;
    strcpy(this->ui.menu.directory, files[0].path);
    this->ui.menu.selected_i = 0;
  }

  if (this->ui.menu.hit.last_ascii) {
    uint new_selection = std::distance(
      files.begin(),
      std::find_if(files.begin(), files.end(),
        [=](const cf_file_t& f){
          return f.name[0] == this->ui.menu.hit.last_ascii
              || f.name[0] == tolower(this->ui.menu.hit.last_ascii);
        })
    );
    if (new_selection < files.size())
      this->ui.menu.selected_i = new_selection;

    this->ui.menu.hit.last_ascii = '\0';
  }
}
