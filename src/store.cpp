#include "store.hpp"

std::unordered_map<std::string, InstanceState> Store::states;
std::unordered_map<std::string, std::mutex> Store::mutexes;
std::unordered_map<std::string, WorkConfig> Store::configs;
std::unordered_map<std::string, Summary> Store::summaries;

std::unordered_map<std::string, Marker> Store::locationMarkers;
std::unordered_map<std::string, Marker> Store::positionMarkers;

std::unordered_map<std::string, std::unordered_map<std::string, Marker>> Store::markers;

std::vector<Selectable> Store::foods{
  {"food_fish_pie", "Ranhain Fish Pie"},
  {"food_fish_pie_perfect", "Perfect Ranhain Fish Pie (P)"},
  {"food_fish_soup", "Jakal's Spicy Fish Soup"},
  {"food_fish_soup_perfect", "Jakal's Spicy Fish Soup (P)"},
};

std::vector<Selectable> Store::potions{
  {"potion_double_exp", "Double EXP Potion"},
  {"potion_triple_exp", "Triple EXP Potion"},
};

std::vector<Selectable> Store::scrolls{
  {CB_POSITION_QUESTS_SCROLL_ANCIENT, "Ancient Scroll"},
  {CB_POSITION_QUESTS_SCROLL_DEMONIAC, "Demoniac Scroll"},
  {CB_POSITION_QUESTS_SCROLL_ARCANE, "Arcane Scroll"},
  {CB_POSITION_QUESTS_SCROLL_SHADOW, "Shadow Scroll"},
};

std::unordered_map<std::string, std::string> Store::portals{
  { CB_POSITION_MAP_RANHAIN, "Ranhain"},
  { CB_POSITION_MAP_DEKDUN, "Dekdun"},
  { CB_POSITION_MAP_VULCARDI, "Vulcardi"},
  { CB_POSITION_MAP_ICEROOST, "Iceroost"}
  // "Forilon",
  // "Airos",
  // "Mitron"
};

std::vector<std::string> Store::refreshModes{CB_REFRESH_MODE_LOGOUT, CB_REFRESH_MODE_CLOSE};

std::unordered_map<std::string, std::vector<Monster>> Store::monsters{
  {
    CB_POSITION_MAP_RANHAIN,
    {
      { CB_POSITION_MAP_BEAR, "Bear (Lv.3)" },
      { CB_POSITION_MAP_GIANT_ANT, "Giant Ant (Lv.8)" },
      { CB_POSITION_MAP_CHINAVIA, "Chinavia (Lv.15)" },
      { CB_POSITION_MAP_ENRAGED_TURTLE, "Enraged Turtle (Lv.23)" },
      { CB_POSITION_MAP_GOBLIN_THIEF, "Goblin Thief (Lv.26)" },
      { CB_POSITION_MAP_SCAVANGER, "Scavanger (Lv.32)" },
      { CB_POSITION_MAP_AMPHNALY, "Amphnaly (Lv.40)" },
      { CB_POSITION_MAP_GOBLIN_CHIEF, "Goblin Chief (Lv.50)" }
    }
  },
  {
    CB_POSITION_MAP_DEKDUN,
    {
      { CB_POSITION_MAP_SKELETON_WORKER, "Skeleton Worker (Lv.60)" },
      { CB_POSITION_MAP_LOST_AMUMMY, "Lost Amummy (Lv.70)" },
      { CB_POSITION_MAP_SKELETON_CAPTAIN, "Skeleton Captain (Lv.80)" },
      { CB_POSITION_MAP_SKELETON_WIZARD, "Skeleton Wizard (Lv.85)" },
      { CB_POSITION_MAP_SKELETON_THIEF, "Skeleton Thief (Lv.90)" },
      { CB_POSITION_MAP_THIA, "Thia (Lv.100)" },
      { CB_POSITION_MAP_SKELETON_KING, "Skeleton King (Lv.105)" }
    }
  },
  {
    CB_POSITION_MAP_VULCARDI,
    {
      { CB_POSITION_MAP_BALOR, "Balor (Lv.115)"},
      { CB_POSITION_MAP_GARZON, "Garzon (Lv.130)"},
      { CB_POSITION_MAP_PRO_GIANT, "Pro Giant (Lv.150)"},
      { CB_POSITION_MAP_JARGOL, "Jargol (Lv.170)"},
      { CB_POSITION_MAP_DEVIL_GIANT, "Devil Giant (Lv.190)"}
    }
  },
  {
    CB_POSITION_MAP_ICEROOST,
    {
      { CB_POSITION_MAP_RINCH, "Rinch (Lv.120)"},
      { CB_POSITION_MAP_FROST_SERPENT, "Frost Serpent (Lv.150)"}
    }
  }
};

void Store::loadMarkers()
{
  LOGGER_DEBUG("==== Started Loading Markers ====");
  std::filesystem::path markersDir("data/markers");
  for(auto &entry : std::filesystem::directory_iterator(markersDir))
  {
    parseMarker(entry.path().string());
  }
  LOGGER_DEBUG("==== Finished Loading Markers ====");
}

void Store::parseMarker(const std::string &markerPath)
{
  std::unordered_map<std::string, Marker> markersMap;
  std::string json = readFile(markerPath);

  rapidjson::Document document;
  document.Parse(json.c_str());

  ASSERT(document.IsObject(), "Error parsing {} JSON", markerPath);

  std::string version = document["version"].GetString();
  const rapidjson::Value &markersArray = document["markers"].GetArray();
  const rapidjson::Value &identifier = document["identifier"].GetObject();

  Marker marker{
    identifier["name"].GetString(),
    identifier["location"].GetString(),
    identifier["x"].GetInt(),
    identifier["y"].GetInt(),
    identifier["width"].GetInt(),
    identifier["height"].GetInt()
  };

  markersMap[marker.name] = marker;

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

  markers[identifier["name"].GetString()] = markersMap;

  LOGGER_DEBUG("{} - {}", marker.name, markersMap.size());
}
