#include "bot.hpp"


void Bot::run(const std::string &instance)
{
  int actionsPerSecond = 5;
  WorkConfig &config = Store::configs[instance];
  std::deque<std::string> routines = {"unknown"};
  std::deque<std::string> actions = {"unknown"};

  if(config.farm)
  {
    routines.push_front("farm");
  }

  if(config.combine)
  {
    routines.push_back("combine");
  }

  while(true)
  {
    std::mutex &instanceMutex = Store::mutexes[instance];

    auto start = std::chrono::high_resolution_clock::now();

    InstanceState &state = Store::states[instance];

    // Check if the bot is working, window is open and not minimized
    if(!state.working.load() || !state.open.load() || state.minimized.load())
    {
      state.working.store(false);
      break;
    }

    std::string location = findLocation(instance);

    int swordAmount = -0;
    int potionAmount = -0;

    if(location == "fighting")
    {
      handleFighting(instance, swordAmount, potionAmount);
    }

    int totalSeconds = static_cast<int>(tempo.getTime());

    int hours = totalSeconds / 3600;
    int minutes = (totalSeconds % 3600) / 60;
    int seconds = totalSeconds % 60;

    instanceMutex.lock();

    Summary &summary = Store::summaries[instance];

    summary.time = std::format("{:03d}:{:02d}:{:02d}", hours, minutes, seconds);
    summary.location = location;
    summary.routine = routines[0];
    summary.nextAction = actions[0];
    summary.swords = swordAmount == -1 ? "unknown" : std::to_string(swordAmount);
    summary.potions = potionAmount == -1 ? "unknown" : std::to_string(potionAmount);

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    int hasTimeLeft = duration.count() < 1000 / actionsPerSecond;

    summary.ms = std::to_string(duration.count());
    summary.actionsPerSecond = hasTimeLeft ? std::to_string(actionsPerSecond) : std::to_string(1000 / duration.count());

    instanceMutex.unlock();

    if(hasTimeLeft)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds((1000 / actionsPerSecond) - duration.count()));
    }
  }
}

std::string Bot::findLocation(const std::string &instance)
{
  std::string location = "unknown";

  for(auto &[name, marker] : Store::locationMarkers)
  {
    if(Emulator::compareImages(instance, marker))
    {
      location = marker.location;
      break;
    }
  }

  return location;
}

void Bot::handleFighting(const std::string &instance, int &swords, int &potions)
{
  std::pair<bool, glm::ivec4> swordsRes = Emulator::find(instance, Store::atlasMarkers["sword_btn"], "sword_atlas");
  std::pair<bool, glm::ivec4> potionRes = Emulator::find(instance, Store::atlasMarkers["potion_combat"], "potion_atlas");

  if(swordsRes.first)
  {
    swords = Store::swordsMap[std::format("{}{}", swordsRes.second.x, swordsRes.second.y)];
  }

  if(potionRes.first)
  {
    potions = Store::potionsMap[std::format("{}{}", potionRes.second.x, potionRes.second.y)];
  }
}
