#pragma once

#include <glb/glb.hpp>
#include <Windows.h>
#include "emulator.hpp"
#include "store.hpp"


class Bot
{
  public:
  Bot(){};
  void run(const std::string &instance);

  private:
  Temporizer tempo;

  std::string findLocation(const std::string &instance);
  void handleFighting(const std::string &instance, int &swords, int &potions);
};