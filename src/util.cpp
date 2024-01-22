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
