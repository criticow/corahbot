#pragma once

#include <glb/glb.hpp>
#include <fontawesome6.h>
#include <chrono>
#include <opencv2/opencv.hpp>
#include <Windows.h>
#include "bot.hpp"

class Application;

class GUI : public BaseGUI
{
  public:
  GUI(){};
  GUI(Application *app) : BaseGUI()
  {
    this->app = app;
  };
  virtual void renderUI() override;
  void addFonts();
  void destroy();

  private:
  Application *app = nullptr;
  std::unordered_map<int, std::mutex> mutexes;
  std::unordered_map<int, std::thread> threads;
  void mainUI();
  void automationUI();
  void botThread(Instance &instance, Emulator &emulator);
};