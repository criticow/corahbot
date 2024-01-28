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
  std::string instance;
  WorkConfig *config = nullptr;
  Summary *summary = nullptr;
  std::string currentRoutine;
  std::string currentAction;

  std::string findLocation(const std::string &instance);
  void handleFighting(int &swords, int &potions);
  void handleGear();
  void handleBook();
  void handleInventory();
  void handleItemOpen();
  void handleLogin();
  void handleHome();
  void handleRefill();
  void handleMap();
  void handleQuests();
  void handleQuestReward();
};