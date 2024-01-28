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
    if(document.HasMember("selectedChips"))
      loadList("selectedChips", selectedChips, document);
    if(document.HasMember("buffs"))
      buffs = document["buffs"].GetBool();
    if(document.HasMember("selectedBuffs"))
      loadList("selectedBuffs", selectedBuffs, document);
    if(document.HasMember("quests"))
      quests = document["quests"].GetBool();
    if(document.HasMember("selectedQuests"))
      loadList("selectedQuests", selectedQuests, document);
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
  document.AddMember("selectedChips", parseArray(document, selectedChips), document.GetAllocator());
  document.AddMember("buffs", buffs, document.GetAllocator());
  document.AddMember("selectedBuffs", parseArray(document, selectedBuffs), document.GetAllocator());
  document.AddMember("quests", quests, document.GetAllocator());
  document.AddMember("selectedQuests", parseArray(document, selectedQuests), document.GetAllocator());
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

rapidjson::Value WorkConfig::parseArray(rapidjson::Document &document, std::vector<std::string> &list)
{
  rapidjson::Value array(rapidjson::kArrayType);

  for (const auto& str : list) {
    rapidjson::Value jsonStr(rapidjson::StringRef(str.c_str()));
    array.PushBack(jsonStr, document.GetAllocator());
  }

  return array;
}

void WorkConfig::loadList(const std::string &name, std::vector<std::string> &list, rapidjson::Document &document)
{
  const rapidjson::Value &array = document[name.c_str()].GetArray();

  for(rapidjson::SizeType i = 0; i < array.Size(); i++)
  {
    list.push_back(array[i].GetString());
  }
}
