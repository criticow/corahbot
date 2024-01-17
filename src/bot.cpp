#include "bot.hpp"

void Bot::run(std::mutex &instanceMutex, Instance &instance, Emulator &emulator)
{
  std::string windowName = std::format("{} - {}", instance.index, Random::UUID());

  while(true)
  {
  }
}
