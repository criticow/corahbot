#pragma once

#include <glb/glb.hpp>
#include <rapidjson/document.h>
#include <rapidjson/filereadstream.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

class WorkConfig
{
  public:
  bool farm;
  bool combine;
  bool buffs;
  int refreshMode = 0;
  std::string selectedPortal = "dekdun";
  int selectedMonster = 5;
  int swordsThreshold = 15;
  int potionsThreshold = 3;

  std::vector<std::string> selectedBuffs;

  WorkConfig(){};
  WorkConfig(const std::string &json);

  std::string toJson();
};