#include "bot.hpp"

std::unordered_map<std::string, Marker> Bot::locationMarkers;
std::unordered_map<std::string, Marker> Bot::potionsHomeMarkers;
std::unordered_map<std::string, Marker> Bot::potionsCombatMarkers;

void Bot::run(std::mutex &stateMutex, InstanceState &state, const std::string &instance)
{
  std::string windowName = std::format("{} - {}", instance, Random::UUID());

  while(true)
  {
    std::lock_guard<std::mutex> lock(stateMutex);

    if(!state.working.load() || !state.open.load() || state.minimized.load())
    {
      state.working.store(false);
      break;
    }

    // Find in which part of the game we are
    // bool combat = Emulator::compareImages(instance, locationMarkers["afk_btn"]);
    // bool home = Emulator::compareImages(instance, locationMarkers["start_adv_btn"]);

    // std::pair<bool, glm::ivec4> potionState(false, glm::ivec4(-1));

    // if(combat)
    // {
    //   // Check if the health is less than 10 in combat scene
    //   potionState = Emulator::find(instance, locationMarkers["potion_combat"], "potions_combat");
    // }

    // if(home)
    // {
    //   // Check if the health is less than 10 in home scene
    //   potionState = Emulator::find(instance, locationMarkers["potion_home"], "potions_home");
    //   // LOGGER_DEBUG("At home, potions bellow 10: {}", potionState.first);
    // }
  }


  LOGGER_DEBUG("Thread of instance {} ended", instance);

  if(FindWindow(nullptr, windowName.c_str()))
  {
    cv::destroyWindow(windowName);
  }
}

void Bot::loadMarkers()
{
  LOGGER_DEBUG("Loading Markers");
  parseMaker("data/markers/locations.json", locationMarkers);
  LOGGER_DEBUG("Locations - {}", locationMarkers.size());
  // parseMaker("data/markers/potions_home.json", potionsHomeMarkers);
  // LOGGER_DEBUG("Potions Home - {}", potionsHomeMarkers.size());
  // parseMaker("data/markers/potions_combat.json", potionsCombatMarkers);
  // LOGGER_DEBUG("Potions Combat - {}", potionsCombatMarkers.size());
}

void Bot::parseMaker(const char *markerPath, std::unordered_map<std::string, Marker> &markersMap)
{
  std::string json = readFile(markerPath);

  rapidjson::Document document;
  document.Parse(json.c_str());

  ASSERT(document.IsObject(), "Error parsing {} JSON", markerPath);

  std::string version = document["version"].GetString();
  const rapidjson::Value &markersArray = document["markers"].GetArray();

  for(rapidjson::SizeType i = 0; i < markersArray.Size(); i++)
  {
    const rapidjson::Value &marker = markersArray[i];

    markersMap[marker["name"].GetString()] = Marker(
      marker["name"].GetString(),
      marker["location"].GetString(),
      marker["x"].GetInt(),
      marker["y"].GetInt(),
      marker["width"].GetInt(),
      marker["height"].GetInt()
    );
  }
}
