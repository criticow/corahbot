#include "bot.hpp"


void Bot::run(const std::string &instance)
{
  int actionsPerSecond = 5;
  int crashCounter = 0;

  WorkConfig &config = Store::configs[instance];
  Summary &summary = Store::summaries[instance];

  std::string currentRoutine = CB_ROUTINE_NONE;
  std::string currentAction = CB_ROUTINE_NONE;

  if(config.farm)
    currentRoutine = CB_ROUTINE_FARM;
  else if(config.combine && currentRoutine == CB_ROUTINE_NONE)
    currentRoutine = CB_ROUTINE_COMBINE;

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

    // Sets the routine
    if(config.farm && currentRoutine == CB_ROUTINE_NONE)
      currentRoutine = CB_ROUTINE_FARM;
    else if(config.combine && currentRoutine == CB_ROUTINE_NONE)
      currentRoutine = CB_ROUTINE_COMBINE;
    else if(!config.farm)
      currentRoutine = CB_ROUTINE_NONE;

    std::string location = findLocation(instance);

    int swordAmount = 9999;
    int potionAmount = 9999;

    if(location == CB_LOCATION_FIGHTING_FIGHTING)
    {
      if(currentRoutine == CB_ROUTINE_FARM && currentAction == CB_ACTION_NONE)
        currentAction = CB_ACTION_REFRESH_SWORDS;

      handleFighting(instance, config, summary, currentRoutine, currentAction, swordAmount, potionAmount);
    }

    if(location == CB_LOCATION_GEAR_GEAR || location == CB_LOCATION_GEAR_GUILDLESS_GEAR_GUILDLESS)
      handleGear(instance, config, summary, currentRoutine, currentAction);

    if(location == CB_LOCATION_BOOK_BOOK || location == CB_LOCATION_BOOK_GUILDLESS_BOOK_GUILDLESS)
      handleBook(instance, config, summary, currentRoutine, currentAction);
    
    if(location == CB_LOCATION_INVENTORY_INVENTORY)
      handleInventory(instance, config, summary, currentRoutine, currentAction);

    if(location == CB_LOCATION_ITEM_OPEN_ITEM_OPEN)
      handleItemOpen(instance, config, summary, currentRoutine, currentAction);

    if(location == CB_LOCATION_LOGIN_LOGIN)
      handleLogin(instance, config, summary, currentRoutine, currentAction);

    if(location == CB_LOCATION_HOME_HOME)
      handleHome(instance, config, summary, currentRoutine, currentAction);

    if(location == CB_LOCATION_REFILL_REFILL)
      handleRefill(instance, config, summary, currentRoutine, currentAction);

    if(location == CB_LOCATION_MAP_MAP)
      handleMap(instance, config, summary, currentRoutine, currentAction);

    if(location == CB_LOCATION_DISCONNECTED_DISCONNECTED)
    {
      Emulator::click(instance, Store::markers[CB_LOCATION_DISCONNECTED_DISCONNECTED][CB_LOCATION_DISCONNECTED_DISCONNECTED]);
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    if(location == CB_LOCATION_APP_CLOSED_APP_CLOSED)
    {
      crashCounter++;
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

    summary.time = std::format("{:03d}:{:02d}:{:02d}", hours, minutes, seconds);
    summary.location = location;
    summary.routine = currentRoutine;
    summary.nextAction = currentAction;
    summary.swords = swordAmount == 9999 ? "unknown" : std::to_string(swordAmount);
    summary.potions = potionAmount == 9999 ? "unknown" : std::to_string(potionAmount);
    summary.crashs = std::to_string(crashCounter);

    summary.ms = std::to_string(duration.count());
    summary.actionsPerSecond = hasTimeLeft ? std::to_string(actionsPerSecond) : std::to_string(1000 / duration.count());

    instanceMutex.unlock();
#endif

    // This is so the bot takes n actionsPerSecond
    if(hasTimeLeft)
      std::this_thread::sleep_for(std::chrono::milliseconds((1000 / actionsPerSecond) - duration.count()));
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

void Bot::handleFighting(const std::string &instance, WorkConfig &config, Summary &summary, std::string &currentRoutine, std::string &currentAction, int &swords, int &potions)
{
  std::unordered_map<std::string, Marker> &markers = Store::markers[CB_LOCATION_FIGHTING_FIGHTING];

  // START POTION ACTIONS
  std::pair<bool, glm::ivec4> potionRes = Emulator::find(instance, markers[CB_POSITION_FIGHTING_POTIONS], "atlas/potions");

  // If potions are not visible there is nothing more to do here
  if(!potionRes.first)
  {
    return;
  }

  potions = Store::potionsMap[std::format("{}{}", potionRes.second.x, potionRes.second.y)];

  if(currentRoutine == CB_ROUTINE_FARM && potions > config.potionsThreshold && currentAction == CB_ACTION_REFRESH_POTIONS)
  {
    currentAction = CB_ACTION_REFRESH_SWORDS;
  }

  if(currentRoutine == CB_ROUTINE_FARM && potions < config.potionsThreshold)
  {
    // Open the options menu to start the refresh
    currentAction = CB_ACTION_REFRESH_POTIONS;
    if(Store::refreshModes[config.refreshMode] == CB_REFRESH_MODE_LOGOUT)
    {
      Emulator::click(instance, markers[CB_POSITION_FIGHTING_GEAR]);
      std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }
    else if (Store::refreshModes[config.refreshMode] == CB_REFRESH_MODE_CLOSE)
    {
      Emulator::killapp(instance, CORAH_PACKAGE_NAME);
      std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    }
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

  if(currentAction == CB_ACTION_REFRESH_BUFFS_INVENTORY)
  {
    Emulator::click(instance, markers[CB_POSITION_FIGHTING_BOOK]);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }

  if(currentAction == CB_ACTION_REFRESH_SWORDS)
  {
    // Check if there is buff applied to reapply
    if(config.buffs && Emulator::compareImages(instance, markers[CB_POSITION_FIGHTING_BUFF_DEKDUN]))
    {
      currentAction = CB_ACTION_REFRESH_BUFFS_INVENTORY;
    }

  }
}

void Bot::handleGear(const std::string &instance, WorkConfig &config, Summary &summary, std::string &currentRoutine, std::string &currentAction)
{
  std::unordered_map<std::string, Marker> &markers = Store::markers[CB_LOCATION_GEAR_GEAR];
  std::unordered_map<std::string, Marker> &markers2 = Store::markers[CB_LOCATION_GEAR_GUILDLESS_GEAR_GUILDLESS];

  if(currentAction == CB_ACTION_REFRESH_POTIONS)
  {
    if(Emulator::compareImages(instance, markers[CB_POSITION_GEAR_LOGOUT]))
    {
      Emulator::click(instance, markers[CB_POSITION_GEAR_LOGOUT]);
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    if(Emulator::compareImages(instance, markers2[CB_POSITION_GEAR_GUILDLESS_LOGOUT]))
    {
      Emulator::click(instance, markers2[CB_POSITION_GEAR_GUILDLESS_LOGOUT]);
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
  }
}

void Bot::handleBook(const std::string &instance, WorkConfig &config, Summary &summary, std::string &currentRoutine, std::string &currentAction)
{
  std::unordered_map<std::string, Marker> &markers = Store::markers[CB_LOCATION_BOOK_BOOK];
  std::unordered_map<std::string, Marker> &markers2 = Store::markers[CB_LOCATION_BOOK_GUILDLESS_BOOK_GUILDLESS];

  if(currentRoutine == CB_ROUTINE_FARM && currentAction == CB_ACTION_REFRESH_BUFFS_INVENTORY)
  {
    if(Emulator::compareImages(instance, markers[CB_POSITION_BOOK_BAG]))
    {
      Emulator::click(instance, markers[CB_POSITION_BOOK_BAG]);
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    if(Emulator::compareImages(instance, markers2[CB_POSITION_BOOK_GUILDLESS_BAG]))
    {
      Emulator::click(instance, markers2[CB_POSITION_BOOK_GUILDLESS_BAG]);
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
  }

}

void Bot::handleInventory(const std::string &instance, WorkConfig &config, Summary &summary, std::string &currentRoutine, std::string &currentAction)
{
  std::unordered_map<std::string, Marker> &markers = Store::markers[CB_LOCATION_INVENTORY_INVENTORY];
  std::vector<Marker> invSlots{
    markers[CB_POSITION_INVENTORY_INV_SLOT_1], markers[CB_POSITION_INVENTORY_INV_SLOT_2], markers[CB_POSITION_INVENTORY_INV_SLOT_3],
    markers[CB_POSITION_INVENTORY_INV_SLOT_4], markers[CB_POSITION_INVENTORY_INV_SLOT_5], markers[CB_POSITION_INVENTORY_INV_SLOT_6],
    markers[CB_POSITION_INVENTORY_INV_SLOT_7], markers[CB_POSITION_INVENTORY_INV_SLOT_8], markers[CB_POSITION_INVENTORY_INV_SLOT_9],
    markers[CB_POSITION_INVENTORY_INV_SLOT_10], markers[CB_POSITION_INVENTORY_INV_SLOT_11], markers[CB_POSITION_INVENTORY_INV_SLOT_12],
    markers[CB_POSITION_INVENTORY_INV_SLOT_13], markers[CB_POSITION_INVENTORY_INV_SLOT_14], markers[CB_POSITION_INVENTORY_INV_SLOT_15],
    markers[CB_POSITION_INVENTORY_INV_SLOT_16], markers[CB_POSITION_INVENTORY_INV_SLOT_17], markers[CB_POSITION_INVENTORY_INV_SLOT_18],
    markers[CB_POSITION_INVENTORY_INV_SLOT_19], markers[CB_POSITION_INVENTORY_INV_SLOT_20]
  };

  if(currentRoutine == CB_ROUTINE_FARM)
  {
    if(currentAction == CB_ACTION_REFRESH_BUFFS_INVENTORY)
    {
      std::pair<bool, glm::ivec4> res = {false, glm::ivec4(0)};
      // Look for the first food in the list, if found click if not go to the next
      for(auto &buff : config.selectedBuffs)
      {
        for(auto &marker : invSlots)
        {
          cv::Mat needle = cv::imread("data/images/items/" + buff + ".png");
          cv::Mat haysteack = Emulator::printscreen(instance, marker.x, marker.y, marker.width, marker.height);

          res = Emulator::find(haysteack, needle, 0.95);

          // The item was found
          if(res.first)
          {
            Emulator::click(instance, {marker.x + res.second.x, marker.y + res.second.y, res.second.z, res.second.w});
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            break;
          }
        }

        // The item was found
        if(res.first)
          break;
      }

      // Not found, close and deactivate buffing
      if(!res.first)
      {
        currentAction = CB_ACTION_REFRESH_BUFFS_RETURN;
        config.buffs = false;
      }
    }

    
    if(currentAction == CB_ACTION_REFRESH_BUFFS_RETURN)
    {
      if(Emulator::compareImages(instance, markers[CB_POSITION_INVENTORY_CLOSE_BTN]))
      {
        currentAction = CB_ACTION_REFRESH_SWORDS;
        Emulator::click(instance, markers[CB_POSITION_INVENTORY_CLOSE_BTN]);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
      }
    }
  }
}

void Bot::handleItemOpen(const std::string &instance, WorkConfig &config, Summary &summary, std::string &currentRoutine, std::string &currentAction)
{
  std::unordered_map<std::string, Marker> &markers = Store::markers[CB_LOCATION_ITEM_OPEN_ITEM_OPEN];

  if(currentRoutine == CB_ROUTINE_FARM)
  {
    if(currentAction == CB_ACTION_REFRESH_BUFFS_INVENTORY)
    {
      Marker &useBtnRegion = markers[CB_POSITION_ITEM_OPEN_USE_BTN_REGION];
      Marker &useBtn = markers[CB_POSITION_ITEM_OPEN_USE_BTN];

      cv::Mat haystack = Emulator::printscreen(instance, useBtnRegion);
      cv::Mat needle = cv::imread("data/images/" + useBtn.location + "/" + useBtn.name + ".png");
      // Locate the use button
      std::pair<bool, glm::ivec4> res = Emulator::find(haystack, needle, 0.95);

      if(res.first)
      {
        currentAction = CB_ACTION_REFRESH_BUFFS_RETURN;
        Emulator::click(instance, {useBtnRegion.x + res.second.x, useBtnRegion.y + res.second.y, res.second.z, res.second.w});
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
      }
    }
  }
}

void Bot::handleLogin(const std::string &instance, WorkConfig &config, Summary &summary, std::string &currentRoutine, std::string &currentAction)
{
  std::unordered_map<std::string, Marker> &markers = Store::markers[CB_LOCATION_LOGIN_LOGIN];

  if(currentRoutine == CB_ROUTINE_FARM)
  {
    Emulator::click(instance, markers[CB_POSITION_LOGIN_LOGIN_BTN]);
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
  }
}

void Bot::handleHome(const std::string &instance, WorkConfig &config, Summary &summary, std::string &currentRoutine, std::string &currentAction)
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

void Bot::handleRefill(const std::string &instance, WorkConfig &config, Summary &summary, std::string &currentRoutine, std::string &currentAction)
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

void Bot::handleMap(const std::string &instance, WorkConfig &config, Summary &summary, std::string &currentRoutine, std::string &currentAction)
{
  std::unordered_map<std::string, Marker> &markers = Store::markers[CB_LOCATION_MAP_MAP];

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