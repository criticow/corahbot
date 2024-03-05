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

std::string Bot::textFromImage(cv::Mat &image)
{
  std::string text;

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

std::string Bot::textFromImage(const std::string &windowTitle, Marker &marker)
{
  cv::Mat image = Emulator::printscreen(windowTitle, marker.x, marker.y, marker.width, marker.height);

  // Convert cv::Mat to RGB format (STB Image uses RGB)
  cv::cvtColor(image, image, cv::COLOR_BGR2RGB);

  return textFromImage(image);
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

    if(location == CB_LOCATION_FISHING_FISHING)
      handleFishing();

    if(location == CB_LOCATION_PETS_PETS)
      handlePets();

    if(location == CB_LOCATION_NEW_PETS_FOUND_NEW_PETS_FOUND)
      handleNewPetsFound();

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
      // Set cooldown of 30 min to clear encounter again
      tempo.setCooldown("clear_fishing_" + instance, 1000 * 60 * 0);
      tempo.clearTimepoint("exit_fishing_" + instance);
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
    summary->fishingCooldown = secondsToTime(static_cast<int>(tempo.getCooldown("clear_fishing_" + instance) / 1000 * -1));
    summary->collectPetsCooldown = secondsToTime(static_cast<int>(tempo.getCooldown("collect_pets_" + instance) / 1000 * -1));
    summary->combineGemsCooldown = secondsToTime(static_cast<int>(tempo.getCooldown("combine_gems_" + instance) / 1000 * -1));
    summary->nextRefreshMode = Store::refreshModes[refreshMode];

    instanceMutex.unlock();

    // This is so the bot takes n actionsPerSecond
    if(hasTimeLeft)
      waitFor((int)(1000 / actionsPerSecond) - duration.count());
  }
  tess.End();
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
      if(!Emulator::compareImages(instance, markers[CB_POSITION_FIGHTING_NO_SWORD]))
      {
        // Stop attacking
        refreshSwords = false;
        Emulator::click(instance, markers[CB_POSITION_FIGHTING_NO_SWORD]);
        waitFor(1500, 100);
      }
      // Open map
      Emulator::click(instance, markers[CB_POSITION_FIGHTING_MAP]);
      waitFor(2500, 100);

      if(configMode == CB_REFRESH_MODE_RANDOM)
      {
        // Choose a new refresh mode between logout and map
        refreshMode = Random::choose(0, 1);
      }
    }
  }
  // END POTIONS ACTIONS

  // START SWORDS ACTIONS
  if(currentRoutine == CB_ROUTINE_FARM && refreshSwords && currentAction == CB_ACTION_REFRESH_SWORDS)
  {
    waitFor(1000, 100);
    Emulator::click(instance, markers[CB_POSITION_FIGHTING_NO_SWORD]);
    waitFor(1500, 100);
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
  if(currentRoutine == CB_ROUTINE_FARM && config->quests && config->selectedQuests.size() > 0)
  {
    if(Emulator::compareImages(instance, markers[CB_POSITION_FIGHTING_QUEST_AVAILABLE]))
    {
      waitFor(0, 2500);
      if(Emulator::compareImages(instance, markers[CB_POSITION_FIGHTING_QUEST_AVAILABLE]))
      {
        Emulator::click(instance, markers[CB_POSITION_FIGHTING_QUEST_AVAILABLE]);
        waitFor(1500, 100);
      }
    }

    if(Emulator::compareImages(instance, markers[CB_POSITION_FIGHTING_QUEST_FINISHED]))
    {
      waitFor(0, 2500);
      if(Emulator::compareImages(instance, markers[CB_POSITION_FIGHTING_QUEST_FINISHED]))
      {
        Emulator::click(instance, markers[CB_POSITION_FIGHTING_QUEST_FINISHED]);
        waitFor(1500, 100);
      }
    }
  }

  // Pets
  if(
    currentRoutine == CB_ROUTINE_FARM &&
    config->pets &&
    currentAction == CB_ACTION_REFRESH_SWORDS &&
    !tempo.isOnCooldown("collect_pets_" + instance)
  )
  {
    if(Emulator::compareImages(instance, markers[CB_POSITION_FIGHTING_INTERROGATION]))
    {
      currentAction = CB_ACTION_COLLECT_PETS;
      Emulator::click(instance, markers[CB_POSITION_FIGHTING_BOOK]);
      waitFor(1500, 100);
    }
  }

  // Encounter
  if(
    currentRoutine == CB_ROUTINE_FARM &&
    config->encounter &&
    currentAction == CB_ACTION_REFRESH_SWORDS &&
    !tempo.isOnCooldown("clear_encounter_" + instance)
  )
  {
    currentAction = CB_ACTION_CLEAR_ENCOUNTER;
    Emulator::click(instance, markers[CB_POSITION_FIGHTING_SHOW_ENCOUNTER]);
    waitFor(3500, 100);
  }

  // Fishing
  if(
    currentRoutine == CB_ROUTINE_FARM &&
    config->fishing &&
    currentAction == CB_ACTION_REFRESH_SWORDS &&
    !tempo.isOnCooldown("clear_fishing_" + instance)
  )
  {
    currentAction = CB_ACTION_CLEAR_FISHING;
    bool stillOnFighting = Emulator::compareImages(instance, markers[CB_LOCATION_FIGHTING_FIGHTING]);

    if(Emulator::compareImages(instance, markers[CB_POSITION_FIGHTING_FISHING]) && stillOnFighting)
    {
      Emulator::click(instance, markers[CB_POSITION_FIGHTING_FISHING]);
      waitFor(3500, 100);
    }
  }

  if(
    currentRoutine == CB_ROUTINE_FARM &&
    config->combine && 
    currentAction == CB_ACTION_REFRESH_SWORDS &&
    !tempo.isOnCooldown("combine_gems_" + instance)
  )
  {
    currentAction = CB_ACTION_COMBINE_GEMS;
    Emulator::click(instance, markers[CB_POSITION_FIGHTING_BOOK]);
    waitFor(1500, 100);
  }
}

void Bot::handleNewPetsFound()
{
  std::unordered_map<std::string, Marker> &markers = Store::markers[CB_LOCATION_NEW_PETS_FOUND_NEW_PETS_FOUND];

  if(!config->pets)
  {
    return;
  }

  if(Emulator::compareImages(instance, markers[CB_POSITION_NEW_PETS_FOUND_CONTINUE_BTN]))
  {
    Emulator::click(instance, markers[CB_POSITION_NEW_PETS_FOUND_CONTINUE_BTN]);
    waitFor(2000, 100);
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

  if(currentRoutine != CB_ROUTINE_FARM)
  {
    return;
  }

  if(currentAction == CB_ACTION_REFRESH_BUFFS_INVENTORY || currentAction == CB_ACTION_COMBINE_GEMS)
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

  if(currentAction == CB_ACTION_COLLECT_PETS)
  {
    if(Emulator::compareImages(instance, markers[CB_POSITION_BOOK_PETS_AVAILABLE]))
    {
      Emulator::click(instance, markers[CB_POSITION_BOOK_PETS_AVAILABLE]);
      waitFor(1500, 100);
    }
    else if(Emulator::compareImages(instance, markers2[CB_POSITION_BOOK_GUILDLESS_PETS_AVAILABLE]))
    {
      Emulator::click(instance, markers2[CB_POSITION_BOOK_GUILDLESS_PETS_AVAILABLE]);
      waitFor(1500, 100);
    }
    else
    {
      currentAction = CB_ACTION_REFRESH_SWORDS;
      tempo.setCooldown("collect_pets_" + instance, 1000 * 60 * 10);
      Emulator::click(instance, Store::markers[CB_LOCATION_FIGHTING_FIGHTING][CB_POSITION_FIGHTING_NO_SWORD]);
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
        currentAction = CB_ACTION_CLOSE_INVENTORY;
        config->buffs = false;
      }
    }
    
    if(currentAction == CB_ACTION_CLOSE_INVENTORY)
    {
      if(Emulator::compareImages(instance, markers[CB_POSITION_INVENTORY_CLOSE_BTN]))
      {
        currentAction = CB_ACTION_REFRESH_SWORDS;
        waitFor(2500, 100);
        Emulator::click(instance, markers[CB_POSITION_INVENTORY_CLOSE_BTN]);
        waitFor(2500, 100);
      }
    }

    if(currentAction == CB_ACTION_COMBINE_GEMS)
    {
      std::pair<bool, glm::ivec4> res = {false, glm::ivec4(0)};
      // Look for the first food in the list, if found click if not go to the next
      for(auto &gem : config->selectedGems)
      {
        for(auto &marker : invSlots)
        {
          cv::Mat needle = cv::imread("data/images/items/" + gem + ".png");
          cv::Mat haysteack = Emulator::printscreen(instance, marker.x, marker.y, marker.width, marker.height);

          res = Emulator::find(haysteack, needle, 0.90);

          // The item was found
          if(res.first)
          {
            Emulator::click(instance, {marker.x + res.second.x, marker.y + res.second.y, res.second.z, res.second.w});
            waitFor(1000, 100);
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
        currentAction = CB_ACTION_CLOSE_INVENTORY;
        // Since there are no gems/chips to combine just set a 30 min cooldown
        tempo.setCooldown("combine_gems_" + instance, 1000 * 60 * Random::choose(25, 33));
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
        currentAction = CB_ACTION_CLOSE_INVENTORY;
        Emulator::click(instance, {useBtnRegion.x + res.second.x, useBtnRegion.y + res.second.y, res.second.z, res.second.w});
        waitFor(1000, 100);
      }
    }

    if(currentAction == CB_ACTION_COMBINE_GEMS)
    {
      bool canCombine = Emulator::compareImages(instance, markers[CB_POSITION_ITEM_OPEN_COMBINE_BTN]);
      bool canMerge = Emulator::compareImages(instance, markers[CB_POSITION_ITEM_OPEN_MERGE_BTN]);

      if(canCombine || canMerge)
      {
        Emulator::click(instance, markers[CB_POSITION_ITEM_OPEN_COMBINE_BTN]);
        waitFor(500, 100);
      }
      else
      {
        // 25 - 33 min cooldown if there are no gems to combine/merge (wont work correctly if combining/merging multiple)
        tempo.setCooldown("combine_gems_" + instance, 1000 * 60 * Random::choose(25,33));
        currentAction = CB_ACTION_CLOSE_INVENTORY;
      }

      if(tempo.hasPassed("combine_gems_runtime_" + instance, 1000 * Random::choose(30,45)))
      {
        tempo.setCooldown("combine_gems_" + instance, 1000 * 60 * Random::choose(5,7));
        tempo.clearTimepoint("combine_gems_runtime_" + instance);
        currentAction = CB_ACTION_CLOSE_INVENTORY;
      }
    }

    // Make sure it closes the item if it is still open after doing a action
    if(currentAction != CB_ACTION_REFRESH_SWORDS && Emulator::compareImages(instance, markers[CB_LOCATION_ITEM_OPEN_ITEM_OPEN]))
    {
      Emulator::click(instance, markers[CB_LOCATION_ITEM_OPEN_ITEM_OPEN]);
      waitFor(1000, 100);
    }
  }
}

void Bot::handleLogin()
{
  std::unordered_map<std::string, Marker> &markers = Store::markers[CB_LOCATION_LOGIN_LOGIN];

  if(currentRoutine != CB_ROUTINE_FARM)
  {
    return;
  }

  if(Emulator::compareImages(instance, markers[CB_POSITION_LOGIN_LOGIN_WARRIOR]))
  {
    Emulator::click(instance, markers[CB_POSITION_LOGIN_LOGIN_WARRIOR]);
    waitFor(2000, 100);
  }

  if(Emulator::compareImages(instance, markers[CB_POSITION_LOGIN_LOGIN_HUNTER]))
  {
    Emulator::click(instance, markers[CB_POSITION_LOGIN_LOGIN_HUNTER]);
    waitFor(2000, 100);
  }
  
  if(Emulator::compareImages(instance, markers[CB_POSITION_LOGIN_LOGIN_MAGE]))
  {
    Emulator::click(instance, markers[CB_POSITION_LOGIN_LOGIN_MAGE]);
    waitFor(2000, 100);
  }

  if(Emulator::compareImages(instance, markers[CB_POSITION_LOGIN_LOGIN_ROGUE]))
  {
    Emulator::click(instance, markers[CB_POSITION_LOGIN_LOGIN_ROGUE]);
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
      waitFor(2500, 100);
    }

  }

  if(currentAction == CB_ACTION_REFRESH_SWORDS || currentAction == CB_ACTION_NONE)
  {
    // Check if it is not in the correct portal, to change location
    if(!Emulator::compareImages(instance, markers[config->selectedPortal]))
    {
      LOGGER_DEBUG("It is not in the correct portal");
    }

    if(Emulator::compareImages(instance, markers[monster.name]))
    {
      Emulator::click(instance, markers[monster.name]);
      waitFor(1500, 100);
    }
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

void Bot::handleFishing()
{
  std::unordered_map<std::string, Marker> &markers = Store::markers[CB_LOCATION_FISHING_FISHING];

  bool fishingFinished = false;

  if(!config->fishing)
  {
    return;
  }

  Marker baitTextRegion = markers[CB_POSITION_FISHING_BAIT_TEXT_REGION];
  Marker baitTextRegionDown = markers[CB_POSITION_FISHING_BAIT_TEXT_REGION_DOWN];

  // Check baits up and down
  std::string text = textFromImage(instance, baitTextRegion);
  std::string textDown = textFromImage(instance, baitTextRegionDown);

  size_t pos10 = text.find("10/30");
  size_t pos20 = text.find("20/30");
  size_t pos30 = text.find("30/30");
  size_t pos0 = text.find("0/30");

  if(pos0 != std::string::npos && pos10 == std::string::npos && pos20 == std::string::npos && pos30 == std::string::npos)
  {
    fishingFinished = true;
  }

  pos0 = textDown.find("0/30");
  pos10 = textDown.find("10/30");
  pos20 = textDown.find("20/30");
  pos30 = textDown.find("30/30");

  if(pos0 != std::string::npos && pos10 == std::string::npos && pos20 == std::string::npos && pos30 == std::string::npos)
  {
    fishingFinished = true;
  }

  Marker dailyQuotaRegion = markers[CB_POSITION_FISHING_DAILY_QUOTA_TEXT];
  Marker dailyQuotaRegionDown = markers[CB_POSITION_FISHING_DAILY_QUOTA_TEXT_DOWN];

  std::string dqText = textFromImage(instance, dailyQuotaRegion);
  std::string dqTextDown = textFromImage(instance, dailyQuotaRegionDown);

  size_t resPos = dqText.find("res");
  size_t resPosDown = dqTextDown.find("res");

  if(resPos != std::string::npos || resPosDown != std::string::npos)
  {
    fishingFinished = true;
  }

  // Check if getting bait is disabled
  bool baiting = Emulator::compareImages(instance, markers[CB_POSITION_FISHING_BAITING_DISABLED]);
  bool baitingDown = Emulator::compareImages(instance, markers[CB_POSITION_FISHING_BAITING_DISABLED_DOWN]);

  if(baiting)
  {
    Emulator::click(instance, markers[CB_POSITION_FISHING_BAITING_DISABLED]);
    waitFor(1500, 100);
    return;
  }

  if(baitingDown)
  {
    Emulator::click(instance, markers[CB_POSITION_FISHING_BAITING_DISABLED_DOWN]);
    waitFor(1500, 100);
    return;
  }

  if(fishingFinished || currentAction != CB_ACTION_CLEAR_FISHING || tempo.hasPassed("exit_fishing_" + instance, 1000 * 45))
  {
    currentAction = CB_ACTION_REFRESH_SWORDS;

    waitFor(3000, 100);
    if(Emulator::compareImages(instance, markers[CB_POSITION_FISHING_CLOSE_BTN]))
    {
      Emulator::click(instance, markers[CB_POSITION_FISHING_CLOSE_BTN]);
      waitFor(2000, 100);
    }

    // Set cooldown of 30 min to clear encounter again
    tempo.setCooldown("clear_fishing_" + instance, 1000 * 60 * Random::choose(38, 43));
    tempo.clearTimepoint("exit_fishing_" + instance);
    return;
  }

  bool info = Emulator::compareImages(instance, markers[CB_POSITION_FISHING_INFO]);
  bool infoDown = Emulator::compareImages(instance, markers[CB_POSITION_FISHING_INFO_DOWN]);

  if(info)
  {
    Emulator::click(instance, markers[CB_POSITION_FISHING_START_FISHING]);
    waitFor(2000, 100);
  }

  if(infoDown)
  {
    Emulator::click(instance, markers[CB_POSITION_FISHING_START_FISHING_DOWN]);
    waitFor(2000, 100);
  }
}

void Bot::handlePets()
{
  std::unordered_map<std::string, Marker> &markers = Store::markers[CB_LOCATION_PETS_PETS];
  if(!config->pets || currentRoutine != CB_ROUTINE_FARM)
  {
    return;
  }

  // Check if it is on cooldown or the max time has passed
  std::vector<Marker> pets = {
    markers[CB_POSITION_PETS_CLAIM_1X1], markers[CB_POSITION_PETS_CLAIM_1X2], markers[CB_POSITION_PETS_CLAIM_1X3],
    markers[CB_POSITION_PETS_CLAIM_2X1], markers[CB_POSITION_PETS_CLAIM_2X2], markers[CB_POSITION_PETS_CLAIM_2X3],
    markers[CB_POSITION_PETS_CLAIM_3X1], markers[CB_POSITION_PETS_CLAIM_3X2], markers[CB_POSITION_PETS_CLAIM_3X3],
    markers[CB_POSITION_PETS_CLAIM_4X1], markers[CB_POSITION_PETS_CLAIM_4X2], markers[CB_POSITION_PETS_CLAIM_4X3]
  };

  // Check if there is new pets to get
  if(Emulator::compareImages(instance, markers[CB_POSITION_PETS_NEW_PETS]))
  {
    Emulator::click(instance, markers[CB_POSITION_PETS_NEW_PETS]);
    waitFor(2000, 100);
    return;
  }

  for(auto &marker : pets)
  {
    if(Emulator::compareImages(instance, marker))
    {
      Marker position = marker;
      position.y = marker.y + 10;

      Emulator::click(instance, position);
      waitFor(1500, 100);
      Emulator::click(instance, position);
      waitFor(1500, 100);
      return;
    }
  }

  if(Emulator::compareImages(instance, markers[CB_POSITION_PETS_CLOSE_BTN]))
  {
    currentAction = CB_ACTION_REFRESH_SWORDS;
    Emulator::click(instance, markers[CB_POSITION_PETS_CLOSE_BTN]);
    waitFor(2500, 100);
  }
}

void Bot::handleEncounter()
{
  std::unordered_map<std::string, Marker> &markers = Store::markers[CB_LOCATION_ENCOUNTER_ENCOUNTER];

  if(!config->encounter)
  {
    return;
  }

  bool encounterFinished = false;

  Marker textRegion = markers[CB_POSITION_ENCOUNTER_ENERGY_TEXT_REGION];

  std::string energyText = textFromImage(instance, textRegion);
  if(!energyText.empty())
  {
    // Og string looks like 4 0/200(numbers here)
    std::vector<std::string> values = util::split(energyText, '(');
    if(!values.empty())
    {
      // substring will look like
      values = util::split(values[0], ' ');
      std::string value = values.size() > 1 ? values[1] : values[0];
      if(value == "0/200")
      {
        encounterFinished = true;
      }
    }
  }

  // Exit if the encounter is finished or has passed 1.5 minutes
  if(encounterFinished || currentAction != CB_ACTION_CLEAR_ENCOUNTER || tempo.hasPassed("exit_encounter_" + instance, 1000 * 60 * 1.5))
  {
    currentAction = CB_ACTION_REFRESH_SWORDS;
    currentEncounterAttack = 0;
    currentEncounterMonster = 0;

    waitFor(3000, 100);
    if(Emulator::compareImages(instance, markers[CB_LOCATION_ENCOUNTER_ENCOUNTER]))
    {
      Emulator::click(instance, markers[CB_LOCATION_ENCOUNTER_ENCOUNTER]);
      waitFor(2000, 100);
    }

    // Set cooldown of 30 min to clear encounter again
    tempo.setCooldown("clear_encounter_" + instance, 1000 * 60 * Random::choose(28, 33));
    tempo.clearTimepoint("exit_encounter_" + instance);
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
    currentEncounterAttack = 0;
    currentEncounterMonster = 0;
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
    currentEncounterAttack = 0;
    currentEncounterMonster = 0;
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
    Marker &monsterText = markers[CB_POSITION_ENCOUNTER_TOP_LEFT_TEXT];
    if(!Emulator::compareImages(instance, monsterText))
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
    Marker &monsterText = markers[CB_POSITION_ENCOUNTER_TOP_RIGHT_TEXT];
    if(!Emulator::compareImages(instance, monsterText))
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
    Marker &monsterText = markers[CB_POSITION_ENCOUNTER_BOTTOM_RIGHT_TEXT];
    if(!Emulator::compareImages(instance, monsterText))
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
    Marker &monsterText = markers[CB_POSITION_ENCOUNTER_BOTTOM_LEFT_TEXT];
    if(!Emulator::compareImages(instance, monsterText))
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
    bool res = false;
    for(auto &quest : config->selectedQuests)
    {
      res = !Emulator::compareImages(instance, markers[quest]);

      if(res)
      {
        waitFor(500, 1500);
        Emulator::click(instance, markers[quest]);
        waitFor(2500, 100);
        break;
      }
    }

    if(!res)
    {
      config->quests = false;
      waitFor(500, 1500);
      Emulator::click(instance, markers[CB_POSITION_QUESTS_CLOSE_BTN]);
      waitFor(500, 100);
    }
  }
}