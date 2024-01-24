#pragma once

#include <glb/glb.hpp>
#include "util.hpp"
#include <Windows.h>
#include <Psapi.h>
#include <wingdi.h>
#include <opencv2/opencv.hpp>
#include "store.hpp"

#define CORAH_PACKAGE_NAME "com.coorangar.corah"

struct Marker;

class Emulator
{
  public:

  static void arrange();
  // static std::vector<std::string> listInstances();
  static std::vector<std::string> list();

  static void launch(int index);
  static void runapp(const std::string &windowTitle, const std::string &packageName);
  static void killappall(const std::string &windowTitle);
  static void quit(const std::string &windowTitle);
  static void click(const std::string &windowTitle, glm::ivec2 point);
  static void click(const std::string &windowTitle, Marker &marker);

  static std::pair<bool, glm::ivec4> find(const std::string &windowTitle, const std::string &needlePath, float threshold);
  static std::pair<bool, glm::ivec4> find(const std::string &windowTitle, Marker marker , const std::string &haystackPath);
  static std::pair<bool, glm::ivec4> find(cv::Mat haystack, cv::Mat needle);
  
  static void drawRectangles(cv::Mat &canvas, const std::vector<glm::ivec4> &points);
  static bool compareImages(const std::string &windowTitle, Marker marker);

  static cv::Mat printscreen(const std::string &windowTitle);
  static cv::Mat printscreen(const std::string &windowTitle, int x, int y, int width, int height);
};