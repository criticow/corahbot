#include "gui.hpp"
#include "application.hpp"

ImVec4 yellow(1.0f, 0.9f, 0.2f, 1.0f);
ImVec4 orange(0.99f, 0.69f, 0.26f, 1.0f);
ImVec4 white(1.0f, 1.0f, 1.0f, 1.0f);
ImVec4 black(0.0f, 0.0f, 0.0f, 1.0f);
ImVec4 green(0.0f, 1.0f, 0.0f, 1.0f);
ImVec4 red(1.0f, 0.0f, 0.0f, 1.0f);
ImVec4 blue(0.0f, 0.0f, 1.0f, 1.0f);

int currentTab = 0;

void botThread(const std::string &instance)
{
  Bot bot;
  bot.run(instance);
}

void RoundedTag(const char* label, ImVec4 bgColor, ImVec4 textColor, ImVec4 borderColor) {
  const ImVec2 labelSize = ImGui::CalcTextSize(label, NULL, true);
  const ImVec2 padding(ImGui::GetStyle().FramePadding);

  ImVec2 pos = ImGui::GetCursorScreenPos();
  ImVec2 size = ImVec2(labelSize.x + padding.x * 2, labelSize.y + padding.y * 2);

  ImGui::InvisibleButton(label, size);

  // Draw background
  ImGui::GetWindowDrawList()->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), IM_COL32(bgColor.x * 255, bgColor.y * 255, bgColor.z * 255, bgColor.w * 255), 4.0f);

  // Draw border
  ImGui::GetWindowDrawList()->AddRect(pos, ImVec2(pos.x + size.x, pos.y + size.y), IM_COL32(borderColor.x * 255, borderColor.y * 255, borderColor.z * 255, borderColor.w * 255), 4.0f);

  // Center the text within the rectangle
  ImVec2 textPos = ImVec2(pos.x + padding.x, pos.y + padding.y);
  ImGui::GetWindowDrawList()->AddText(textPos, IM_COL32(textColor.x * 255, textColor.y * 255, textColor.z * 255, textColor.w * 255), label);
}

void GUI::renderUI()
{
  ImVec2 windowSize = ImGui::GetIO().DisplaySize;
  int baseWindowHeight = 120;
  int statsWindowWidth = 125;

  // START GENERAL WINDOW
  ImGui::Begin("GeneralWindow", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar);
  ImGui::SetWindowPos(ImVec2(0, 0));
  ImGui::SetWindowSize(ImVec2(windowSize.x - statsWindowWidth, baseWindowHeight));
  ImGui::SeparatorText("General");

  if(ImGui::Button("Vsync"))
  {
    Application::toggleVsync();
  }

  ImGui::SameLine();
  if(ImGui::Button("Start All"))
  {
    for(auto &instance : instances)
    {
      InstanceState &state = Store::states[instance];

      // Only if it is not already working
      if(!state.working.load())
      {
        util::saveConfig(instance, Store::configs[instance]);
        state.working.store(true);
        std::thread(botThread, std::ref(instance)).detach();
      }
    }
  }

  ImGui::SameLine();
  if(ImGui::Button("Stop All"))
  {
    for(auto &instance : instances)
    {
      InstanceState &state = Store::states[instance];
      state.working.store(false);
    }
  }

  if(ImGui::Button("Arrange 1 4"))
  {
    Emulator::arrange(1, 4);
  }

  ImGui::SameLine();
  if(ImGui::Button("Arrange 1 5"))
  {
    Emulator::arrange(1, 5);
  }

  ImGui::SameLine();
  if(ImGui::Button("Arrange 1 6"))
  {
    Emulator::arrange(1, 6);
  }

    if(ImGui::Button("Arrange 2 4"))
  {
    Emulator::arrange(2, 4);
  }

  ImGui::SameLine();
  if(ImGui::Button("Arrange 2 5"))
  {
    Emulator::arrange(2, 5);
  }

  ImGui::SameLine();
  if(ImGui::Button("Arrange 2 6"))
  {
    Emulator::arrange(2, 6);
  }

  ImGui::End();
  // END GENERAL WINDOW

  // START STATS WINDOW
  ImGui::Begin("StatsWindow", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar);
  ImGui::SetWindowPos(ImVec2(windowSize.x - statsWindowWidth, 0));
  ImGui::SetWindowSize(ImVec2(statsWindowWidth, baseWindowHeight));

  ImGui::SeparatorText("Stats");
  ImGui::Text(std::format("MS: {:.2f}", Application::tempo.ms).c_str());
  ImGui::Text(std::format("FPS: {:.2f}", Application::tempo.fps).c_str());

  ImGui::End();
  // END STATS WINDOW

  // STARTS INSTANCES WINDOW
  ImGui::Begin("InstancesWindow", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar);
  ImGui::SetWindowPos(ImVec2(0, baseWindowHeight));
  ImGui::SetWindowSize(ImVec2(windowSize.x, windowSize.y - baseWindowHeight));
  
  if(!this->instances.empty() && ImGui::BeginTabBar("Tabs"))
  {
    for(size_t i = 0; i < this->instances.size(); i++)
    {
      std::string &instance = this->instances[i];

      if(ImGui::BeginTabItem(instance.c_str()))
      {
        this->statesUI(instance);
        this->actionsUI(instance);

        ImGui::SeparatorText("Routines");
        if(ImGui::BeginTabBar("ActionsTabBar"))
        {
          this->farmUI(instance);
          this->buffsUI(instance);
          this->questsUI(instance);
          this->summaryUI(instance);
          ImGui::EndTabBar();
        }

        ImGui::EndTabItem();
      }
    }

    ImGui::EndTabBar();
  }

  ImGui::End();
  // END INSTANCES WINDOW
}

void GUI::farmUI(const std::string &instance)
{
  WorkConfig &config = Store::configs[instance];

  if(ImGui::BeginTabItem("Farm"))
  {
    InstanceState &state = Store::states[instance];
    ImGui::Checkbox("Enable", &config.farm);

    if(!config.farm)
    {
      ImGui::EndTabItem();
      return;
    }

    ImGui::Checkbox("Reboot", &config.reboot);
    ImGui::SameLine();
    ImGui::Checkbox("Restart", &config.restart);
    ImGui::Checkbox("Collect Pets", &config.pets);
    ImGui::SameLine();
    ImGui::Checkbox("Fishing Like Fisherman", &config.fishing);
    ImGui::SameLine();
    ImGui::Checkbox("Encounter", &config.encounter);

    std::unordered_map<std::string, std::string> &portals = Store::portals;
    std::unordered_map<std::string, std::vector<Monster>> &monsters = Store::monsters;

    std::vector<Monster> &monsterList = monsters[config.selectedPortal];
    
    ImGui::Columns(2, "FarmColumns", false);
    // LEFT COLUMN

    ImGui::PushID(0);
    ImGui::Text("Portal");
    if(ImGui::BeginCombo(" ", portals[config.selectedPortal].c_str()))
    {
      for(auto &[key, portal] : portals)
      {
        const bool isSelected = (config.selectedPortal == key);
        if(ImGui::Selectable(portal.c_str(), isSelected, ImGuiSelectableFlags_SpanAllColumns))
        {
          config.selectedPortal = key;
        }

        if(isSelected)
        {
          ImGui::SetItemDefaultFocus();
        }
      }
      ImGui::EndCombo();
    }
    ImGui::PopID();

    ImGui::PushID(1);
    ImGui::Text("Refresh Mode");
    if(ImGui::BeginCombo(" ", Store::refreshModes[config.refreshMode].c_str()))
    {
      for(size_t i = 0; i < Store::refreshModes.size(); i++)
      {
        const bool isSelected = (config.refreshMode == i);
        if(ImGui::Selectable(Store::refreshModes[i].c_str(), isSelected, ImGuiSelectableFlags_SpanAllColumns))
        {
          config.refreshMode = i;
        }

        if(isSelected)
        {
          ImGui::SetItemDefaultFocus();
        }
      }
      ImGui::EndCombo();
    }
    ImGui::PopID();

    ImGui::NextColumn();

    if(config.selectedMonster >= monsterList.size())
    {
      config.selectedMonster = 0;
    }

    // RIGHT COLUMN
    ImGui::PushID(2);
    ImGui::Text("Monster");
    if(ImGui::BeginCombo(" ", monsterList[config.selectedMonster].displayName.c_str()))
    {
      for(size_t i = 0; i < monsterList.size(); i++)
      {
        const bool isSelected = (config.selectedMonster == i);
        if(ImGui::Selectable(monsterList[i].displayName.c_str(), isSelected, ImGuiSelectableFlags_SpanAllColumns))
        {
          config.selectedMonster = i;
        }

        if(isSelected)
        {
          ImGui::SetItemDefaultFocus();
        }
      }
      ImGui::EndCombo();
    }
    ImGui::PopID();
    ImGui::Columns(1);

    ImGui::EndTabItem();
  }
}

void GUI::buffsUI(const std::string &instance)
{
  WorkConfig &config = Store::configs[instance];

  if(ImGui::BeginTabItem("Buffs"))
  {
    ImGui::Checkbox("Enable", &config.buffs);

    if(!config.buffs)
    {
      ImGui::EndTabItem();
      return;
    }

    ImGui::Spacing();
    ImGui::Columns(2, "BuffsColumns", false);
    selectableList(instance, "Foods", Store::foods, config.selectedBuffs);
    ImGui::NextColumn();
    selectableList(instance, "Potions", Store::potions, config.selectedBuffs);
    ImGui::Columns(1);
    ImGui::EndTabItem();
  }
}

void GUI::questsUI(const std::string & instance)
{
  WorkConfig &config = Store::configs[instance];
  if(ImGui::BeginTabItem("Quests"))
  {
    ImGui::Checkbox("Enable", &config.quests);

    if(!config.quests)
    {
      ImGui::EndTabItem();
      return;
    }

    ImGui::Spacing();
    ImGui::Columns(2, "QuestsColumns", false);
    selectableList(instance, "Scrolls", Store::scrolls, config.selectedQuests);
    ImGui::Columns(1);
    ImGui::EndTabItem();
  }
}

void GUI::statesUI(const std::string &instance)
{
  ImGui::SeparatorText("State");
  InstanceState &state = Store::states[instance];

  if(state.working.load())
  {
    RoundedTag("Bot Working", green, black, green);
  }
  if(!state.working.load())
  {
    RoundedTag("Bot Stopped", red, white, red);
  }
  ImGui::Spacing();
}

void GUI::actionsUI(const std::string &instance)
{
  ImGui::SeparatorText("Actions");
  InstanceState &state = Store::states[instance];
  if(ImGui::Button(state.working.load() ? ICON_FA_STOP : ICON_FA_PLAY) || Application::input.pressed(KEY_F1))
  {
    if(!state.working.load())
    {
      util::saveConfig(instance, Store::configs[instance]);
      state.working.store(true);
      std::thread(botThread, std::ref(instance)).detach();
      return;
    }

    state.working.store(false);
  }
  ImGui::Spacing();
}

void GUI::summaryUI(const std::string &instance)
{
  if(ImGui::BeginTabItem("Summary"))
  {
    if(!Store::states[instance].working.load())
    {
      const char *text = "Bot is not working on this instance";

      ImVec2 textSize = ImGui::CalcTextSize(text);
      ImVec2 contentWindowSize = ImGui::GetContentRegionAvail();
      ImVec2 textPosition = ImVec2((contentWindowSize.x - textSize.x) * 0.5f, (contentWindowSize.y - textSize.y) * 0.5f);

      // Get the current cursor position
      ImVec2 cursorPos = ImGui::GetCursorPos();

      // Set the cursor position to center the text
      ImGui::SetCursorPos(ImVec2(cursorPos.x + textPosition.x, cursorPos.y + textPosition.y));

      ImGui::Text(text);

      ImGui::EndTabItem();
      return;
    }

    Store::mutexes[instance].lock();

    Summary &summary = Store::summaries[instance];

    ImGui::Text(("Time: " + summary.time).c_str());
    ImGui::Text(("MS: " + summary.ms).c_str());
    ImGui::Text(("ActionsPerSecond: " + summary.actionsPerSecond).c_str());
    ImGui::Text(("Routine: " + summary.routine).c_str());
    ImGui::Text(("Location: " + summary.location).c_str());
    ImGui::Text(("Next Action: " + summary.nextAction).c_str());
    ImGui::Text(("Refresh Swords: " + summary.refreshSwords).c_str());
    ImGui::Text(("Refresh Potions: " + summary.refreshPotions).c_str());
    ImGui::Text(("Crashs: " + summary.crashs).c_str());
    ImGui::Text(("Quests Done: " + summary.questsDone).c_str());
    ImGui::Text(("Next Encounter: " + summary.encounterCooldown).c_str());
    ImGui::Text(("Next Fishing: " + summary.fishingCooldown).c_str());
    ImGui::Text(("Next Refresh Mode: " + summary.nextRefreshMode).c_str());

    Store::mutexes[instance].unlock();

    ImGui::EndTabItem();
  }
}

void GUI::selectableList(const std::string &instance, const std::string &name, std::vector<Selectable> &selectableList, std::vector<std::string> &selectedItems)
{
  ImGui::SeparatorText("Chips");
  for(auto &item : selectableList)
  {
    std::vector<std::string>::iterator it = std::find(selectedItems.begin(), selectedItems.end(), item.name);
    bool isSelected = it != selectedItems.end();
    size_t index;
    std::string name = item.displayName;

    if(isSelected)
    {
      index = std::distance(selectedItems.begin(), it);
      name = std::to_string(index + 1) + " - " + name;
    }

    if(ImGui::Selectable(name.c_str(), isSelected))
    {
      // Already exists in the list of selected buffs
      if(isSelected)
      {
        selectedItems.erase(std::remove(selectedItems.begin(), selectedItems.end(), item.name), selectedItems.end());
        continue;
      }

      selectedItems.push_back(item.name);
    }
  }
}

void GUI::loadFonts()
{
  // Merge icons into default tool font
  ImGuiIO& io = ImGui::GetIO();
  io.Fonts->AddFontDefault();

  ImFontConfig config;
  config.MergeMode = true;
  config.GlyphMinAdvanceX = 10.0f; // Use if you want to make the icon monospaced
  static const ImWchar icon_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
  io.Fonts->AddFontFromFileTTF("data/fonts/fontawesome-webfont.ttf", 10.0f, &config, icon_ranges);
}

void GUI::cleanup()
{
  this->destroy();

  // This makes the bot threads close safely
  for(auto &instance : instances)
  {
    InstanceState &state = Store::states[instance];
    state.working.store(false);
  }

  cv::destroyAllWindows();
}

void GUI::init()
{
  ImGuiStyle &style = ImGui::GetStyle();
  style.Colors[ImGuiCol_Tab] = ImVec4(0.20f, 0.20f, 0.20f, 1.0f); // Active tab color

  this->loadFonts();
  this->instances = Emulator::list();
  std::sort(this->instances.begin(), this->instances.end());

  // Initialize the instance states
  for(auto &instance : this->instances)
  {
    Store::states[instance] = InstanceState{false};
    Store::configs[instance] = util::getConfig(instance);
    Store::summaries[instance] = Summary{};
  }
}