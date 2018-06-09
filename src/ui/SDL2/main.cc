#include "gui.h"

int main(int argc, char* argv[]) {
  SDL_GUI gui (argc, argv);
  if (int error = gui.init_error())
    return error;

  return gui.run();
}
