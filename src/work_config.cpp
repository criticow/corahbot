#include "work_config.hpp"

WorkConfig::WorkConfig(const std::string &json)
{
  rapidjson::Document document;
  document.Parse(json.c_str());

  if(!document.HasParseError() && document.IsObject())
  {
    if(document.HasMember("farm"))
      farm = document["farm"].GetBool();
    if(document.HasMember("combine"))
      combine = document["combine"].GetBool();
    if(document.HasMember("buffs"))
      buffs = document["buffs"].GetBool();
    if(document.HasMember("selectedBuffs"))
    {
      const rapidjson::Value &array = document["selectedBuffs"].GetArray();

      for(rapidjson::SizeType i = 0; i < array.Size(); i++)
      {
        selectedBuffs.push_back(array[i].GetString());
      }
    }
    if(document.HasMember("selectedPortal"))
      selectedPortal = document["selectedPortal"].GetString();
    if(document.HasMember("swordsThreshold"))
      swordsThreshold = document["swordsThreshold"].GetInt();
    if(document.HasMember("refreshMode"))
      refreshMode = document["refreshMode"].GetInt();
    if(document.HasMember("potionsThreshold"))
      potionsThreshold = document["potionsThreshold"].GetInt();
    if(document.HasMember("selectedMonster"))
      selectedMonster = document["selectedMonster"].GetInt();
  }
}

std::string WorkConfig::toJson()
{
  rapidjson::Document document;
  document.SetObject();

  document.AddMember("farm", farm, document.GetAllocator());
  document.AddMember("combine", combine, document.GetAllocator());
  document.AddMember("buffs", buffs, document.GetAllocator());

  rapidjson::Value array(rapidjson::kArrayType);
  for (const auto& str : selectedBuffs) {
    rapidjson::Value jsonStr(rapidjson::StringRef(str.c_str()));
    array.PushBack(jsonStr, document.GetAllocator());
  }

  document.AddMember("selectedBuffs", array, document.GetAllocator());
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
