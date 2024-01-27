#pragma once

#include <glb/glb.hpp>
#include <filesystem>
#include "work_config.hpp"

namespace util
{
  std::vector<std::string> split(const std::string &str, char delimiter);
  std::string parseCMD(const std::string &cmd);
  std::string parseCMD(std::string cmd, std::vector<std::string> args);
  void saveConfig(const std::string &instance, WorkConfig &config);
  WorkConfig getConfig(const std::string &instance);
};