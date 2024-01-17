#pragma once

#include <glb/glb.hpp>
#include <Windows.h>
#include "emulator.hpp"

class Bot
{
  public:
  Bot(){};
  void run(std::mutex &instanceMutex, Instance &instance, Emulator &emulator);
};