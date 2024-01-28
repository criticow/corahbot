#include "bot.hpp"

void Bot::waitFor(int amount, int deviation)
{
  int finalAmount = Random::choose(amount, amount + deviation);
  std::this_thread::sleep_for(std::chrono::milliseconds(finalAmount));
}

void Bot::run(const std::string &instance)
{
  int actionsPerSecond = 5;
  int crashCounter = 0;

  this->instance = instance;
  config = &Store::configs[instance];
  summary = &Store::summaries[instance];

  currentRoutine = CB_ROUTINE_NONE;
  currentAction = CB_ROUTINE_NONE;

  if(config->farm)
    currentRoutine = CB_ROUTINE_FARM;
  else if(config->combine && currentRoutine == CB_ROUTINE_NONE)
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
    if(config->farm && currentRoutine == CB_ROUTINE_NONE)
      currentRoutine = CB_ROUTINE_FARM;
    else if(config->combine && currentRoutine == CB_ROUTINE_NONE)
      currentRoutine = CB_ROUTINE_COMBINE;
    else if(!config->farm)
      currentRoutine = CB_ROUTINE_NONE;

    std::string location = findLocation(instance);

    int swordAmount = 9999;
    int potionAmount = 9999;

    if(location == CB_LOCATION_FIGHTING_FIGHTING)
    {
      if(currentRoutine == CB_ROUTINE_FARM && currentAction == CB_ACTION_NONE)
        currentAction = CB_ACTION_REFRESH_SWORDS;

      handleFighting(swordAmount, potionAmount);
    }

    if(location == CB_LOCATION_GEAR_GEAR || location == CB_LOCATION_GEAR_GUILDLESS_GEAR_GUILDLESS)
      handleGear();

    if(location == CB_LOCATION_BOOK_BOOK || location == CB_LOCATION_BOOK_GUILDLESS_BOOK_GUILDLESS)
      handleBook();
    
    if(location == CB_LOCATION_INVENTORY_INVENTORY)
      handleInventory();

    if(location == CB_LOCATION_ITEM_OPEN_ITEM_OPEN)
      handleItemOpen();

    if(location == CB_LOCATION_LOGIN_LOGIN)
      handleLogin();

    if(location == CB_LOCATION_HOME_HOME)
      handleHome();

    if(location == CB_LOCATION_REFILL_REFILL)
      handleRefill();

    if(location == CB_LOCATION_MAP_MAP)
      handleMap();
    
    if(location == CB_LOCATION_QUESTS_QUESTS)
      handleQuests();

    if(location == CB_LOCATION_QUEST_REWARD_QUEST_REWARD)
      handleQuestReward();

    if(location == CB_LOCATION_DISCONNECTED_DISCONNECTED)
    {
      Emulator::click(instance, Store::markers[CB_LOCATION_DISCONNECTED_DISCONNECTED][CB_LOCATION_DISCONNECTED_DISCONNECTED]);
      waitFor(500, 100);
    }

    if(location == CB_LOCATION_APP_CLOSED_APP_CLOSED)
    {
      crashCounter++;
      Emulator::click(instance, Store::markers[CB_LOCATION_APP_CLOSED_APP_CLOSED][CB_LOCATION_APP_CLOSED_APP_CLOSED]);
      waitFor(1500, 100);
    }

    if(location == CB_LOCATION_APP_CRASHED_APP_CRASHED)
    {
      Emulator::click(instance, Store::markers[CB_LOCATION_LOGIN_LOGIN][CB_POSITION_LOGIN_LOGIN_BTN]);
      waitFor(500, 100);
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

    summary->time = std::format("{:03d}:{:02d}:{:02d}", hours, minutes, seconds);
    summary->location = location;
    summary->routine = currentRoutine;
    summary->nextAction = currentAction;
    summary->swords = swordAmount == 9999 ? "unknown" : std::to_string(swordAmount);
    summary->potions = potionAmount == 9999 ? "unknown" : std::to_string(potionAmount);
    summary->crashs = std::to_string(crashCounter);
    summary->ms = std::to_string(duration.count());
    summary->actionsPerSecond = hasTimeLeft ? std::to_string(actionsPerSecond) : std::to_string(1000 / duration.count());

    instanceMutex.unlock();
#endif

    // This is so the bot takes n actionsPerSecond
    if(hasTimeLeft)
      waitFor((int)(1000 / actionsPerSecond) - duration.count());
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

void Bot::handleFighting(int &swords, int &potions)
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

  if(currentRoutine == CB_ROUTINE_FARM && potions > config->potionsThreshold && currentAction == CB_ACTION_REFRESH_POTIONS)
  {
    currentAction = CB_ACTION_REFRESH_SWORDS;
  }

  if(currentRoutine == CB_ROUTINE_FARM && potions < config->potionsThreshold)
  {
    // Open the options menu to start the refresh
    currentAction = CB_ACTION_REFRESH_POTIONS;
    if(Store::refreshModes[config->refreshMode] == CB_REFRESH_MODE_LOGOUT)
    {
      Emulator::click(instance, markers[CB_POSITION_FIGHTING_GEAR]);
      waitFor(700, 100);
    }
    else if (Store::refreshModes[config->refreshMode] == CB_REFRESH_MODE_CLOSE)
    {
      Emulator::killapp(instance, CORAH_PACKAGE_NAME);
      waitFor(1500, 100);
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

  if(currentRoutine == CB_ROUTINE_FARM && swords < config->swordsThreshold && currentAction == CB_ACTION_REFRESH_SWORDS)
  {
    // If the current swords is below the threshold the swords should be reset
    Emulator::click(instance, markers[CB_POSITION_FIGHTING_SWORDS]);
    waitFor(3000, 100);
  }
  // END SWORDS ACTIONS

  if(currentAction == CB_ACTION_REFRESH_BUFFS_INVENTORY)
  {
    Emulator::click(instance, markers[CB_POSITION_FIGHTING_BOOK]);
    waitFor(500, 100);
  }

  if(currentAction == CB_ACTION_REFRESH_SWORDS)
  {
    // Check if there is buff applied to reapply
    if(config->buffs && Emulator::compareImages(instance, markers[CB_POSITION_FIGHTING_BUFF_DEKDUN]))
    {
      currentAction = CB_ACTION_REFRESH_BUFFS_INVENTORY;
    }
  }

  // Quests
  if(currentRoutine == CB_ROUTINE_FARM && config->quests)
  {
    if(Emulator::compareImages(instance, markers[CB_POSITION_FIGHTING_QUEST_AVAILABLE]))
    {
      Emulator::click(instance, markers[CB_POSITION_FIGHTING_QUEST_AVAILABLE]);
      waitFor(500, 100);
    }

    if(Emulator::compareImages(instance, markers[CB_POSITION_FIGHTING_QUEST_FINISHED]))
    {
      Emulator::click(instance, markers[CB_POSITION_FIGHTING_QUEST_FINISHED]);
      waitFor(500, 100);
    }
  }
}

void Bot::handleGear()
{
  std::unordered_map<std::string, Marker> &markers = Store::markers[CB_LOCATION_GEAR_GEAR];
  std::unordered_map<std::string, Marker> &markers2 = Store::markers[CB_LOCATION_GEAR_GUILDLESS_GEAR_GUILDLESS];

  if(currentAction == CB_ACTION_REFRESH_POTIONS)
  {
    if(Emulator::compareImages(instance, markers[CB_POSITION_GEAR_LOGOUT]))
    {
      Emulator::click(instance, markers[CB_POSITION_GEAR_LOGOUT]);
      waitFor(1000, 100);
    }

    if(Emulator::compareImages(instance, markers2[CB_POSITION_GEAR_GUILDLESS_LOGOUT]))
    {
      Emulator::click(instance, markers2[CB_POSITION_GEAR_GUILDLESS_LOGOUT]);
      waitFor(1000, 100);
    }
  }
}

void Bot::handleBook()
{
  std::unordered_map<std::string, Marker> &markers = Store::markers[CB_LOCATION_BOOK_BOOK];
  std::unordered_map<std::string, Marker> &markers2 = Store::markers[CB_LOCATION_BOOK_GUILDLESS_BOOK_GUILDLESS];

  if(currentRoutine == CB_ROUTINE_FARM && currentAction == CB_ACTION_REFRESH_BUFFS_INVENTORY)
  {
    if(Emulator::compareImages(instance, markers[CB_POSITION_BOOK_BAG]))
    {
      Emulator::click(instance, markers[CB_POSITION_BOOK_BAG]);
      waitFor(1500, 100);
    }

    if(Emulator::compareImages(instance, markers2[CB_POSITION_BOOK_GUILDLESS_BAG]))
    {
      Emulator::click(instance, markers2[CB_POSITION_BOOK_GUILDLESS_BAG]);
      waitFor(1500, 100);
    }
  }

}

void Bot::handleInventory()
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
      for(auto &buff : config->selectedBuffs)
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
            waitFor(500, 100);
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
        config->buffs = false;
      }
    }

    
    if(currentAction == CB_ACTION_REFRESH_BUFFS_RETURN)
    {
      if(Emulator::compareImages(instance, markers[CB_POSITION_INVENTORY_CLOSE_BTN]))
      {
        currentAction = CB_ACTION_REFRESH_SWORDS;
        Emulator::click(instance, markers[CB_POSITION_INVENTORY_CLOSE_BTN]);
        waitFor(500, 100);
      }
    }
  }
}

void Bot::handleItemOpen()
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
        waitFor(1000, 100);
      }
    }
  }
}

void Bot::handleLogin()
{
  std::unordered_map<std::string, Marker> &markers = Store::markers[CB_LOCATION_LOGIN_LOGIN];

  if(currentRoutine == CB_ROUTINE_FARM)
  {
    Emulator::click(instance, markers[CB_POSITION_LOGIN_LOGIN_BTN]);
    waitFor(2000, 100);
  }
}

void Bot::handleHome()
{
  std::unordered_map<std::string, Marker> &markers = Store::markers[CB_LOCATION_HOME_HOME];
  bool shoudlRefill = Emulator::compareImages(instance, markers[CB_POSITION_HOME_POTION]);

  if(currentRoutine == CB_ROUTINE_FARM && !shoudlRefill)
  {
    if(Emulator::compareImages(instance, markers[CB_POSITION_HOME_REFILL_BTN]))
    {
      Emulator::click(instance, markers[CB_POSITION_HOME_REFILL_BTN]);
      waitFor(500, 100);
    }

    return;
  }

  if(currentRoutine == CB_ROUTINE_FARM && Emulator::compareImages(instance, markers[CB_POSITION_HOME_START_BTN]))
  {
    Emulator::click(instance, markers[CB_POSITION_HOME_START_BTN]);
    waitFor(500, 100);
  }
}

void Bot::handleRefill()
{
  std::unordered_map<std::string, Marker> &markers = Store::markers[CB_LOCATION_REFILL_REFILL];

  if(currentRoutine == CB_ROUTINE_FARM && Emulator::compareImages(instance, markers[CB_POSITION_REFILL_MAX_BTN_ACTIVE]))
  {
    Emulator::click(instance, markers[CB_POSITION_REFILL_MAX_BTN_ACTIVE]);
    waitFor(300, 100);
  }

  if(currentRoutine == CB_ROUTINE_FARM && Emulator::compareImages(instance, markers[CB_POSITION_REFILL_REFILL_BTN_ACTIVE]))
  {
    Emulator::click(instance, markers[CB_POSITION_REFILL_REFILL_BTN_ACTIVE]);
    waitFor(1500, 100);
  }

  if(currentRoutine == CB_ROUTINE_FARM && Emulator::compareImages(instance, markers[CB_POSITION_REFILL_MAX_BTN_INACTIVE]))
  {
    currentAction = CB_ACTION_REFRESH_SWORDS;
    Emulator::click(instance, markers[CB_POSITION_REFILL_MAX_BTN_INACTIVE]);
    waitFor(300, 100);
  }
}

void Bot::handleMap()
{
  std::unordered_map<std::string, Marker> &markers = Store::markers[CB_LOCATION_MAP_MAP];

  Monster &monster = Store::monsters[config->selectedPortal][config->selectedMonster];

  // Check if it is not in the correct portal, to change location
  if(currentRoutine == CB_ROUTINE_FARM && !Emulator::compareImages(instance, markers[config->selectedPortal]))
  {
    LOGGER_DEBUG("It is not in the correct portal");
  }

  if(currentRoutine == CB_ROUTINE_FARM && Emulator::compareImages(instance, markers[monster.name]))
  {
    Emulator::click(instance, markers[monster.name]);
    waitFor(3000, 100);
  }
}

void Bot::handleQuestReward()
{
  std::unordered_map<std::string, Marker> &markers = Store::markers[CB_LOCATION_QUEST_REWARD_QUEST_REWARD];

  if(currentRoutine == CB_ROUTINE_FARM)
  {
    if(Emulator::compareImages(instance, markers[CB_POSITION_QUEST_REWARD_CLAIM_BTN]))
    {
      Emulator::click(instance, markers[CB_POSITION_QUEST_REWARD_CLAIM_BTN]);
      waitFor(1000, 100);
    }
  }
}


void Bot::handleQuests()
{
  std::unordered_map<std::string, Marker> &markers = Store::markers[CB_LOCATION_QUESTS_QUESTS];

  if(currentRoutine == CB_ROUTINE_FARM && config->quests)
  {
    Marker &dragStart = markers[CB_POSITION_QUESTS_DRAG_START];
    Marker &dragEnd = markers[CB_POSITION_QUESTS_DRAG_END];

    Emulator::drag(instance, {dragStart.x, dragStart.y}, {dragEnd.x, dragEnd.y});
    waitFor(1500, 100);

    bool res = false;
    for(auto &quest : config->selectedQuests)
    {
      res = !Emulator::compareImages(instance, markers[quest]);

      if(res)
      {
        Emulator::click(instance, markers[quest]);
        waitFor(1500, 100);
        break;
      }
    }

    if(!res)
    {
      config->quests = false;
      Emulator::click(instance, markers[CB_POSITION_QUESTS_CLOSE_BTN]);
      waitFor(500, 100);
    }
  }
}