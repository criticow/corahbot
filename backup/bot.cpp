#include "bot.hpp"

void Bot::run(std::mutex &instanceMutex, Instance &instance, Emulator &emulator)
{
  std::string windowName = std::format("{} - {}", instance.index, Random::UUID());

  while(true)
  {
    instanceMutex.lock();
    cv::Mat image = emulator.printscreen(instance.name);

    if(image.empty() || !instance.isWorking)
    {
      instance.isDebugging = false;
      instance.isWorking = false;
      instanceMutex.unlock();
      break;
    }

    if(instance.isDebugging)
    {
      cv::namedWindow(windowName);
      cv::imshow(windowName, image);
      int key = cv::waitKey(1);

      if(key == 27 || cv::getWindowProperty(windowName, cv::WND_PROP_VISIBLE) < 1)
      {
        instance.isDebugging = false;
      }
    }
    else if (!instance.isDebugging)
    {
      if(FindWindow(nullptr, windowName.c_str()))
      {
        cv::destroyWindow(windowName);
      }
    }

    instanceMutex.unlock();
  }

  if(FindWindow(nullptr, windowName.c_str()))
  {
    cv::destroyWindow(windowName);
  }
}
