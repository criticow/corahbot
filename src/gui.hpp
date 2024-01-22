#pragma once

#include <glb/glb.hpp>
#include <fontawesome6.h>
#include <chrono>
#include <opencv2/opencv.hpp>
#include "bot.hpp"

class Application;

class GUI : public BaseGUI
{
  public:
  std::unordered_map<std::string, InstanceState> instanceStates;
  std::unordered_map<std::string, std::mutex> instanceMutexes;

  std::vector<std::string> instances;

  GUI() : BaseGUI(){};
  virtual void renderUI() override;
  void farmUI(const std::string &instance);
  void combineUI(const std::string &instance);
  void cleanup();
  void update();
  void init();


  private:
  std::unordered_map<std::string, WorkConfig> configs;
  void loadFonts();
};