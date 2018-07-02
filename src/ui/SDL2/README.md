# SDL GUI

It's not the cleanest code, but it works, and is pretty modular.

- `SDL_GUI` - `gui.h/cc` - Core SDL functionality
  - Set up + teardown main Renderer, Window, Controllers, etc...
  - Contains main Loop
  - Initializes and routes events / output to and from `GUIModules`
- `gui_modules` - Individual components of the UI
  - Implement simple interface (accept input, update self, render output)
  - `EmuModule` - ANESE core
    - Owns, updates, and presents the output of ANESE core
    - Handles loading / unloading ROMs, Movies, and savs
  - `MenuModule` - Menu system
    - Runs the menu system
    - Has reference to `EmuModule` (to load ROMs)
