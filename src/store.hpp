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
  std::atomic<bool> working = false;

  // Default constructor
  InstanceState() = default;

  // Custom copy assignment operator
  InstanceState& operator=(const InstanceState& other) {
    if (this != &other) {
      working.store(other.working.load());
    }
    return *this;
  }

  // Custom constructor
  InstanceState(bool working) : working(working) {}
};

struct Selectable
{
  std::string name;
  std::string displayName;
};

struct Summary
{
  std::string time = "00:00:00";
  std::string ms = "unknown";
  std::string actionsPerSecond = "unknown";
  std::string routine = "unknown";
  std::string location = "unknown";
  std::string nextAction = "unknown";
  std::string refreshSwords = "false";
  std::string refreshPotions = "false";
  std::string crashs = "0";
  std::string questsDone = "0";
  std::string encounterCooldown = "00:00:00";
  std::string fishingCooldown = "00:00:00";
  std::string nextRefreshMode = "unknown";
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

  static void loadMarkers();

  private:
  static void parseMarker(const std::string &markerPath);
};