#pragma once

#include <glb/glb.hpp>
#include "util.hpp"
#include <Windows.h>
#include <Psapi.h>
#include <wingdi.h>
#include <opencv2/opencv.hpp>

#define CORAH_PACKAGE_NAME "com.coorangar.corah"

struct InstanceState
{
  std::atomic<bool> open = false;
  std::atomic<bool> working = false;
  std::atomic<bool> minimized = false;

  // Default constructor
  InstanceState() = default;

  // Custom copy assignment operator
  InstanceState& operator=(const InstanceState& other) {
    if (this != &other) {
      open.store(other.open.load());
      working.store(other.working.load());
      minimized.store(other.minimized.load());
    }
    return *this;
  }

  // Custom constructor
  InstanceState(bool open, bool working, bool minimized) : open(open), working(working), minimized(minimized) {}
};

struct Marker;

class Emulator
{
  public:

  static void arrange();
  static std::vector<std::string> list();
  static void launch(int index);
  static void runapp(int index, const std::string &packageName);
  static void quit(int index);
  static void click(const std::string &windowTitle, glm::ivec2 point);
  static std::pair<bool, glm::ivec4> find(const std::string &windowTitle, const std::string &needlePath, float threshold);
  static std::pair<bool, glm::ivec4> find(const std::string &windowTitle, Marker marker , const std::string &haystackPath);
  static void drawRectangles(cv::Mat &canvas, const std::vector<glm::ivec4> &points);
  static bool compareImages(const std::string &windowTitle, Marker marker);

  static cv::Mat printscreen(const std::string &windowTitle);
  static cv::Mat printscreen(const std::string &windowTitle, int x, int y, int width, int height);

  private:
  static std::pair<bool, glm::ivec4> find(cv::Mat haystack, cv::Mat needle);
};