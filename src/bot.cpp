#include "bot.hpp"


void Bot::run(const std::string &instance)
{
  int actionsPerSecond = 5;
  WorkConfig &config = Store::configs[instance];

  std::string currentRoutine = BOT_ROUTINE_NONE;
  std::string currentAction = BOT_ACTION_NONE;

  if(config.farm)
  {
    currentRoutine = BOT_ROUTINE_FARM;
  }

  if(config.combine && currentRoutine == BOT_ROUTINE_NONE)
  {
    currentRoutine = BOT_ROUTINE_COMBINE;
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

    int swordAmount = 9999;
    int potionAmount = 9999;

    if(location == BOT_LOCATION_FIGHTING)
    {
      if(currentRoutine == BOT_ROUTINE_FARM && currentAction == BOT_ACTION_NONE)
      {
        currentAction = BOT_ACTION_REFRESH_SWORDS;
      }

      handleFighting(instance, currentAction, swordAmount, potionAmount);
    }

    if(location == BOT_LOCATION_MENU_RIGHT)
    {
      handleMenuLeft(instance, currentAction);
    }

    if(location == BOT_LOCATION_LOGIN)
    {
      handleLogin(instance, currentAction);
    }

    if(location == BOT_LOCATION_HOME)
    {
      handleHome(instance, currentAction);
    }

    if(location == BOT_LOCATION_REFILL_MENU)
    {
      handleRefillMenu(instance, currentAction);
    }

    if(location == BOT_LOCATION_MAP)
    {
      handleMap(instance, currentAction);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    int hasTimeLeft = duration.count() < 1000 / actionsPerSecond;

#ifndef NDEBUG
    int totalSeconds = static_cast<int>(tempo.getTime());

    int hours = totalSeconds / 3600;
    int minutes = (totalSeconds % 3600) / 60;
    int seconds = totalSeconds % 60;

    instanceMutex.lock();

    Summary &summary = Store::summaries[instance];

    summary.time = std::format("{:03d}:{:02d}:{:02d}", hours, minutes, seconds);
    summary.location = location;
    summary.routine = currentRoutine;
    summary.nextAction = currentAction;
    summary.swords = swordAmount == 9999 ? "unknown" : std::to_string(swordAmount);
    summary.potions = potionAmount == 9999 ? "unknown" : std::to_string(potionAmount);


    summary.ms = std::to_string(duration.count());
    summary.actionsPerSecond = hasTimeLeft ? std::to_string(actionsPerSecond) : std::to_string(1000 / duration.count());

    instanceMutex.unlock();
#endif

    // This is so the bot takes n actionsPerSecond
    if(hasTimeLeft)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds((1000 / actionsPerSecond) - duration.count()));
    }
  }
}

std::string Bot::findLocation(const std::string &instance)
{
  std::string location = BOT_LOCATION_UNKNOWN;

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

void Bot::handleFighting(const std::string &instance, std::string &currentAction, int &swords, int &potions)
{
  // START POTION ACTIONS
  std::pair<bool, glm::ivec4> potionRes = Emulator::find(instance, Store::positionMarkers["potion_combat"], "potion_atlas");

  // If potions are not visible there is nothing more to do here
  if(!potionRes.first)
  {
    return;
  }

  WorkConfig &config = Store::configs[instance];
  potions = Store::potionsMap[std::format("{}{}", potionRes.second.x, potionRes.second.y)];

  if(potions > config.potionsThreshold && currentAction == BOT_ACTION_REFRESH_POTIONS)
  {
    currentAction = BOT_ACTION_REFRESH_SWORDS;
  }

  if(potions < config.potionsThreshold)
  {
    // Open the options menu to start the refresh
    currentAction = BOT_ACTION_REFRESH_POTIONS;
    Emulator::killappall(instance);
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    Emulator::runapp(instance, CORAH_PACKAGE_NAME);
  }
  // END POTIONS ACTIONS

  // START SWORDS ACTIONS
  Marker &swordBtn = Store::positionMarkers["sword_btn"];
  std::pair<bool, glm::ivec4> swordsRes = Emulator::find(instance, swordBtn, "sword_atlas");
  if(!swordsRes.first)
  {
    return;
  }
  
  swords = Store::swordsMap[std::format("{}{}", swordsRes.second.x, swordsRes.second.y)];

  if(swords < config.swordsThreshold && currentAction == BOT_ACTION_REFRESH_SWORDS)
  {
    // If the current swords is below the threshold the swords should be reset
    Emulator::click(instance, swordBtn);
    // Wait 800ms after clicking
    std::this_thread::sleep_for(std::chrono::milliseconds(800));
  }
  // END SWORDS ACTIONS
}

void Bot::handleMenuLeft(const std::string & instance, std::string & currentAction)
{
  if(currentAction == BOT_ACTION_REFRESH_POTIONS)
  {
    // Should close
  }
}

void Bot::handleLogin(const std::string &instance, std::string &currentAction)
{
  bool res = Emulator::compareImages(instance, Store::locationMarkers["login"]);

  if(res)
  {
    Marker &loginBtn = Store::positionMarkers["login_btn"];
    Emulator::click(instance, loginBtn);
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
  }
}

void Bot::handleHome(const std::string &instance, std::string &currentAction)
{
  bool shoudlRefill = Emulator::compareImages(instance, Store::positionMarkers["potion_home"]);

  if(!shoudlRefill)
  {
    Marker &refilBtn = Store::positionMarkers["refill_btn"];

    if(Emulator::compareImages(instance, refilBtn))
    {
      Emulator::click(instance, refilBtn);
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
  }

  Marker &homeStartBtn = Store::positionMarkers["home_start_btn"];
  if(Emulator::compareImages(instance, homeStartBtn))
  {
    Emulator::click(instance, homeStartBtn);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }
}

void Bot::handleRefillMenu(const std::string & instance, std::string & currentAction)
{
  Marker &maxBtnActive = Store::positionMarkers["max_btn_active"];
  if(Emulator::compareImages(instance, maxBtnActive))
  {
    Emulator::click(instance, maxBtnActive);
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
  }

  Marker &refilBtnActive = Store::positionMarkers["refill_btn_active"];
  if(Emulator::compareImages(instance, refilBtnActive))
  {
    Emulator::click(instance, refilBtnActive);
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
  }

  Marker &maxBtnInactive = Store::positionMarkers["max_btn_inactive"];
  if(Emulator::compareImages(instance, maxBtnInactive))
  {
    currentAction = BOT_ACTION_REFRESH_SWORDS;
    Emulator::click(instance, Store::positionMarkers["refill_close_btn"]);
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
  }
}

void Bot::handleMap(const std::string &instance, std::string &currentAction)
{
  WorkConfig &config = Store::configs[instance];
  Monster &monster = Store::monsters[config.selectedPortal][config.selectedMonster];

  // Check if it is not in the correct portal, to change location
  if(!Emulator::compareImages(instance, Store::positionMarkers["map_" + config.selectedPortal]))
  {
    LOGGER_DEBUG("It is not in the correct portal");
  }

  Marker &monsterMarker = Store::positionMarkers["monster_" + monster.name];
  if(Emulator::compareImages(instance, monsterMarker))
  {
    Emulator::click(instance, monsterMarker);
    std::this_thread::sleep_for(std::chrono::milliseconds(3000));
  }
}
