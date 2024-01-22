#pragma once

#include <glb/glb.hpp>
#include "gui.hpp"
#include <opencv2/core/utils/logger.hpp>
#include "bot.hpp"
#include <Windows.h>
#include <psapi.h>

class Application
{
  public:
  Window window;
  static Time time;
  Input input;
  static Texture2D gemsTexture;

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