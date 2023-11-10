#ifndef CONFIGREADER_H__
#define CONFIGREADER_H__

#include <fstream>
#include <map>
#include <string>

#include "Host.hpp"

enum ReaderState {
  kMainStart,

  kHttpBlockStart,
  kHttpBlockIn,
  kHttpBlockEnd,

  kServerBlockStart,
  kServerBlockIn,
  kServerBlockEnd,

  kLocationBlockStart,
  kLocationBlockIn,
  kLocationBlockEnd,

  kEnd,

  kDead,
};

class ConfigReader {
 public:
  ConfigReader(const char* filename);

  void                                        readFile();

  std::map<std::pair<std::string, int>, Host> getHosts();
  Host                                        getDefaultHost();

 private:
  Host                                        _default_host;
  std::map<std::pair<std::string, int>, Host> _hosts;
  std::string                                 _filename;

  enum ReaderState                            _state;

  void                                        onStart(std::string word, std::ifstream& config);
  void                                        onBlockStart(std::string word, enum ReaderState state);
  void                                        onHttpBlockIn(std::string word, std::ifstream& config);
  void                                        onServerBlockIn(std::string word, std::ifstream& config);
  void                                        onHttpBlockEnd(std::string word, std::ifstream& config);
  void                                        onServerBlockEnd(std::string word, std::ifstream& config);

  void                                        addHost(const Host& host);
  void                                        handleLocationBlock(std::string word, std::ifstream& config, Host& host);
};

#endif