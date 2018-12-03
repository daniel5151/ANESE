#include "menu.h"

#include <algorithm>

#include <cute_files.h>
#include <SDL_inprint2.h>
#include <SDL.h>

#include "../../fs/util.h"

MenuSubModule::MenuSubModule(SharedState& gui, SDL_Window* window, SDL_Renderer* renderer)
: GUISubModule(gui, window, renderer)
{
  fprintf(stderr, "[GUI][Menu] Initializing...\n");

  // Update from config
  strcpy(this->nav.directory, this->gui.config.roms_dir);

  // Setup SDL2_inprint font
  this->inprint = new SDL2_inprint(this->renderer);
}

MenuSubModule::~MenuSubModule() {
  fprintf(stderr, "[GUI][Menu] Shutting down...\n");

  // Update config
  ANESE_fs::util::get_abs_path(this->gui.config.roms_dir, this->nav.directory, 260);

  delete this->inprint;

  // unload ROM
  this->gui.unload_rom();
}

void MenuSubModule::input(const SDL_Event& event) {
  // First, check if actions should even be recorded...
  if (event.type == SDL_KEYDOWN) {
    switch (event.key.keysym.sym) {
    case SDLK_ESCAPE:
      this->gui.status.in_menu = !this->gui.status.in_menu; break;
    }
  }
  if (event.type == SDL_CONTROLLERBUTTONDOWN ||
      event.type == SDL_CONTROLLERBUTTONUP) {
    switch (event.cbutton.button) {
    case SDL_CONTROLLER_BUTTON_LEFTSTICK:
      this->gui.status.in_menu = !this->gui.status.in_menu; break;
    }
  }

  if (!this->gui.status.in_menu) return;

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

void MenuSubModule::update() {
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
      this->gui.unload_rom();
      this->gui.load_rom(file.path);
      this->gui.status.in_menu = false;
    }
  }

  // Potentially update directory listing
  if (this->nav.should_update_dir) {
    this->nav.should_update_dir = false;
    files.clear();

    // Get file-listing
    cf_dir_t dir;
    cf_dir_open(&dir, this->nav.directory);
    while (dir.has_next) {
      cf_file_t file;
      cf_read_file(&dir, &file);
      files.push_back(file);
      cf_dir_next(&dir);
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

void MenuSubModule::output() {
  if (!this->gui.status.in_menu) return;
  // menu uses the EmuModule's rendering context

  // Paint transparent bg
  this->bg.x = this->bg.y = 0;
  SDL_RenderGetLogicalSize(this->renderer, &this->bg.w, &this->bg.h);
  if (this->bg.w == 0 && this->bg.h == 0)
    SDL_GetWindowSize(this->window, &this->bg.w, &this->bg.h);
  SDL_SetRenderDrawColor(this->renderer, 0, 0, 0, 200);
  SDL_RenderFillRect(this->renderer, &this->bg);

  // Paint menu
  for (uint i = 0; i < this->nav.files.size(); i++) {
    const cf_file_t& file = this->nav.files[i];

    u32 color;
    if (this->nav.selected_i == i)         color = 0xff0000; // red   - selected
    else if (strcmp("nes", file.ext) == 0) color = 0x00ff00; // green - .nes
    else if (strcmp("zip", file.ext) == 0) color = 0x00ffff; // cyan  - .zip
    else                                   color = 0xffffff; // white - folder

    this->inprint->set_color(color);
    this->inprint->print(file.name, 10, this->bg.h / 2 + (i - this->nav.selected_i) * 12);
  }
}
