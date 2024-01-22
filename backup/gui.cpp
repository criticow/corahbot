#include "gui.hpp"
#include "application.hpp"

std::unordered_map<int, std::mutex> mutexes;
std::unordered_map<int, std::thread> threads;

void GUI::botThread(Instance &instance, Emulator &emulator)
{
  Bot bot;
  bot.run(mutexes[instance.index], instance, emulator);
}

void GUI::renderUI()
{
  ASSERT(this->app, "App is not set");

  ImVec2 windowSize = ImGui::GetIO().DisplaySize;

  ImGui::Begin("BaseWindow", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar);

  ImGui::SetWindowPos(ImVec2(0, 0));
  ImGui::SetWindowSize(windowSize);

  if(ImGui::BeginTabBar("Tabs"))
  {
    if(ImGui::BeginTabItem("Main"))
    {
      this->mainUI();
      ImGui::EndTabItem();
    }
    if(ImGui::BeginTabItem("Automation"))
    {
      this->automationUI();
      ImGui::EndTabItem();
    }
    ImGui::EndTabBar();
  }

  ImGui::End();
}

void GUI::addFonts()
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
  for(auto &[index, thread] : threads)
  {
    if(thread.joinable())
    {
      LOGGER_DEBUG("Joining tread from destroy");
      thread.join();
    }
  }

  cv::destroyAllWindows();
}


void GUI::mainUI()
{
}

void GUI::automationUI()
{
  Emulator &emulator = this->app->emulator;

  emulator.list();

  if (ImGui::BeginTabBar("Instances"))
  {
    for (auto &instance : emulator.instances)
    {
      mutexes[instance.index].lock();
      if (!instance.isRunning)
      {
        mutexes[instance.index].unlock();
        continue;
      }
      mutexes[instance.index].unlock();

      ImGui::PushID(instance.index);

      if (ImGui::BeginTabItem(instance.name.c_str()))
      {
        mutexes[instance.index].lock();
        if (ImGui::Button(instance.isWorking ? "Stop Bot" : "Start Bot"))
        {
          // Start
          if(!instance.isWorking)
          {
            instance.isWorking = true;
            // Cleanup the thread if it already exists
            if(threads.find(instance.index) != threads.end())
            {
              std::thread &td = threads[instance.index];
              if(td.joinable())
              {
                td.join();
              }
              threads.erase(instance.index);
            }
            // Create new thread for the bot
            threads[instance.index] = std::thread(&GUI::botThread, this, std::ref(instance), std::ref(emulator));
          }
          // Stop
          else
          {
            instance.isWorking = false;
            instance.isDebugging = false;

            // Thread cleanup
            if(threads.find(instance.index) != threads.end())
            {
              std::thread &td = threads[instance.index];
              if(td.joinable())
              {
                td.join();
              }

              threads.erase(instance.index);
            }
          }
        }
        mutexes[instance.index].unlock();

        mutexes[instance.index].lock();
        ImGui::BeginDisabled(!instance.isWorking);
        if(ImGui::Button(instance.isDebugging ? "Close CV" : "Open CV"))
        {
          instance.isDebugging = !instance.isDebugging;
        }
        ImGui::EndDisabled();
        mutexes[instance.index].unlock();

        if(ImGui::Button("ScreenShot"))
        {
          cv::Mat image = emulator.printscreen(instance.name);
          std::string name = Random::UUID() + ".png";
          cv::imwrite(name, image);
        }

        ImGui::EndTabItem();
      }

      ImGui::PopID();
    }

    ImGui::EndTabBar();
  }
}