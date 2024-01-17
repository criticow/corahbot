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
  std::filesystem::path tmpFile = std::filesystem::temp_directory_path() / (Random::UUID() + ".txt");

  int res = std::system((cmd + " > " + tmpFile.string()).c_str());

  if(res == 0)
  {
    std::ifstream file(tmpFile);
    if(file.is_open())
    {
      std::ostringstream sstream;
      sstream << file.rdbuf();
      response = sstream.str();
      file.close();
    }
    std::filesystem::remove(tmpFile);
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
