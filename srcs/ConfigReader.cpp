#include "ConfigReader.h"

ConfigReader::ConfigReader(const char* filename) : filename(filename) {}

void ConfigReader::readFile() {
  std::ifstream i(filename);
  if (i.fail()) throw std::invalid_argument("cannot read file");
  bool inHttpBlock = false;
  bool inServerBlock = false;
  bool inLocationBlock = false;
  Host h;
  // loop:
  // get line
  // trim white space
  // if comment
  //  loop
  //  jmp to state
  //  if match handle key
  //  else exception
}

std::map<std::pair<std::string, int>, Host> ConfigReader::getHosts() {
  return _hosts;
}
Host ConfigReader::getDefaultHost() { return _default_host; }
