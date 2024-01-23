#pragma once

#include <glb/glb.hpp>
#include <fontawesome6.h>
#include <chrono>
#include <opencv2/opencv.hpp>
#include "bot.hpp"
#include "store.hpp"

class Application;

class GUI : public BaseGUI
{
  public:
  GUI() : BaseGUI(){};
  virtual void renderUI() override;
  void cleanup();
  void update();
  void init();

  private:
  Temporizer tempo;
  std::vector<std::string> instances;

  void loadFonts();
  void farmUI(const std::string &instance);
  void combineUI(const std::string &instance);
  void statesUI(const std::string &instance);
  void actionsUI(const std::string &instance);
  void summaryUI(const std::string &instance);
};