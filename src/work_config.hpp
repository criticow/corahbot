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
  bool farm = false;;
  bool buffs = false;
  int buffType = 0;
  bool quests = false;
  bool reboot = false;
  bool restart = false;
  bool pets = true;
  bool encounter = false;
  bool fishing = false;
  bool combine = false;

  int refreshMode = 1;
  std::string selectedPortal = CB_POSITION_MAP_RANHAIN;
  int selectedMonster = 0;
  int swordsThreshold = 15;
  int potionsThreshold = 1;

  std::vector<std::string> selectedBuffs;
  std::vector<std::string> selectedPremiumBuffs;
  std::vector<std::string> selectedQuests;
  std::vector<std::string> selectedGems;

  WorkConfig(){};
  WorkConfig(const std::string &json);

  std::string toJson();
  private:
  rapidjson::Value createArrayFromVector(rapidjson::Document &document, std::vector<std::string> &vector);
  void loadVectorFromArray(const std::string &name, std::vector<std::string> &vector, rapidjson::Document &document);
};