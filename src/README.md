# Directory Structure

- `common/`
  - Contains useful tidbits of code used by ANESE core, and by the UI
    - ex: typdefs, utility functions, my Serialization library
- `nes/`
  - Conatins the ANESE core.
  - Is completely self-contained, with any files in `nes/` only `#include`ing
    files that are themselves under `nes/` and `common/`
  - No dependencies aside from clib (doesn't use any STL!)
- `ui/`
  - Contain the various frontends to ANESE
    - At the moment, there is only one: `ui/SDL2`
    - In the future: LibRetro core?
  - Contains all the wideNES code!
  - Handles all code not-directly related to emulating NES
    - ex: saving/loading files, outputting audio/video, reading input

