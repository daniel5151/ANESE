#include "gui.h"

#include <algorithm>

#define CUTE_FILES_IMPLEMENTATION
#include <cute_files.h>
#include <SDL2_inprint.h>

void SDL_GUI::Menu::init() {
  // nada
}

SDL_GUI::Menu::~Menu() {
  // nada
}

void SDL_GUI::Menu::input(const SDL_Event& event) {
  if (event.type == SDL_KEYDOWN) {
    switch (event.key.keysym.sym) {
    case SDLK_RETURN: this->menu.hit.enter = true; break;
    case SDLK_DOWN:   this->menu.hit.down  = true; break;
    case SDLK_UP:     this->menu.hit.up    = true; break;
    case SDLK_LEFT:   this->menu.hit.left  = true; break;
    case SDLK_RIGHT:  this->menu.hit.right = true; break;
    }
    // support for basic "fast skipping" though file-names
    const char* key_pressed = SDL_GetKeyName(event.key.keysym.sym);
    bool is_valid_char = isalnum(key_pressed[0])
                      || key_pressed[0] == '.';
    if (is_valid_char && key_pressed[1] == '\0')
      this->menu.hit.last_ascii = key_pressed[0];
    // Special case: space
    if (event.key.keysym.sym == SDLK_SPACE)
      this->menu.hit.last_ascii = ' ';
  }

  if (event.type == SDL_CONTROLLERBUTTONDOWN || event.type == SDL_CONTROLLERBUTTONUP) {
    switch (event.cbutton.button) {
    case SDL_CONTROLLER_BUTTON_A:         this->menu.hit.enter = true; break;
    case SDL_CONTROLLER_BUTTON_DPAD_UP:   this->menu.hit.up    = true; break;
    case SDL_CONTROLLER_BUTTON_DPAD_DOWN: this->menu.hit.down  = true; break;
    case SDL_CONTROLLER_BUTTON_DPAD_LEFT: this->menu.hit.left  = true; break;
    }
  }
}

void SDL_GUI::Menu::update() {
  std::vector<cf_file_t>& files = this->menu.files; // helpful alias

  // First, check if the user wants to navigate / load a rom
  if (this->menu.hit.enter || this->menu.hit.right) {
    this->menu.hit.enter = false;
    this->menu.hit.right = false;
    const cf_file_t& file = files[this->menu.selected_i];
    if (file.is_dir) {
      // Navigate into directory
      strcpy(this->menu.directory, file.path);
      this->menu.selected_i = 0;
      this->menu.should_update_dir = true;
    } else {
      // Load-up ROM (and close menu)
      fprintf(stderr, "[Menu] Selected '%s'\n", file.name);
      this->gui.unload_rom(this->gui.emu.cart);
      this->gui.load_rom(file.path);
      this->in_menu = false;
    }
  }

  // Potentially update directory listing
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

  // Handle navigation...

  if (this->menu.hit.up) {
    this->menu.hit.up = false;
    this->menu.selected_i -= this->menu.selected_i ? 1 : 0;
  }

  if (this->menu.hit.down) {
    this->menu.hit.down = false;
    this->menu.selected_i += (this->menu.selected_i < (files.size() - 1));
  }

  if (this->menu.hit.left) {
    this->menu.hit.left = false;
    strcpy(this->menu.directory, files[0].path);
    this->menu.selected_i = 0;
  }

  if (this->menu.quicknav.timeout) { this->menu.quicknav.timeout--; }
  else {
    this->menu.quicknav.i = 0;
    memset(this->menu.quicknav.buf, '\0', 16);
  }

  if (this->menu.hit.last_ascii) {
    // clear buffer in ~1s from last keypress
    this->menu.quicknav.timeout = 30;

    // buf[15] is always '\0'
    this->menu.quicknav.buf[this->menu.quicknav.i++ % 15] = tolower(this->menu.hit.last_ascii);

    uint new_selection = std::distance(
      files.begin(),
      std::find_if(files.begin(), files.end(),
        [=](const cf_file_t& f){
          std::string fname = std::string(f.name).substr(0, this->menu.quicknav.i);
          std::transform(fname.begin(), fname.end(), fname.begin(), tolower);
          return strcmp(fname.c_str(), this->menu.quicknav.buf) == 0;
        })
    );
    if (new_selection < files.size())
      this->menu.selected_i = new_selection;

    this->menu.hit.last_ascii = '\0';
  }
}

void SDL_GUI::Menu::output() {
  /*----------  Rendering  ----------*/

  // Paint transparent bg
  this->bg.x = this->bg.y = 0;
  SDL_RenderGetLogicalSize(this->gui.sdl.renderer, &this->bg.w, &this->bg.h);
  SDL_SetRenderDrawColor(this->gui.sdl.renderer, 0, 0, 0, 200);
  SDL_RenderFillRect(this->gui.sdl.renderer, &this->bg);

  // Paint menu
  for (uint i = 0; i < this->menu.files.size(); i++) {
    const cf_file_t& file = this->menu.files[i];

    u32 color;
    if (this->menu.selected_i == i)        color = 0xff0000; // red   - selected
    else if (strcmp("nes", file.ext) == 0) color = 0x00ff00; // green - .nes
    else if (strcmp("zip", file.ext) == 0) color = 0x00ffff; // cyan  - .zip
    else                                   color = 0xffffff; // white - folder

    SDL2_inprint::incolor(color, /* unused */ 0);
    SDL2_inprint::inprint(this->gui.sdl.renderer, file.name,
      10, this->bg.h / this->gui.SCREEN_SCALE + (i - this->menu.selected_i) * 12
    );
  }
}
