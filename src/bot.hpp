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
  int questsDone = 0;
  std::string location = CB_LOCATION_UNKNOWN;
  bool refreshSwords = false;
  bool refreshPotions = false;

  void findLocation(const std::string &instance);

  void handleFighting();
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
  void handleAborQuest();

  void waitFor(int amount, int deviation = 0);
};