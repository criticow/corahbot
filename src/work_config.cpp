#include "work_config.hpp"

WorkConfig::WorkConfig(const std::string &json)
{
  rapidjson::Document document;
  document.Parse(json.c_str());

  if(!document.HasParseError() && document.IsObject())
  {
    if(document.HasMember("farm")) farm = document["farm"].GetBool();
    if(document.HasMember("buffs")) buffs = document["buffs"].GetBool();
    if(document.HasMember("reboot")) reboot = document["reboot"].GetBool();
    if(document.HasMember("restart")) restart = document["restart"].GetBool();
    if(document.HasMember("pets")) pets = document["pets"].GetBool();
    if(document.HasMember("encounter")) encounter = document["encounter"].GetBool();
    if(document.HasMember("fishing")) fishing = document["fishing"].GetBool();
    if(document.HasMember("combine")) combine = document["combine"].GetBool();
    if(document.HasMember("quests")) quests = document["quests"].GetBool();

    if(document.HasMember("selectedBuffs")) loadVectorFromArray("selectedBuffs", selectedBuffs, document);
    if(document.HasMember("selectedPremiumBuffs")) loadVectorFromArray("selectedPremiumBuffs", selectedPremiumBuffs, document);
    if(document.HasMember("selectedQuests")) loadVectorFromArray("selectedQuests", selectedQuests, document);
    if(document.HasMember("selectedGems")) loadVectorFromArray("selectedGems", selectedGems, document);

    if(document.HasMember("selectedPortal")) selectedPortal = document["selectedPortal"].GetString();
    if(document.HasMember("buffType")) buffType = document["buffType"].GetInt();
    if(document.HasMember("swordsThreshold")) swordsThreshold = document["swordsThreshold"].GetInt();
    if(document.HasMember("refreshMode")) refreshMode = document["refreshMode"].GetInt();
    if(document.HasMember("potionsThreshold")) potionsThreshold = document["potionsThreshold"].GetInt();
    if(document.HasMember("selectedMonster")) selectedMonster = document["selectedMonster"].GetInt();
  }
}

std::string WorkConfig::toJson()
{
  rapidjson::Document document;
  document.SetObject();

  document.AddMember("farm", farm, document.GetAllocator());
  document.AddMember("buffs", buffs, document.GetAllocator());
  document.AddMember("reboot", reboot, document.GetAllocator());
  document.AddMember("restart", restart, document.GetAllocator());
  document.AddMember("quests", quests, document.GetAllocator());
  document.AddMember("pets", pets, document.GetAllocator());
  document.AddMember("encounter", encounter, document.GetAllocator());
  document.AddMember("fishing", fishing, document.GetAllocator());
  document.AddMember("combine", combine, document.GetAllocator());

  document.AddMember("selectedBuffs", createArrayFromVector(document, selectedBuffs), document.GetAllocator());
  document.AddMember("selectedPremiumBuffs", createArrayFromVector(document, selectedPremiumBuffs), document.GetAllocator());
  document.AddMember("selectedQuests", createArrayFromVector(document, selectedQuests), document.GetAllocator());
  document.AddMember("selectedGems", createArrayFromVector(document, selectedGems), document.GetAllocator());

  document.AddMember("refreshMode", refreshMode, document.GetAllocator());
  document.AddMember("selectedPortal", rapidjson::StringRef(selectedPortal.c_str()), document.GetAllocator());
  document.AddMember("buffType", buffType, document.GetAllocator());
  document.AddMember("selectedMonster", selectedMonster, document.GetAllocator());
  document.AddMember("swordsThreshold", swordsThreshold, document.GetAllocator());
  document.AddMember("potionsThreshold", potionsThreshold, document.GetAllocator());

  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

  document.Accept(writer);
  return buffer.GetString();
}

rapidjson::Value WorkConfig::createArrayFromVector(rapidjson::Document &document, std::vector<std::string> &vector)
{
  rapidjson::Value array(rapidjson::kArrayType);

  for (const auto& str : vector) {
    rapidjson::Value jsonStr(rapidjson::StringRef(str.c_str()));
    array.PushBack(jsonStr, document.GetAllocator());
  }

  return array;
}

void WorkConfig::loadVectorFromArray(const std::string &name, std::vector<std::string> &vector, rapidjson::Document &document)
{
  const rapidjson::Value &array = document[name.c_str()].GetArray();

  for(rapidjson::SizeType i = 0; i < array.Size(); i++)
  {
    vector.push_back(array[i].GetString());
  }
}
