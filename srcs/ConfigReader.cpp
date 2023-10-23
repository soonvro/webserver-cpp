#include "ConfigReader.h"

#include <iostream>
#include <sstream>

ConfigReader::ConfigReader(const char* filename) : filename(filename) {}

void ConfigReader::readFile() {
  std::ifstream i(filename);
  if (i.fail()) throw std::invalid_argument("cannot read file");
  bool inHttpBlock = false;
  bool inServerBlock = false;
  bool inLocationBlock = false;
  Host h;
  std::string word;
  enum ReaderState state = kStart;

  while (i >> word) {
    switch (state) {
      case kStart:
        // 주석
        // http 블록
        break;
      case kHttpBlockStart:
        // {
        break;
      case kHttpBlockIn:
        // 주석
        // server 블록
        break;
      case kServerBlockStart:
        // 주석
        // location 블록
        break;
      case kLocationBlockStart:
        // 주석
        break;
      default:
        throw std::runtime_error("invalid syntax : " + word);
    }
  }
}

std::map<std::pair<std::string, int>, Host> ConfigReader::getHosts() {
  return _hosts;
}
Host ConfigReader::getDefaultHost() { return _default_host; }
