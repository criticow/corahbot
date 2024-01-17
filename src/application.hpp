#pragma once

#include <glb/glb.hpp>
#include "gui.hpp"
#include "emulator.hpp"

class Application
{
  public:
  Window window;
  Emulator emulator;
  Time time;
  Input input;

  Application(int width, int height, const char *title);
  void run();

  private:
  GUI gui;

  void render();
  void update();
  void setup();
  void appLoop();
  void destroy();
};