#pragma once

#include <glb/glb.hpp>
#include "gui.hpp"
#include <opencv2/core/utils/logger.hpp>
#include "bot.hpp"
#include <Windows.h>
#include <psapi.h>
#include "store.hpp"

class Application
{
  public:
  Window window;
  static Temporizer tempo;
  static bool vSyncEnabled;
  static Input input;

  Application(int width, int height, const char *title);
  void run();
  static void toggleVsync();

  private:
  GUI gui;

  void render();
  void update();
  void setup();
  void appLoop();
  void destroy();
};