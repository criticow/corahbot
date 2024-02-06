#pragma once

#include <glb/glb.hpp>
#include <fontawesome6.h>
#include <chrono>
#include <opencv2/opencv.hpp>
#include "bot.hpp"
#include "store.hpp"
#include "work_config.hpp"

class Application;

class GUI : public BaseGUI
{
  public:
  GUI() : BaseGUI(){};
  virtual void renderUI() override;
  void cleanup();
  void init();

  private:
  Temporizer tempo;
  std::vector<std::string> instances;

  void loadFonts();
  void statesUI(const std::string &instance);
  void actionsUI(const std::string &instance);
  void farmUI(const std::string &instance);
  void buffsUI(const std::string &instance);
  void questsUI(const std::string &instance);
  void summaryUI(const std::string &instance);

  void selectableList(const std::string &instance, const std::string &name, std::vector<Selectable> &selectableList, std::vector<std::string> &selectedItems);
};