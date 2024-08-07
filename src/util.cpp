#include "util.hpp"

std::vector<std::string> util::split(const std::string &str, char delimiter)
{
  std::vector<std::string> lines;
  std::stringstream ss(str);
  std::string line;

  while(std::getline(ss, line, delimiter))
  {
    lines.push_back(line);
  }

  return lines;
}

std::string util::parseCMD(const std::string &cmd)
{
  std::string response;

  FILE* pipe = _popen((cmd).c_str(), "r");
  if(pipe != nullptr)
  {
    char buffer[128];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
    {
      response += buffer;
    }
    _pclose(pipe);
  }

  return response;
}

std::string util::parseCMD(std::string cmd, std::vector<std::string> args)
{
  for(auto &arg : args) {
    cmd += " \"" + arg + "\"";
  }

  return parseCMD(cmd);
}

void util::saveConfig(const std::string &instance, WorkConfig &config)
{
  std::filesystem::path configPath = std::filesystem::path(getenv("USERPROFILE")) / ".corahbot";

  // Load config
  try
  {
    // Create the config folder if it does not exists
    if(!std::filesystem::is_directory(configPath))
    {
      std::filesystem::create_directory(configPath);
      std::filesystem::create_directory(std::filesystem::path(configPath / "instances"));
    }

    std::filesystem::path instanceFile = std::filesystem::path(configPath / "instances" / (instance + ".json"));
    std::ofstream file(instanceFile);

    if(file.is_open())
    {
      file << config.toJson();
      file.close();
    }
  }
  catch(const std::exception& e)
  {
    std::cerr << e.what() << '\n';
  }
}

WorkConfig util::getConfig(const std::string &instance)
{
  WorkConfig workConfig{};
  std::filesystem::path configPath = std::filesystem::path(getenv("USERPROFILE")) / ".corahbot";
  // Load config
  try
  {
    // Create the config folder if it does not exists
    if(!std::filesystem::is_directory(configPath))
    {
      std::filesystem::create_directory(configPath);
      std::filesystem::create_directory(std::filesystem::path(configPath / "instances"));
    }

    std::filesystem::path instanceFile = std::filesystem::path(configPath / "instances" / (instance + ".json"));
    std::ifstream file(instanceFile);

    if(file.is_open())
    {
      LOGGER_DEBUG("Loading config for instance: {}", instance);
      std::ostringstream ss;
      ss << file.rdbuf();

      workConfig = WorkConfig(ss.str());

      file.close();
    }
  }
  catch(const std::exception& e)
  {
    std::cerr << e.what() << '\n';
  }

  return workConfig;
}

std::string util::trim(const std::string &str)
{
  size_t first = str.find_first_not_of(" \t\n\r");
  size_t last = str.find_last_not_of(" \t\n\r");

  if (first == std::string::npos || last == std::string::npos) {
    // The string is empty or contains only whitespaces
    return "";
  }

  return str.substr(first, last - first + 1);
}
