#pragma once

#include <glb/glb.hpp>
#include "util.hpp"
#include <Windows.h>
#include <wingdi.h>
#include <opencv2/opencv.hpp>

#define CORAH_PACKAGE_NAME "com.coorangar.corah"

struct Instance
{
  uint16_t index;
  std::string name;
  HWND hwnd;
  int PID;
  bool isRunning;
  bool isWorking;
  bool isDebugging;
};

class Emulator
{
  public:
  std::vector<Instance> instances;

  Emulator(){};

  void arrange();
  void list();
  void launch(uint16_t index);
  void runapp(uint16_t index, const std::string &packageName);
  void quit(uint16_t index);
  void destroy();

  cv::Mat printscreen(const std::string &windowTitle);

  private:
  std::string console = "C:/Nox/bin/NoxConsole.exe";
};