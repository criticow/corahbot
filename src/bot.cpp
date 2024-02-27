#include "bot.hpp"

std::string Bot::secondsToTime(int seconds)
{
  int h = seconds / 3600;
  int m = (seconds % 3600) / 60;
  int s = seconds % 60;

  return std::format("{:02d}:{:02d}:{:02d}", h, m, s);
}

void Bot::waitFor(int amount, int deviation)
{
  int finalAmount = Random::choose(amount, amount + deviation);
  std::this_thread::sleep_for(std::chrono::milliseconds(finalAmount));
}

bool Bot::checkEncounterRewards(std::unordered_map<std::string, Marker> &markers)
{
  bool claimed = false;
  if(Emulator::compareImages(instance, markers[CB_POSITION_ENCOUNTER_CLAIM_TOP_LEFT]))
  {
    Emulator::click(instance, markers[CB_POSITION_ENCOUNTER_CLAIM_TOP_LEFT]);
    waitFor(1500, 100);
    claimed = true;
  }

  if(Emulator::compareImages(instance, markers[CB_POSITION_ENCOUNTER_CLAIM_TOP_RIGHT]))
  {
    Emulator::click(instance, markers[CB_POSITION_ENCOUNTER_CLAIM_TOP_RIGHT]);
    waitFor(1500, 100);
    claimed = true;
  }

  if(Emulator::compareImages(instance, markers[CB_POSITION_ENCOUNTER_CLAIM_BOTTOM_RIGHT]))
  {
    Emulator::click(instance, markers[CB_POSITION_ENCOUNTER_CLAIM_BOTTOM_RIGHT]);
    waitFor(1500, 100);
    claimed = true;
  }

  if(Emulator::compareImages(instance, markers[CB_POSITION_ENCOUNTER_CLAIM_BOTTOM_LEFT]))
  {
    Emulator::click(instance, markers[CB_POSITION_ENCOUNTER_CLAIM_BOTTOM_LEFT]);
    waitFor(1500, 100);
    claimed = true;
  }

  return claimed;
}

std::string Bot::textFromImage(const std::string &windowTitle, Marker &marker)
{
  std::string text;

  cv::Mat image = Emulator::printscreen(windowTitle, marker.x, marker.y, marker.width, marker.height);

  // Convert cv::Mat to RGB format (STB Image uses RGB)
  cv::cvtColor(image, image, cv::COLOR_BGR2RGB);

  // Get image dimensions
  int width = image.cols;
  int height = image.rows;
  int channels = image.channels();

  // Read the image using stb_image
  unsigned char* imageData = image.data;
  
  // Set the image for OCR
  tess.SetImage(imageData, width, height, channels, channels * width);

  // Perform OCR and get the result
  char* outText = tess.GetUTF8Text();

  // Check if the detected text is empty or contains only whitespace
  if (outText != nullptr && std::string(outText).find_first_not_of(" \t\n\r\0") != std::string::npos)
  {
    text = outText;
  }

  // Release resources
  delete[] outText;

  return text;
}

void Bot::run(const std::string &instance)
{
  initTess();
  int actionsPerSecond = 5;
  int crashCounter = 0;

  this->instance = instance;
  config = &Store::configs[instance];
  summary = &Store::summaries[instance];

  currentRoutine = CB_ROUTINE_NONE;
  currentAction = CB_ROUTINE_NONE;

  int expectedRebootTime = 1000 * 60 * Random::choose(300, 310); // Reboot emulator
  int expectedRestartTime = 1000 * 60 * 60; // Restart the app 

  if(config->reboot)
    LOGGER_DEBUG("Reboot instance {} on {}ms", instance, expectedRebootTime);

  if(config->restart)
    LOGGER_DEBUG("Restarting app on instance {} on {}ms", instance, expectedRestartTime);

  if(config->farm)
    currentRoutine = CB_ROUTINE_FARM;

  tempo.setCooldown("clear_encounter_" + instance, 0);

  refreshMode = config->refreshMode;

  if(Store::refreshModes[config->refreshMode] == CB_REFRESH_MODE_RANDOM)
  {
    refreshMode = Random::choose(0, 1);
  }

  while(true)
  {
    HWND hwnd = FindWindow(nullptr, instance.c_str());

    // Window not found, instance is closed
    if(!hwnd)
    {
      continue;
    }

    RECT rect;
    GetClientRect(hwnd, &rect);

    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;

    // Window is minimized, cant send message events
    if(width == 0 || height == 0)
    {
      continue;
    }

    std::mutex &instanceMutex = Store::mutexes[instance];

    auto start = std::chrono::high_resolution_clock::now();

    InstanceState &state = Store::states[instance];

    // Check if the bot is working
    if(!state.working.load())
    {
      state.working.store(false);
      break;
    }

    if(config->reboot)
    {
      // 45 min reset
      // Check if the expected time to reboot has passed

      if(tempo.hasPassed("reboot_" + instance, expectedRebootTime))
      {
        LOGGER_DEBUG("Rebooting instance {}", instance);
        glm::ivec2 position = Emulator::getPosition(instance);
        Emulator::reboot(instance);
        // Wait for 30 sec after rebooting to start running again
        waitFor(1000 * 30);
        Emulator::setPosition(instance, position);
        tempo.clearTimepoint("reboot_" + instance);
      }
    }

    if(config->restart)
    {
      if(tempo.hasPassed("restart_" + instance, expectedRestartTime))
      {
        LOGGER_DEBUG("Restarting app on instance {}", instance);
        Emulator::killapp(instance, CORAH_PACKAGE_NAME);
        waitFor(1500, 100);
        tempo.clearTimepoint("restart_" + instance);
      }
    }

    // Sets the routine
    if(config->farm && currentRoutine == CB_ROUTINE_NONE)
      currentRoutine = CB_ROUTINE_FARM;
    else if(!config->farm)
      currentRoutine = CB_ROUTINE_NONE;

    findLocation(instance);

    // If is not inside the encounter clear the encounter action
    if(location != CB_LOCATION_ENCOUNTER_ENCOUNTER && currentAction == CB_ACTION_CLEAR_ENCOUNTER)
      currentAction = CB_ACTION_REFRESH_SWORDS;

    if(location == CB_LOCATION_FIGHTING_FIGHTING)
    {
      if(currentRoutine == CB_ROUTINE_FARM && currentAction == CB_ACTION_NONE)
        currentAction = CB_ACTION_REFRESH_SWORDS;

      handleFighting();
    }

    if(location == CB_LOCATION_GEAR_GEAR || location == CB_LOCATION_GEAR_GUILDLESS_GEAR_GUILDLESS)
      handleGear();

    if(location == CB_LOCATION_BOOK_BOOK || location == CB_LOCATION_BOOK_GUILDLESS_BOOK_GUILDLESS)
      handleBook();
    
    if(location == CB_LOCATION_INVENTORY_INVENTORY)
      handleInventory();

    if(location == CB_LOCATION_ITEM_OPEN_ITEM_OPEN)
      handleItemOpen();

    if(location == CB_LOCATION_LOGIN_LOGIN || location == CB_LOCATION_FAKE_LOGIN_FAKE_LOGIN)
      handleLogin();

    if(location == CB_LOCATION_HOME_HOME)
      handleHome();

    if(location == CB_LOCATION_REFILL_REFILL)
      handleRefill();

    if(location == CB_LOCATION_REFILL_PREMIUM_REFILL_PREMIUM)
      handleRefill();

    if(location == CB_LOCATION_MAP_MAP)
      handleMap();
    
    if(location == CB_LOCATION_QUESTS_QUESTS)
      handleQuests();

    if(location == CB_LOCATION_QUEST_REWARD_QUEST_REWARD)
      handleQuestReward();
    
    if(location == CB_LOCATION_ABORT_QUEST_ABORT_QUEST)
      handleAborQuest();

    if(location == CB_LOCATION_ENCOUNTER_ENCOUNTER)
      handleEncounter();

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

    if(location == CB_LOCATION_SYSTEM_STOPPED_SYSTEM_STOPPED)
    {
      Emulator::click(instance, Store::markers[CB_LOCATION_LOGIN_LOGIN][CB_POSITION_LOGIN_LOGIN_BTN]);
      waitFor(500, 100);
    }

    if(location == CB_LOCATION_REFER_OPEN_REFER_OPEN)
    {
      // Clicking anywhere above the region of the refering method
      Emulator::click(instance, Store::markers[CB_LOCATION_REFILL_REFILL][CB_POSITION_REFILL_REFILL_CLOSE_BTN]);
      waitFor(500, 100);
    }

    if(location == CB_LOCATION_MOB_INFO_OPEN_MOB_INFO_OPEN)
    {
      std::unordered_map<std::string, Marker> &markers = Store::markers[CB_LOCATION_MOB_INFO_OPEN_MOB_INFO_OPEN];
      // Sometimes if the game or the emulator is lagging the bot opens a mob info window after selecting the mob
      // from the map and gets stuck, here it will be checked if said window is open and close it
      if(Emulator::compareImages(instance, markers[CB_POSITION_MOB_INFO_OPEN_CLOSE_BTN]))
      {
        Emulator::click(instance, markers[CB_POSITION_MOB_INFO_OPEN_CLOSE_BTN]);
        waitFor(1000, 100);
      }
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    int hasTimeLeft = duration.count() < 1000 / actionsPerSecond;

    instanceMutex.lock();

    summary->time = secondsToTime(static_cast<int>(tempo.getTime()));
    summary->location = location;
    summary->routine = currentRoutine;
    summary->nextAction = currentAction;
    summary->crashs = std::to_string(crashCounter);
    summary->refreshSwords =  refreshSwords ? "true" : "false";
    summary->refreshPotions =  refreshPotions ? "true" : "false";
    summary->ms = std::to_string(duration.count());
    summary->actionsPerSecond = hasTimeLeft ? std::to_string(actionsPerSecond) : std::to_string(1000 / duration.count());
    summary->questsDone = std::to_string(questsDone);
    summary->encounterCooldown = secondsToTime(static_cast<int>(tempo.getCooldown("clear_encounter_" + instance) / 1000 * -1));
    summary->nextRefreshMode = Store::refreshModes[refreshMode];

    instanceMutex.unlock();

    // This is so the bot takes n actionsPerSecond
    if(hasTimeLeft)
      waitFor((int)(1000 / actionsPerSecond) - duration.count());
  }
}

void Bot::findLocation(const std::string &instance)
{
  location = CB_LOCATION_UNKNOWN;

  for(auto &[name, markers] : Store::markers)
  {
    if(Emulator::compareImages(instance, markers[name]))
    {
      location = markers[name].location;
      break;
    }
  }
}

void Bot::handleFighting()
{
  std::unordered_map<std::string, Marker> &markers = Store::markers[CB_LOCATION_FIGHTING_FIGHTING];

  // START POTION ACTIONS
  refreshSwords = Emulator::compareImages(instance, markers[CB_POSITION_FIGHTING_NO_SWORD]);
  bool potion0 = Emulator::compareImages(instance, markers[CB_POSITION_FIGHTING_POTION_0]);
  bool potion1 = Emulator::compareImages(instance, markers[CB_POSITION_FIGHTING_POTION_1]);

  refreshPotions = potion0 || potion1;

  // Bot is stuck at the swords
  if(Emulator::compareImages(instance, markers[CB_POSITION_FIGHTING_NO_SWORD_STUCK]))
  {
    refreshPotions = true;
  }

  if(currentRoutine == CB_ROUTINE_FARM && !refreshPotions && currentAction == CB_ACTION_REFRESH_POTIONS)
  {
    currentAction = CB_ACTION_REFRESH_SWORDS;
  }

  if(currentRoutine == CB_ROUTINE_FARM && refreshPotions)
  {
    // Open the options menu to start the refresh
    currentAction = CB_ACTION_REFRESH_POTIONS;
    std::string configMode = Store::refreshModes[config->refreshMode];
    std::string currentMode = Store::refreshModes[refreshMode];

    if(currentMode == CB_REFRESH_MODE_LOGOUT)
    {
      waitFor(1000, 100);
      Emulator::click(instance, markers[CB_POSITION_FIGHTING_GEAR]);
      waitFor(1500, 100);

      if(configMode == CB_REFRESH_MODE_RANDOM)
      {
        // Choose a new refresh mode between logout and map
        refreshMode = Random::choose(0, 1);
      }
    }
    else if (currentMode == CB_REFRESH_MODE_CLOSE)
    {
      Emulator::killapp(instance, CORAH_PACKAGE_NAME);
      waitFor(1500, 100);
    }
    else if (currentMode == CB_REFRESH_MODE_MAP)
    {
      // Check if swords are already disabled
      if(Emulator::compareImages(instance, markers[CB_POSITION_FIGHTING_NO_SWORD]))
      {
        // Open map
        Emulator::click(instance, markers[CB_POSITION_FIGHTING_MAP]);
        waitFor(1500, 100);

        if(configMode == CB_REFRESH_MODE_RANDOM)
        {
          // Choose a new refresh mode between logout and map
          refreshMode = Random::choose(0, 1);
        }
      }
      else
      {
        // Stop attacking
        Emulator::click(instance, markers[CB_POSITION_FIGHTING_NO_SWORD]);
        waitFor(1500, 100);
      }
    }
  }
  // END POTIONS ACTIONS

  // START SWORDS ACTIONS
  if(currentRoutine == CB_ROUTINE_FARM && refreshSwords && currentAction == CB_ACTION_REFRESH_SWORDS)
  {
    // If the current swords is below the threshold the swords should be reset
    if(Emulator::compareImages(instance, markers[CB_POSITION_FIGHTING_NO_SWORD]))
    {
      waitFor(1000, 100);
      Emulator::click(instance, markers[CB_POSITION_FIGHTING_NO_SWORD]);
      waitFor(1500, 100);
    }
  }
  // END SWORDS ACTIONS

  if(currentAction == CB_ACTION_REFRESH_BUFFS_INVENTORY)
  {
    waitFor(3000, 100);
    Emulator::click(instance, markers[CB_POSITION_FIGHTING_BOOK]);
    waitFor(3000, 100);
  }

  if(currentAction == CB_ACTION_REFRESH_SWORDS)
  {
    Marker &buffLoc = markers["buff_" + config->selectedPortal];
    // Check if there is buff applied to reapply
    if(config->buffs && Emulator::compareImages(instance, buffLoc))
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

  // Pets
  if(currentRoutine == CB_ROUTINE_FARM && config->pets)
  {
  }

  // Encounter
  if(currentRoutine == CB_ROUTINE_FARM && config->encounter && !tempo.isOnCooldown("clear_encounter_" + instance))
  {
    currentAction = CB_ACTION_CLEAR_ENCOUNTER;
    Emulator::click(instance, markers[CB_POSITION_FIGHTING_SHOW_ENCOUNTER]);
    waitFor(3500, 100);
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

          res = Emulator::find(haysteack, needle, 0.90);

          // The item was found
          if(res.first)
          {
            Emulator::click(instance, {marker.x + res.second.x, marker.y + res.second.y, res.second.z, res.second.w});
            waitFor(2500, 100);
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
        waitFor(2500, 100);
        Emulator::click(instance, markers[CB_POSITION_INVENTORY_CLOSE_BTN]);
        waitFor(2500, 100);
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
  bool potion29 = Emulator::compareImages(instance, markers[CB_POSITION_HOME_POTION_29]);
  bool potion89 = Emulator::compareImages(instance, markers[CB_POSITION_HOME_POTION_89]);
  bool shoudlRefill = potion29 || potion89;

  if(currentRoutine == CB_ROUTINE_FARM && !shoudlRefill)
  {
    if(Emulator::compareImages(instance, markers[CB_POSITION_HOME_REFILL_BTN]))
    {
      Emulator::click(instance, markers[CB_POSITION_HOME_REFILL_BTN]);
      waitFor(1000, 100);
    }

    return;
  }

  if(currentRoutine == CB_ROUTINE_FARM && Emulator::compareImages(instance, markers[CB_POSITION_HOME_START_BTN]))
  {
    currentAction = CB_ACTION_REFRESH_SWORDS;
    Emulator::click(instance, markers[CB_POSITION_HOME_START_BTN]);
    waitFor(1500, 100);
  }
}

void Bot::handleRefill()
{
  std::unordered_map<std::string, Marker> &markers = Store::markers[location];
  currentAction = CB_ACTION_REFRESH_SWORDS;

  if(location == CB_LOCATION_REFILL_REFILL)
  {
    if(currentRoutine == CB_ROUTINE_FARM && Emulator::compareImages(instance, markers[CB_POSITION_REFILL_MAX_BTN_ACTIVE]))
    {
      Emulator::click(instance, markers[CB_POSITION_REFILL_MAX_BTN_ACTIVE]);
      waitFor(500, 100);
    }

    if(currentRoutine == CB_ROUTINE_FARM && Emulator::compareImages(instance, markers[CB_POSITION_REFILL_REFILL_BTN_ACTIVE]))
    {
      Emulator::click(instance, markers[CB_POSITION_REFILL_REFILL_BTN_ACTIVE]);
      waitFor(1500, 100);
    }

    if(currentRoutine == CB_ROUTINE_FARM && Emulator::compareImages(instance, markers[CB_POSITION_REFILL_MAX_BTN_INACTIVE]))
    {
      Emulator::click(instance, markers[CB_POSITION_REFILL_REFILL_CLOSE_BTN]);
      waitFor(500, 100);
    }
  }

  if(location == CB_LOCATION_REFILL_PREMIUM_REFILL_PREMIUM)
  {
    if(currentRoutine == CB_ROUTINE_FARM && Emulator::compareImages(instance, markers[CB_POSITION_REFILL_PREMIUM_MAX_BTN_ACTIVE]))
    {
      Emulator::click(instance, markers[CB_POSITION_REFILL_PREMIUM_MAX_BTN_ACTIVE]);
      waitFor(500, 100);
    }

    if(currentRoutine == CB_ROUTINE_FARM && Emulator::compareImages(instance, markers[CB_POSITION_REFILL_PREMIUM_REFILL_BTN_ACTIVE]))
    {
      Emulator::click(instance, markers[CB_POSITION_REFILL_PREMIUM_REFILL_BTN_ACTIVE]);
      waitFor(1500, 100);
    }

    if(currentRoutine == CB_ROUTINE_FARM && Emulator::compareImages(instance, markers[CB_POSITION_REFILL_PREMIUM_MAX_BTN_INACTIVE]))
    {
      Emulator::click(instance, markers[CB_POSITION_REFILL_PREMIUM_REFILL_CLOSE_BTN]);
      waitFor(500, 100);
    }
  }
}

void Bot::handleMap()
{
  std::unordered_map<std::string, Marker> &markers = Store::markers[CB_LOCATION_MAP_MAP];

  Monster &monster = Store::monsters[config->selectedPortal][config->selectedMonster];

  if(currentRoutine != CB_ROUTINE_FARM)
  {
    return;
  }

  if(currentAction == CB_ACTION_REFRESH_POTIONS)
  {
    if(Emulator::compareImages(instance, markers[CB_POSITION_MAP_TOWN]))
    {
      Emulator::click(instance, markers[CB_POSITION_MAP_TOWN]);
      waitFor(1500, 100);
    }

    // Close the map if it should not go the the fighting scene
    if(Emulator::compareImages(instance, markers[CB_LOCATION_MAP_MAP]))
    {
      currentAction = CB_ACTION_REFRESH_SWORDS;
      // Click on the close button
      if(Emulator::compareImages(instance, markers[CB_POSITION_MAP_CLOSE_BTN]))
      {
        Emulator::click(instance, markers[CB_POSITION_MAP_CLOSE_BTN]);
        waitFor(1000, 100);
      }
    }
  }

  if(currentAction == CB_ACTION_REFRESH_SWORDS || currentAction == CB_ACTION_NONE)
  {
    // Check if it is not in the correct portal, to change location
    // if(!Emulator::compareImages(instance, markers[config->selectedPortal]))
    // {
    //   LOGGER_DEBUG("It is not in the correct portal");
    // }

    if(Emulator::compareImages(instance, markers[monster.name]))
    {
      Emulator::click(instance, markers[monster.name]);
      waitFor(1500, 100);
    }
  }
}

void Bot::handleQuestReward()
{
  std::unordered_map<std::string, Marker> &markers = Store::markers[CB_LOCATION_QUEST_REWARD_QUEST_REWARD];

  if(currentRoutine == CB_ROUTINE_FARM)
  {
    if(Emulator::compareImages(instance, markers[CB_POSITION_QUEST_REWARD_CLAIM_BTN]))
    {
      questsDone++;
      Emulator::click(instance, markers[CB_POSITION_QUEST_REWARD_CLAIM_BTN]);
      waitFor(1000, 100);
    }
  }
}

void Bot::handleAborQuest()
{
  std::unordered_map<std::string, Marker> &markers = Store::markers[CB_LOCATION_ABORT_QUEST_ABORT_QUEST];

  if(currentRoutine == CB_ROUTINE_FARM)
  {
    if(Emulator::compareImages(instance, markers[CB_POSITION_ABORT_QUEST_NO_BTN]))
    {
      Emulator::click(instance, markers[CB_POSITION_ABORT_QUEST_NO_BTN]);
      waitFor(500, 100);
    }
  }
}

void Bot::handleEncounter()
{
  std::unordered_map<std::string, Marker> &markers = Store::markers[CB_LOCATION_ENCOUNTER_ENCOUNTER];

  bool encounterFinished = false;

  std::string energyText = textFromImage(instance, markers[CB_POSITION_ENCOUNTER_ENERGY_TEXT_REGION]);
  if(!energyText.empty())
  {
    // Og string looks like 4 0/200(numbers here)
    std::vector<std::string> values = util::split(energyText, '(');
    if(!values.empty())
    {
      // substring will look like
      values = util::split(values[0], ' ');
      if(values.size() > 1)
      {
        if(values[1] == "0/200")
        {
          encounterFinished = true;
        }
      }
    }
  }

  // Exit if the encounter is finished or has passed 2 minutes
  if(config->encounter && (encounterFinished || tempo.hasPassed("exit_encounter_" + instance, 1000 * 60 * 2)))
  {
    currentAction = CB_ACTION_REFRESH_SWORDS;
    currentEncounterAttack = 0;
    currentEncounterMonster = 0;

    if(Emulator::compareImages(instance, markers[CB_LOCATION_ENCOUNTER_ENCOUNTER]))
    {
      Emulator::click(instance, markers[CB_LOCATION_ENCOUNTER_ENCOUNTER]);
      waitFor(2000, 100);
    }

    // Set cooldown of 30 min to clear encounter again
    tempo.setCooldown("clear_encounter_" + instance, 1000 * 60 * 30);
    return;
  }

  if(!config->encounter || currentAction != CB_ACTION_CLEAR_ENCOUNTER)
  {
    return;
  }

  // Check if there is something to claim
  if(checkEncounterRewards(markers))
  {
    return;
  }
  // End check

  // Check if needs to go to the next room
  if(Emulator::compareImages(instance, markers[CB_POSITION_ENCOUNTER_NEXT_ROOM]))
  {
    waitFor(3500, 100);
    checkEncounterRewards(markers);
    Emulator::click(instance, markers[CB_POSITION_ENCOUNTER_NEXT_ROOM]);
    waitFor(1500, 100);
    return;
  }
  // End check

  // Check if needs to start a new encounter
  if(Emulator::compareImages(instance, markers[CB_POSITION_ENCOUNTER_NEW_ENCOUNTER]))
  {
    Emulator::click(instance, markers[CB_POSITION_ENCOUNTER_NEW_ENCOUNTER]);
    waitFor(1500, 100);
    return;
  }
  // End Check

  // Check if needs to claim middle reward
  if(Emulator::compareImages(instance, markers[CB_POSITION_ENCOUNTER_REWARD_MIDDLE]))
  {
    Emulator::click(instance, markers[CB_POSITION_ENCOUNTER_REWARD_MIDDLE]);
    waitFor(1500, 100);
    return;
  }
  // End check

  if(currentEncounterMonster == 0)
  {
    Marker &pixel = markers[CB_POSITION_ENCOUNTER_TOP_LEFT_PIXEL];
    cv::Vec3b color = Emulator::getPixelColor(instance, pixel.x, pixel.y);

    int blue = color[0];
    int green = color[1];
    int red = color[2];

    // dark color when mob is dead is 1, 40, 75 if the current color at this point is diff the mob is alive
    bool hasHp = red != 1 || green != 40 || blue != 75;

    if(hasHp)
    {
      Emulator::click(instance, markers[CB_POSITION_ENCOUNTER_MONSTER_TOP_LEFT]);
      waitFor(1200, 100);
    }
    else
    {
      currentEncounterMonster = 1;
    }
  }

  if(currentEncounterMonster == 1)
  {
    Marker &pixel = markers[CB_POSITION_ENCOUNTER_TOP_RIGHT_PIXEL];
    cv::Vec3b color = Emulator::getPixelColor(instance, pixel.x, pixel.y);

    int blue = color[0];
    int green = color[1];
    int red = color[2];

    // dark color when mob is dead is 1, 40, 75 if the current color at this point is diff the mob is alive
    bool hasHp = red != 1 || green != 40 || blue != 75;

    if(hasHp)
    {
      Emulator::click(instance, markers[CB_POSITION_ENCOUNTER_MONSTER_TOP_RIGHT]);
      waitFor(1200, 100);
    }
    else
    {
      currentEncounterMonster = 2;
    }
  }

  if(currentEncounterMonster == 2)
  {
    Marker &pixel = markers[CB_POSITION_ENCOUNTER_BOTTOM_RIGHT_PIXEL];
    cv::Vec3b color = Emulator::getPixelColor(instance, pixel.x, pixel.y);

    int blue = color[0];
    int green = color[1];
    int red = color[2];

    // dark color when mob is dead is 1, 40, 75 if the current color at this point is diff the mob is alive
    bool hasHp = red != 1 || green != 40 || blue != 75;

    if(hasHp)
    {
      Emulator::click(instance, markers[CB_POSITION_ENCOUNTER_MONSTER_BOTTOM_RIGHT]);
      waitFor(1200, 100);
    }
    else
    {
      currentEncounterMonster = 3;
    }
  }

  if(currentEncounterMonster == 3)
  {
    Marker &pixel = markers[CB_POSITION_ENCOUNTER_BOTTOM_LEFT_PIXEL];
    cv::Vec3b color = Emulator::getPixelColor(instance, pixel.x, pixel.y);

    int blue = color[0];
    int green = color[1];
    int red = color[2];

    // dark color when mob is dead is 1, 40, 75 if the current color at this point is diff the mob is alive
    bool hasHp = red != 1 || green != 40 || blue != 75;

    if(hasHp)
    {
      Emulator::click(instance, markers[CB_POSITION_ENCOUNTER_MONSTER_BOTTOM_LEFT]);
      waitFor(1200, 100);
    }
  }

  currentEncounterAttack++;

  if(currentEncounterAttack > 1)
  {
    currentEncounterMonster++;
    currentEncounterAttack = 0;
  }

  if(currentEncounterMonster > 3) currentEncounterMonster = 0;
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
        waitFor(2500, 100);
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