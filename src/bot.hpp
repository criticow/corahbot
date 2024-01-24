#pragma once

#include <glb/glb.hpp>
#include <Windows.h>
#include "emulator.hpp"
#include "store.hpp"

#define BOT_ACTION_NONE "none"
#define BOT_ACTION_REFRESH_SWORDS "refresh_swords"
#define BOT_ACTION_REFRESH_POTIONS "refresh_potions"

#define BOT_ROUTINE_NONE "none"
#define BOT_ROUTINE_FARM "farm"
#define BOT_ROUTINE_COMBINE "combine"

#define BOT_LOCATION_UNKNOWN "unknown"
#define BOT_LOCATION_FIGHTING "fighting"
#define BOT_LOCATION_MENU_RIGHT "menu_right"
#define BOT_LOCATION_MENU_LEFT "menu_left"
#define BOT_LOCATION_LOGIN "login"
#define BOT_LOCATION_HOME "home"
#define BOT_LOCATION_REFILL_MENU "refill_menu"
#define BOT_LOCATION_MAP "map"

class Bot
{
  public:
  Bot(){};
  void run(const std::string &instance);

  private:
  Temporizer tempo;

  std::string findLocation(const std::string &instance);
  void handleFighting(const std::string &instance, std::string &currentAction, int &swords, int &potions);
  void handleMenuLeft(const std::string &instance, std::string &currentAction);
  void handleLogin(const std::string &instance, std::string &currentAction);
  void handleHome(const std::string &instance, std::string &currentAction);
  void handleRefillMenu(const std::string &instance, std::string &currentAction);
  void handleMap(const std::string &instance, std::string &currentAction);
};