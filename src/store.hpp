#pragma once

#include <glb/glb.hpp>
#include <rapidjson/document.h>
#include <rapidjson/filereadstream.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include "generated_macros.hpp"
#include "work_config.hpp"

struct Monster
{
  std::string name;
  std::string displayName;
};

struct Marker
{
  std::string name;
  std::string location;
  int x;
  int y;
  int width;
  int height;

  std::string toJson()
  {
    rapidjson::Document document;
    document.SetObject();

    document.AddMember("name", rapidjson::StringRef(name.c_str()), document.GetAllocator());
    document.AddMember("location", rapidjson::StringRef(location.c_str()), document.GetAllocator());
    document.AddMember("x", x, document.GetAllocator());
    document.AddMember("y", y, document.GetAllocator());
    document.AddMember("width", width, document.GetAllocator());
    document.AddMember("height", height, document.GetAllocator());

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

    document.Accept(writer);
    return buffer.GetString();
  }
};

struct InstanceState
{
  std::atomic<bool> open = true;
  std::atomic<bool> working = false;
  std::atomic<bool> minimized = false;

  // Default constructor
  InstanceState() = default;

  // Custom copy assignment operator
  InstanceState& operator=(const InstanceState& other) {
    if (this != &other) {
      open.store(other.open.load());
      working.store(other.working.load());
      minimized.store(other.minimized.load());
    }
    return *this;
  }

  // Custom constructor
  InstanceState(bool open, bool working, bool minimized) : open(open), working(working), minimized(minimized) {}
};

struct Selectable
{
  std::string name;
  std::string displayName;
};

struct Summary
{
  std::string time = "000:00:00";
  std::string ms = "unknown";
  std::string actionsPerSecond = "unknown";
  std::string routine = "unknown";
  std::string location = "unknown";
  std::string nextAction = "unknown";
  std::string swords = "unknown";
  std::string potions = "unknown";
  std::string crashs = "0";
  std::string questsDone = "0";
};

class Store
{
  public:
  static std::unordered_map<std::string, InstanceState> states;
  static std::unordered_map<std::string, std::mutex> mutexes;
  static std::unordered_map<std::string, WorkConfig> configs;
  static std::unordered_map<std::string, Summary> summaries;

  static std::unordered_map<std::string, Marker> locationMarkers;
  static std::unordered_map<std::string, Marker> positionMarkers;
  static std::unordered_map<std::string, std::unordered_map<std::string, Marker>> markers;

  static std::unordered_map<std::string, std::string> portals;
  static std::unordered_map<std::string, std::vector<Monster>> monsters;
  static std::vector<Selectable> foods;
  static std::vector<Selectable> potions;
  static std::vector<Selectable> scrolls;

  static std::vector<std::string> refreshModes;

  static std::unordered_map<std::string, int> swordsMap;
  static std::unordered_map<std::string, int> potionsMap;

  static void loadMarkers();

  private:
  static void parseMarker(const std::string &markerPath);
};