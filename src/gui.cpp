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
  int baseWindowHeight = 100;
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
  
  if(!this->instances.empty() && ImGui::BeginTabBar("Tabs", ImGuiTabBarFlags_Reorderable))
  {
    for(size_t i = 0; i < this->instances.size(); i++)
    {
      std::string &instance = this->instances[i];
      ImGui::PushID(i);

      if(ImGui::BeginTabItem(instance.c_str()))
      {
        this->statesUI(instance);
        this->actionsUI(instance);

        ImGui::SeparatorText("Routines");
        if(ImGui::BeginTabBar("ActionsTabBar"))
        {
          this->farmUI(instance);
          this->combineUI(instance);
          this->summaryUI(instance);
          ImGui::EndTabBar();
        }

        ImGui::EndTabItem();
      }
      ImGui::PopID();
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

    std::unordered_map<std::string, std::string> &portals = Store::portals;
    std::unordered_map<std::string, std::vector<Monster>> &monsters = Store::monsters;

    std::vector<Monster> &monsterList = monsters[config.selectedPortal];
    
    ImGui::SetNextItemWidth(35);
    ImGui::InputInt("Swords Threshold", &config.swordsThreshold, 0, 50);
    config.swordsThreshold = std::clamp(config.swordsThreshold, 0, 50);

    ImGui::SameLine();
    ImGui::SetNextItemWidth(35);
    ImGui::InputInt("Potions Threshold", &config.potionsThreshold, 0, 29);
    config.potionsThreshold = std::clamp(config.potionsThreshold, 0, 29);

    ImGui::PushID(0);
    ImGui::Text("Portal");
    if(ImGui::BeginCombo(" ", portals[config.selectedPortal].c_str(), ImGuiComboFlags_WidthFitPreview))
    {
      for(auto &[key, portal] : portals)
      {
        const bool isSelected = (config.selectedPortal == key);
        if(ImGui::Selectable(portal.c_str(), isSelected))
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
    ImGui::Text("Monster");
    if(ImGui::BeginCombo(" ", monsterList[config.selectedMonster].displayName.c_str(), ImGuiComboFlags_WidthFitPreview))
    {
      for(size_t i = 0; i < monsterList.size(); i++)
      {
        const bool isSelected = (config.selectedMonster == i);
        if(ImGui::Selectable(monsterList[i].displayName.c_str(), isSelected))
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

    ImGui::EndTabItem();
  }
}

void GUI::combineUI(const std::string &instance)
{
  WorkConfig &config = Store::configs[instance];
  if(ImGui::BeginTabItem("Combine"))
  {
    ImGui::Checkbox("Enable", &config.combine);

    if(!config.combine)
    {
      ImGui::EndTabItem();
      return;
    }

    int columns = 6;
    ImGui::BeginTable("GemsTable", columns, ImGuiTableFlags_Borders);

    for(size_t i = 0; i < columns; i++)
    {
      if(i % 2 == 0)
      {
        ImGui::TableSetupColumn("Gem", ImGuiTableColumnFlags_None);
        continue;
      }

      ImGui::TableSetupColumn("Chip", ImGuiTableColumnFlags_None);
    }

    ImGui::TableHeadersRow();
    
    int counter = 0;
    for(auto &gem : config.gems)
    {
      if(counter == 0)
      {
        ImGui::TableNextRow();
      }

      ImGui::TableSetColumnIndex(counter);
      counter++;
      ImGui::PushStyleColor(ImGuiCol_Text, gem.color);
      if(ImGui::Selectable(gem.name.c_str(), gem.selected))
      {
        gem.selected = !gem.selected;

        if(gem.selected)
        {
          config.selectedGems.push_back(gem.name);
        }
        else
        {
          config.selectedGems.erase(
            std::remove(config.selectedGems.begin(), config.selectedGems.end(), gem.name), config.selectedGems.end()
          );
        }
      }
      ImGui::PopStyleColor();

      if(counter == columns)
      {
        counter = 0;
      }
    }
    ImGui::EndTable();

    if(config.selectedGems.empty())
    {
      ImGui::EndTabItem();
      ImGui::EndDisabled();
      return;
    }

    // Display the selected items in the order they were selected
    ImGui::Spacing();
    ImGui::Spacing();

    ImGui::Text("Combine priority:");

    ImGui::Spacing();
    ImGui::Separator();


    ImGui::Columns(4, nullptr, false);

    int rows = 6;
    for(size_t i = 1; i <= config.selectedGems.size(); i++) {
      std::string prioNumber = i < 10 ? "0" + std::to_string(i) : std::to_string(i);
      ImGui::Text(std::format("{} - {}", prioNumber, config.selectedGems[i - 1]).c_str());

      if(i % rows == 0)
      {
        ImGui::NextColumn();
      }
    }

    ImGui::Columns(1, nullptr, false);

    ImGui::EndTabItem();
  }
}

void GUI::statesUI(const std::string &instance)
{
  ImGui::SeparatorText("State");
  InstanceState &state = Store::states[instance];

  if(state.open.load())
  {
    RoundedTag("Window Open", green, black, green);
  }
  if(!state.open.load())
  {
    RoundedTag("Window Closed", red, white, red);
  }
  if(state.working.load())
  {
    ImGui::SameLine();
    RoundedTag("Bot Working", green, black, green);
  }
  if(!state.working.load())
  {
    ImGui::SameLine();
    RoundedTag("Bot Stopped", red, white, red);
  }
  if(!state.minimized.load())
  {
    ImGui::SameLine();
    RoundedTag("Window Acessible", green, black, green);
  }
  if(state.minimized.load())
  {
    ImGui::SameLine();
    RoundedTag("Window Minimized", red, white, red);
  }
  ImGui::Spacing();
}

void GUI::actionsUI(const std::string &instance)
{
  ImGui::SeparatorText("Actions");
  InstanceState &state = Store::states[instance];
  ImGui::BeginDisabled(state.working.load());
  if(ImGui::Button(ICON_FA_PLAY) || Application::input.pressed(ACTION_ACCEPT))
  {
    state.working.store(true);
    std::thread(botThread, std::ref(instance)).detach();
  }
  ImGui::EndDisabled();

  ImGui::SameLine();
  ImGui::BeginDisabled(!state.working.load());
  if(ImGui::Button(ICON_FA_STOP))
  {
    state.working.store(false);
  }
  ImGui::EndDisabled();

  ImGui::Spacing();
}

void GUI::summaryUI(const std::string &instance)
{
  #ifndef NDEBUG
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
    ImGui::Text(("Swords: " + summary.swords).c_str());
    ImGui::Text(("Potions: " + summary.potions).c_str());

    Store::mutexes[instance].unlock();

    ImGui::EndTabItem();
  }
  #endif
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
    state.open.store(false);
    state.working.store(false);
    state.minimized.store(true);
  }

  cv::destroyAllWindows();
}

void GUI::update()
{
  if(Application::input.pressed(KEY_1)) {currentTab = 0; LOGGER_DEBUG("1");}
  if(Application::input.pressed(KEY_2)) {currentTab = 1;}
  if(Application::input.pressed(KEY_3)) {currentTab = 2;}
  if(Application::input.pressed(KEY_4)) {currentTab = 3;}
  if(Application::input.pressed(KEY_5)) {currentTab = 4;}
  if(Application::input.pressed(KEY_6)) {currentTab = 5;}
  if(Application::input.pressed(KEY_7)) {currentTab = 6;}
  if(Application::input.pressed(KEY_8)) {currentTab = 7;}
  if(Application::input.pressed(KEY_9)) {currentTab = 8;}

  // if(this->tempo.hasPassed("InstancesUpdate", 500))
  // {
  //   std::vector<std::string> instanceNames = Emulator::list();

  //   for(auto &instance : instanceNames)
  //   {
  //     // Insert the new item on the list only if does not already exists
  //     if(std::find(this->instances.begin(), this->instances.end(), instance) == this->instances.end())
  //     {
  //       this->instances.push_back(instance);
  //       Store::states[instance] = InstanceState{true, false, false};
  //       Store::configs[instance] = WorkConfig{};
  //       Store::summaries[instance] = Summary{};
  //     }
  //   }
  // }

  // Update the instance states, check if the window is closed or minimized
  for(auto &instance : this->instances)
  {
    HWND hwnd = FindWindow(nullptr, instance.c_str());
    InstanceState &state = Store::states[instance];

    // Window not found, instance is closed
    if(!hwnd)
    {
      state.open.store(false);
      state.working.store(false);
      continue;
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
      continue;
    }

    state.minimized.store(false);
  }
}

void GUI::init()
{
  ImGuiStyle &style = ImGui::GetStyle();
  style.Colors[ImGuiCol_Tab] = ImVec4(0.5f, 0.0f, 0.0f, 0.7f); // Active tab color

  this->loadFonts();
  this->instances = Emulator::list();
  std::sort(this->instances.begin(), this->instances.end());

  // Initialize the instance states
  for(auto &instance : this->instances)
  {
    Store::states[instance] = InstanceState{true, false, false};
    Store::configs[instance] = WorkConfig{};
    Store::summaries[instance] = Summary{};
  }
}
