#include "bot.hpp"


void Bot::run(const std::string &instance)
{
  int actionsPerSecond = 5;
  WorkConfig &config = Store::configs[instance];

  std::string currentRoutine = CB_ROUTINE_NONE;
  std::string currentAction = CB_ROUTINE_NONE;

  if(config.farm)
  {
    currentRoutine = CB_ROUTINE_FARM;
  }

  if(config.combine && currentRoutine == CB_ROUTINE_NONE)
  {
    currentRoutine = CB_ROUTINE_COMBINE;
  }

  while(true)
  {
    std::mutex &instanceMutex = Store::mutexes[instance];

    auto start = std::chrono::high_resolution_clock::now();

    InstanceState &state = Store::states[instance];

    {
      HWND hwnd = FindWindow(nullptr, instance.c_str());
      InstanceState &state = Store::states[instance];

      // Window not found, instance is closed
      if(!hwnd)
      {
        state.open.store(false);
        state.working.store(false);
        break;
      }

      state.open.store(true);

      RECT rect;
      GetClientRect(hwnd, &rect);

      int width = rect.right - rect.left;
      int height = rect.bottom - rect.top;

      // Window is minimized, cant send message events
      if(width == 0 || height == 0)
      {
        state.minimized.store(true);
        state.working.store(false);
        break;
      }

      state.minimized.store(false);
    }

    // Check if the bot is working, window is open and not minimized
    if(!state.working.load() || !state.open.load() || state.minimized.load())
    {
      state.working.store(false);
      break;
    }

    std::string location = findLocation(instance);

    int swordAmount = 9999;
    int potionAmount = 9999;

    if(location == CB_LOCATION_FIGHTING_FIGHTING)
    {
      if(currentRoutine == CB_ROUTINE_FARM && currentAction == CB_ACTION_NONE)
      {
        currentAction = CB_ACTION_REFRESH_SWORDS;
      }

      handleFighting(instance, currentRoutine, currentAction, swordAmount, potionAmount);
    }

    if(location == CB_LOCATION_GEAR_GEAR)
    {
      handleGear(instance, currentRoutine, currentAction);
    }

    if(location == CB_LOCATION_LOGIN_LOGIN)
    {
      handleLogin(instance, currentRoutine, currentAction);
    }

    if(location == CB_LOCATION_HOME_HOME)
    {
      handleHome(instance, currentRoutine, currentAction);
    }

    if(location == CB_LOCATION_REFILL_REFILL)
    {
      handleRefill(instance, currentRoutine, currentAction);
    }

    if(location == CB_LOCATION_MAP_MAP)
    {
      handleMap(instance, currentRoutine, currentAction);
    }

    if(location == CB_LOCATION_DISCONNECTED_DISCONNECTED)
    {
      Emulator::click(instance, Store::markers[CB_LOCATION_DISCONNECTED_DISCONNECTED][CB_LOCATION_DISCONNECTED_DISCONNECTED]);
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    if(location == CB_LOCATION_APP_CLOSED_APP_CLOSED)
    {
      Emulator::click(instance, Store::markers[CB_LOCATION_APP_CLOSED_APP_CLOSED][CB_LOCATION_APP_CLOSED_APP_CLOSED]);
      std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    }

    if(location == CB_LOCATION_APP_CRASHED_APP_CRASHED)
    {
      Emulator::click(instance, Store::markers[CB_LOCATION_LOGIN_LOGIN][CB_POSITION_LOGIN_LOGIN_BTN]);
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
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
  std::string location = CB_LOCATION_UNKNOWN;

  for(auto &[name, markers] : Store::markers)
  {
    if(Emulator::compareImages(instance, markers[name]))
    {
      location = markers[name].location;
      break;
    }
  }

  return location;
}

void Bot::handleFighting(const std::string &instance, std::string &currentRoutine, std::string &currentAction, int &swords, int &potions)
{
  std::unordered_map<std::string, Marker> &markers = Store::markers[CB_LOCATION_FIGHTING_FIGHTING];

  // START POTION ACTIONS
  std::pair<bool, glm::ivec4> potionRes = Emulator::find(instance, markers[CB_POSITION_FIGHTING_POTIONS], "atlas/potions");

  // If potions are not visible there is nothing more to do here
  if(!potionRes.first)
  {
    return;
  }

  WorkConfig &config = Store::configs[instance];
  potions = Store::potionsMap[std::format("{}{}", potionRes.second.x, potionRes.second.y)];

  if(currentRoutine == CB_ROUTINE_FARM && potions > config.potionsThreshold && currentAction == CB_ACTION_REFRESH_POTIONS)
  {
    currentAction = CB_ACTION_REFRESH_SWORDS;
  }

  if(currentRoutine == CB_ROUTINE_FARM && potions < config.potionsThreshold)
  {
    // Open the options menu to start the refresh
    currentAction = CB_ACTION_REFRESH_POTIONS;
    Emulator::killapp(instance, CORAH_PACKAGE_NAME);
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
  }
  // END POTIONS ACTIONS

  // START SWORDS ACTIONS
  std::pair<bool, glm::ivec4> swordsRes = Emulator::find(instance, markers[CB_POSITION_FIGHTING_SWORDS], "atlas/swords");
  if(!swordsRes.first)
  {
    return;
  }
  
  swords = Store::swordsMap[std::format("{}{}", swordsRes.second.x, swordsRes.second.y)];

  if(currentRoutine == CB_ROUTINE_FARM && swords < config.swordsThreshold && currentAction == CB_ACTION_REFRESH_SWORDS)
  {
    // If the current swords is below the threshold the swords should be reset
    Emulator::click(instance, markers[CB_POSITION_FIGHTING_SWORDS]);
    std::this_thread::sleep_for(std::chrono::milliseconds(3000));
  }
  // END SWORDS ACTIONS
}

void Bot::handleGear(const std::string &instance, std::string &currentRoutine, std::string &currentAction)
{
  if(currentAction == CB_ACTION_REFRESH_POTIONS)
  {
    // Should close
  }
}

void Bot::handleLogin(const std::string &instance, std::string &currentRoutine, std::string &currentAction)
{
  std::unordered_map<std::string, Marker> &markers = Store::markers[CB_LOCATION_LOGIN_LOGIN];
  bool res = Emulator::compareImages(instance, markers[CB_LOCATION_LOGIN_LOGIN]);

  if(currentRoutine == CB_ROUTINE_FARM && res)
  {
    Emulator::click(instance, markers[CB_POSITION_LOGIN_LOGIN_BTN]);
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
  }
}

void Bot::handleHome(const std::string &instance, std::string &currentRoutine, std::string &currentAction)
{
  std::unordered_map<std::string, Marker> &markers = Store::markers[CB_LOCATION_HOME_HOME];
  bool shoudlRefill = Emulator::compareImages(instance, markers[CB_POSITION_HOME_POTION]);

  if(currentRoutine == CB_ROUTINE_FARM && !shoudlRefill)
  {
    if(Emulator::compareImages(instance, markers[CB_POSITION_HOME_REFILL_BTN]))
    {
      Emulator::click(instance, markers[CB_POSITION_HOME_REFILL_BTN]);
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    return;
  }

  if(currentRoutine == CB_ROUTINE_FARM && Emulator::compareImages(instance, markers[CB_POSITION_HOME_START_BTN]))
  {
    Emulator::click(instance, markers[CB_POSITION_HOME_START_BTN]);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }
}

void Bot::handleRefill(const std::string &instance, std::string &currentRoutine, std::string &currentAction)
{
  std::unordered_map<std::string, Marker> &markers = Store::markers[CB_LOCATION_REFILL_REFILL];

  if(currentRoutine == CB_ROUTINE_FARM && Emulator::compareImages(instance, markers[CB_POSITION_REFILL_MAX_BTN_ACTIVE]))
  {
    Emulator::click(instance, markers[CB_POSITION_REFILL_MAX_BTN_ACTIVE]);
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
  }

  if(currentRoutine == CB_ROUTINE_FARM && Emulator::compareImages(instance, markers[CB_POSITION_REFILL_REFILL_BTN_ACTIVE]))
  {
    Emulator::click(instance, markers[CB_POSITION_REFILL_REFILL_BTN_ACTIVE]);
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
  }

  if(currentRoutine == CB_ROUTINE_FARM && Emulator::compareImages(instance, markers[CB_POSITION_REFILL_MAX_BTN_INACTIVE]))
  {
    currentAction = CB_ACTION_REFRESH_SWORDS;
    Emulator::click(instance, markers[CB_POSITION_REFILL_MAX_BTN_INACTIVE]);
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
  }
}

void Bot::handleMap(const std::string &instance, std::string &currentRoutine, std::string &currentAction)
{
  std::unordered_map<std::string, Marker> &markers = Store::markers[CB_LOCATION_MAP_MAP];

  WorkConfig &config = Store::configs[instance];
  Monster &monster = Store::monsters[config.selectedPortal][config.selectedMonster];

  // Check if it is not in the correct portal, to change location
  if(currentRoutine == CB_ROUTINE_FARM && !Emulator::compareImages(instance, markers[config.selectedPortal]))
  {
    LOGGER_DEBUG("It is not in the correct portal");
  }

  if(currentRoutine == CB_ROUTINE_FARM && Emulator::compareImages(instance, markers[monster.name]))
  {
    Emulator::click(instance, markers[monster.name]);
    std::this_thread::sleep_for(std::chrono::milliseconds(3000));
  }
}