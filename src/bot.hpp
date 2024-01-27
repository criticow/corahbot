#pragma once

#include <glb/glb.hpp>
#include <Windows.h>
#include "emulator.hpp"
#include "store.hpp"
#include "work_config.hpp"
#include "generated_macros.hpp"

class Bot
{
  public:
  Bot(){};
  void run(const std::string &instance);

  private:
  Temporizer tempo;

  std::string findLocation(const std::string &instance);
  void handleFighting(const std::string &instance, WorkConfig &config, Summary &summary, std::string &currentRoutine, std::string &currentAction, int &swords, int &potions);
  void handleGear(const std::string &instance, WorkConfig &config, Summary &summary, std::string &currentRoutine, std::string &currentAction);
  void handleBook(const std::string &instance, WorkConfig &config, Summary &summary, std::string &currentRoutine, std::string &currentAction);
  void handleInventory(const std::string &instance, WorkConfig &config, Summary &summary, std::string &currentRoutine, std::string &currentAction);
  void handleItemOpen(const std::string &instance, WorkConfig &config, Summary &summary, std::string &currentRoutine, std::string &currentAction);
  void handleLogin(const std::string &instance, WorkConfig &config, Summary &summary, std::string &currentRoutine, std::string &currentAction);
  void handleHome(const std::string &instance, WorkConfig &config, Summary &summary, std::string &currentRoutine, std::string &currentAction);
  void handleRefill(const std::string &instance, WorkConfig &config, Summary &summary, std::string &currentRoutine, std::string &currentAction);
  void handleMap(const std::string &instance, WorkConfig &config, Summary &summary, std::string &currentRoutine, std::string &currentAction);
};