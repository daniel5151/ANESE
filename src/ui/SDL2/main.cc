#include "gui.h"

int main(int argc, char* argv[]) {
  SDL_GUI gui;
  if (int error = gui.init(argc, argv))
    return error;
  return gui.run();
}
