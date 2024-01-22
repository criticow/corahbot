#pragma once

#include <glb/glb.hpp>
#include <Windows.h>
#include "emulator.hpp"
#include <rapidjson/document.h>
#include <rapidjson/filereadstream.h>

struct Marker
{
  std::string name;
  std::string location;
  int x;
  int y;
  int width;
  int height;
};

struct SelectableItem
{
  int index;
  bool selected = false;
  std::string name;
  ImVec4 color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
};

struct WorkConfig
{
  bool farm;
  bool combine;
  std::vector<std::string> selectedGems;
  std::vector<SelectableItem> gems{
    {0, false, "Amethyst", ImVec4(249 / 255.0f, 39 / 255.0f, 223 / 255.0f, 1.0f)},
    {1, false, "Amethyst (C)", ImVec4(249 / 255.0f, 39 / 255.0f, 223 / 255.0f, 1.0f)},
    {2, false, "Topaz", ImVec4(246 / 255.0f, 180 / 255.0f, 33 / 255.0f, 2.0f)},
    {3, false, "Topaz (C)", ImVec4(246 / 255.0f, 180 / 255.0f, 33 / 255.0f, 2.0f)},
    {4, false, "Opal", ImVec4(0 / 255.0f, 70 / 255.0f, 250 / 255.0f, 1.0f)},
    {5, false, "Opal (C)", ImVec4(0 / 255.0f, 70 / 255.0f, 250 / 255.0f, 1.0f)}
  };
  int selectedPortal = 0;

  int selectedMonster = 0;
};

class Bot
{
  public:
  Bot(){};
  void run(std::mutex &stateMutex, InstanceState &state, const std::string &instance);
  static void loadMarkers();
  private:
  static std::unordered_map<std::string, Marker> locationMarkers;
  static std::unordered_map<std::string, Marker> potionsCombatMarkers;
  static std::unordered_map<std::string, Marker> potionsHomeMarkers;
  static void parseMaker(const char *markerPath, std::unordered_map<std::string, Marker> &markersMap);
};