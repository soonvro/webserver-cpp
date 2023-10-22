#ifndef CONFIGREADER_H__
#define CONFIGREADER_H__

#include <Host.hpp>
#include <fstream>
#include <map>
#include <string>

enum ReaderState {
  kMainBlockStart,
  kMainBlockEnd,

  kHttpBlockStart,
  kHttpBlockEnd,

  kServerBlockStart,
  kServerBlockEnd,

  kLocationBlockStart,
  kLocationBlockEnd,

};

class ConfigReader {
 public:
  ConfigReader(const char* filename);

  void readFile();

  std::map<std::pair<std::string, int>, Host> getHosts();
  Host getDefaultHost();

 private:
  Host _default_host;
  std::map<std::pair<std::string, int>, Host> _hosts;
  std::string filename;

  enum ReaderState _state;
};

#endif