#include "store.hpp"

std::unordered_map<std::string, InstanceState> Store::states;
std::unordered_map<std::string, std::mutex> Store::mutexes;
std::unordered_map<std::string, WorkConfig> Store::configs;
std::unordered_map<std::string, Summary> Store::summaries;

std::unordered_map<std::string, Marker> Store::locationMarkers;
std::unordered_map<std::string, Marker> Store::positionMarkers;

std::unordered_map<std::string, std::unordered_map<std::string, Marker>> Store::markers;

std::unordered_map<std::string, std::string> Store::portals{
  { "ranhain", "Ranhain"},
  { "dekdun", "Dekdun"},
  // "Vulcardi",
  // "Iceroost",
  // "Forilon",
  // "Airos",
  // "Mitron"
};

std::vector<std::string> Store::refreshModes{CB_REFRESH_MODE_LOGOUT, CB_REFRESH_MODE_CLOSE};

std::unordered_map<std::string, std::vector<Monster>> Store::monsters{
  {
    "ranhain",
    {
      { "bear", "Bear (Lv.3)" },
      { "giant_ant", "Giant Ant (Lv.8)" },
      { "chinavia", "Chinavia (Lv.15)" },
      { "enraged_turtle", "Enraged Turtle (Lv.23)" },
      { "goblin_thief", "Goblin Thief (Lv.26)" },
      { "scavanger", "Scavanger (Lv.32)" },
      { "amphnaly", "Amphnaly (Lv.40)" },
      { "goblin_chief", "Goblin Chief (Lv.50)" }
    }
  },
  {
    "dekdun",
    {
      { "skeleton_worker", "Skeleton Worker (Lv.60)" },
      { "lost_amummy", "Lost Amummy (Lv.70)" },
      { "skeleton_captain", "Skeleton Captain (Lv.80)" },
      { "skeleton_wizard", "Skeleton Wizard (Lv.85)" },
      { "skeleton_thief", "Skeleton Thief (Lv.90)" },
      { "thia", "Thia (Lv.100)" },
      { "skeleton_king", "Skeleton King (Lv.105)" }
    }
  }
};

std::unordered_map<std::string, int> Store::swordsMap{
  {"00", -1},
  {"240", 0},
  {"480", 1},
  {"720", 2},
  {"960", 3},
  {"1200", 4},
  {"1440", 5},
  {"1680", 6},
  {"1920", 7},
  {"2160", 8},
  {"2400", 9},
  {"2640", 10},
  {"2880", 11},
  {"3120", 12},
  {"3360", 13},
  {"3600", 14},
  {"3840", 15},
  {"4080", 16},
  {"4320", 17},
  {"4560", 18},
  {"4800", 19},
  {"5040", 20},
  {"5280", 21},
  {"5520", 22},
  {"5760", 23},
  {"6000", 24},
  {"6240", 25},
  {"6480", 26},
  {"6720", 27},
  {"6960", 28},
  {"7200", 29},
  {"7440", 30},
  {"7680", 31},
  {"7920", 32},
  {"8160", 33},
  {"8400", 34},
  {"8640", 35},
  {"8880", 36},
  {"9120", 37},
  {"9360", 38},
  {"9600", 39},
  {"9840", 40},
  {"10080", 41},
  {"10320", 42},
  {"10560", 43},
  {"10800", 44},
  {"11040", 45},
  {"11280", 46},
  {"11520", 47},
  {"11760", 48},
  {"12000", 49},
  {"12240", 50},
};

std::unordered_map<std::string, int> Store::potionsMap{
  {"00", 0},
  {"120", 1},
  {"240", 2},
  {"360", 3},
  {"480", 4},
  {"600", 5},
  {"720", 6},
  {"840", 7},
  {"960", 8},
  {"1080", 9},
  {"1200", 10},
  {"1320", 11},
  {"1440", 12},
  {"1560", 13},
  {"1680", 14},
  {"1800", 15},
  {"1920", 16},
  {"2040", 17},
  {"2160", 18},
  {"2280", 19},
  {"2400", 20},
  {"2520", 21},
  {"2640", 22},
  {"2760", 23},
  {"2880", 24},
  {"3000", 25},
  {"3120", 26},
  {"3240", 27},
  {"3360", 28},
  {"3480", 29},
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
