#include "store.hpp"

std::unordered_map<std::string, InstanceState> Store::states;
std::unordered_map<std::string, std::mutex> Store::mutexes;
std::unordered_map<std::string, WorkConfig> Store::configs;
std::unordered_map<std::string, Summary> Store::summaries;

std::unordered_map<std::string, Marker> Store::locationMarkers;
std::unordered_map<std::string, Marker> Store::atlasMarkers;

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
  LOGGER_DEBUG("Loading Markers");
  parseMaker("data/markers/locations.json", locationMarkers);
  LOGGER_DEBUG("Location Markers - {}", locationMarkers.size());
  parseMaker("data/markers/atlas.json", atlasMarkers);
  LOGGER_DEBUG("Atlas Markers - {}", atlasMarkers.size());
}

void Store::parseMaker(const char *markerPath, std::unordered_map<std::string, Marker> &markersMap)
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
