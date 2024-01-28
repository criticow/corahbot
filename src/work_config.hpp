#pragma once

#include <glb/glb.hpp>
#include <rapidjson/document.h>
#include <rapidjson/filereadstream.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include "generated_macros.hpp"

class WorkConfig
{
  public:
  bool farm;
  bool combine;
  bool buffs;
  bool quests;
  int refreshMode = 0;
  std::string selectedPortal = CB_POSITION_MAP_RANHAIN;
  int selectedMonster = 0;
  int swordsThreshold = 15;
  int potionsThreshold = 1;

  std::vector<std::string> selectedBuffs;
  std::vector<std::string> selectedQuests;

  WorkConfig(){};
  WorkConfig(const std::string &json);

  std::string toJson();
};