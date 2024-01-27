#include "work_config.hpp"

WorkConfig::WorkConfig(const std::string &json)
{
  rapidjson::Document document;
  document.Parse(json.c_str());

  if(document.IsObject())
  {
    farm = document["farm"].GetBool();
    combine = document["combine"].GetBool();
    // buffs = document["buffs"].GetBool();
    selectedPortal = document["selectedPortal"].GetString();
    swordsThreshold = document["swordsThreshold"].GetInt();
    refreshMode = document["refreshMode"].GetInt();
    potionsThreshold = document["potionsThreshold"].GetInt();
    selectedMonster = document["selectedMonster"].GetInt();
  }
}

std::string WorkConfig::toJson()
{
  rapidjson::Document document;
  document.SetObject();

  document.AddMember("farm", farm, document.GetAllocator());
  document.AddMember("combine", combine, document.GetAllocator());
  // document.AddMember("buffs", buffs, document.GetAllocator());
  document.AddMember("refreshMode", refreshMode, document.GetAllocator());
  document.AddMember("selectedPortal", rapidjson::StringRef(selectedPortal.c_str()), document.GetAllocator());
  document.AddMember("selectedMonster", selectedMonster, document.GetAllocator());
  document.AddMember("swordsThreshold", swordsThreshold, document.GetAllocator());
  document.AddMember("potionsThreshold", potionsThreshold, document.GetAllocator());

  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

  document.Accept(writer);
  return buffer.GetString();
}
