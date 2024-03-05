#pragma once

#include <glb/glb.hpp>
#include <Windows.h>
#include "emulator.hpp"
#include "store.hpp"
#include "work_config.hpp"
#include <tesseract/baseapi.h>
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
  int currentEncounterMonster = 0;
  int currentEncounterAttack = 0;
  int refreshMode = 0;
  std::unordered_map<std::string, cv::Mat> snapshots;
  tesseract::TessBaseAPI tess;

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
  void handleEncounter();
  void handleFishing();
  void handlePets();
  void handleNewPetsFound();

  std::string secondsToTime(int seconds);
  void waitFor(int amount, int deviation = 0);
  bool checkEncounterRewards(std::unordered_map<std::string, Marker> &markers);
  std::string textFromImage(cv::Mat &image);
  std::string textFromImage(const std::string &windowTitle, Marker &marker);

  void initTess()
  {
    ASSERT(!tess.Init("data/tess", "eng", tesseract::OEM_DEFAULT), "Error initializing tesseract");
  };
};