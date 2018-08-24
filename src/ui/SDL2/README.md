# SDL GUI

It's not the cleanest code, but it works, and is pretty modular.

- `SDL_GUI` - `gui.h/cc` - Core SDL functionality
  - Set up + teardown main Renderer, Window, Controllers, etc...
  - Contains main Loop
  - Initializes and routes events / output to and from `GUIModules`
- `gui_modules` - Individual components of the UI
  - Implement simple interface (accept input, update self, render output)
  - `emu` - Basic NES output. Handles keyboard input and such.
  - `ppu-debug` - A collection of debug windows for the PPU
  - `widenes` - ANESE's wideNES implementation
