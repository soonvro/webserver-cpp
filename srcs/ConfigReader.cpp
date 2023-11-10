#include <sstream>

#include "ConfigReader.hpp"
#include "HttpDecoderEnums.h"


ConfigReader::ConfigReader(const char* filename) : filename(filename) {}

void ConfigReader::onStart(std::string word, std::ifstream& config) {
  if (word == "http") {
    _state = kHttpBlockStart;
  } else if (word[0] == '#') {
    std::getline(config, word);
  } else {
    _state = kDead;
  }
}

void ConfigReader::onHttpBlockIn(std::string word, std::ifstream& config) {
  if (word == "server") {
    _state = kServerBlockStart;
  } else if (word == "}") {
    _state = kHttpBlockEnd;
  } else if (word[0] == '#') {
    std::getline(config, word);
  } else {
    _state = kDead;
  }
}

void ConfigReader::onBlockStart(std::string word, enum ReaderState state) {
  if (word == "{") {
    _state = state;
  } else {
    _state = kDead;
  }
}

void ConfigReader::handleLocationBlock(std::string word, std::ifstream& config,
                                       Host& h) {
  enum locationState {
    kLocationStart,
    kLocationPath,
    kLocationCgiPath,
  };

  RouteRule r;

  enum locationState _locationState = kLocationStart;

  std::string tmp;

  bool is_client_limit_set = false;
  bool is_auto_index_set = false;

  while (config >> word) {
    switch (_locationState) {
      case kLocationStart:
        if (word == "~") {
          r.setIsCgi(true);
          config >> word;
          r.setRoute(word);
          if (config >> word && word == "{") {
            _locationState = kLocationCgiPath;
          } else {
            _state = kDead;
            return;
          }
        } else {
          r.setIsCgi(false);
          r.setRoute(word);
          if (config >> word && word == "{") {
            _locationState = kLocationPath;
          } else {
            _state = kDead;
            return;
          }
        }
        break;
      case kLocationCgiPath:
      case kLocationPath:
        if (word == "root") {
          if (config >> word && word[word.size() - 1] == ';' && r.getLocation() == "") {
            word.erase(word.size() - 1);
            r.setLocation(word);
          } else {
            _state = kDead;
            return;
          }
        } else if (word == "index") {
          if (config >> word && word[word.size() - 1] == ';' && r.getIndexPage() == "") {
            word.erase(word.size() - 1);
            r.setIndexPage(word);
          } else {
            _state = kDead;
            return;
          }
        } else if (word == "client_max_body_size") {
          size_t size;
          if (config >> size >> word && word == ";" &&
              r.getIsClientBodySizeSet() == false) {
            r.setMaxClientBodySize(size);
            r.setIsClientBodySizeSet(true);
          } else {
            _state = kDead;
            return;
          }
        } else if (word == "return") {
          int code;
          if (config >> code >> word && word[word.size() - 1] == ';' &&
              !r.hasRedirection(code)) {
            word.erase(word.size() - 1);
            r.addRedirection(code, word);
          } else {
            _state = kDead;
            return;
          }
        } else if (word == "error_page") {
          int code;
          if (config >> code >> word && word[word.size() - 1] == ';' &&
              !r.hasErrorPage(code) && code >= 300 && code < 600) {
            word.erase(word.size() - 1);
            r.addErrorPage(code, word);
          } else {
            _state = kDead;
            return;
          }
        } else if (word == "autoindex") {
          if (!is_auto_index_set && config >> word &&
              (word == "on;" || word == "off;")) {
            r.setAutoIndex(word == "on;");
            is_auto_index_set = true;
          } else {
            _state = kDead;
            return;
          }
        } else if (word == "cgi_root") {
          if (config >> word && word[word.size() - 1] == ';' && r.getCgiPath() == "") {
            word.erase(word.size() - 1);
            r.setCgiPath(word);
          } else {
            _state = kDead;
            return;
          }
        } else if (word == "limit_except") {
          if (is_client_limit_set) {
            _state = kDead;
            return;
          }
          is_client_limit_set = true;
          int methods = 0;
          while (config >> word) {
            if (word == "{") break;
            else if (word == "GET")
              methods |= 1 << (HPS::kGET);
            else if (word == "HEAD")
              methods |= 1 << (HPS::kHEAD);
            else if (word == "POST")
              methods |= 1 << (HPS::kPOST);
            else if (word == "DELETE")
              methods |= 1 << (HPS::kDELETE);
            else {
              _state = kDead;
              return;
            }
          }
          if (!(config >> word) || word != "deny" || !(config >> word) ||
              word != "all;" || !(config >> word) || word != "}") {
            _state = kDead;
            return;
          }
          r.setAcceptedMethods(methods);
        } else if (word == "}") {
          _state = kServerBlockIn;
          if (h.getRouteRules().find(r.getRoute()) != h.getRouteRules().end()) {
            _state = kDead;
            return;
          }
          h.addRouteRule(r.getRoute(), r);
          return;
        } else if (word[0] == '#'){
          std::getline(config, word);
        }else {
          _state = kDead;
          return;
        }
        break;
      default:
        _state = kDead;
        return;
    }
  }

  // 중복 루트
  if (h.getRouteRules().find(r.getRoute()) != h.getRouteRules().end()) {
    _state = kDead;
    return;
  } else {
    h.addRouteRule(r.getRoute(), r);
  }
}

void ConfigReader::onServerBlockIn(std::string word, std::ifstream& config) {
  Host h;
  while (_state != kDead) {
    if (word == "listen") {
      int port;

      if (!(config >> port >> word) || word != ";" || port < 0 ||
          port > 65535 || config.fail() || h.getPort() != -1) {
        _state = kDead;
        return;
      }
      h.setPort(port);
    } else if (word == "server_name") {
      if (!(config >> word) || word[word.size() - 1] != ';' || config.fail() ||
          h.getName() != "") {
        _state = kDead;
        return;
      }
      word.erase(word.size() - 1);
      h.setName(word);
    } else if (word == "location") {
      handleLocationBlock(word, config, h);
    } else if (word == "}") {
      _state = kServerBlockEnd;
      addHost(h);
      return;
    } else if (word[0] == '#') {
      std::getline(config, word);
    } else{
      _state = kDead;
      return;
    }
    if (!(config >> word)) {
      _state = kDead;
      return;
    }
  }
}

void ConfigReader::onHttpBlockEnd(std::string word, std::ifstream& config) {
  if (word[0] == '#') {
    std::getline(config, word);
    _state = kEnd;
  } else {
    _state = kDead;
  }
}

void ConfigReader::onServerBlockEnd(std::string word, std::ifstream& config) {
  if (word[0] == '#') {
    std::getline(config, word);
  } else if (word == "server") {
    _state = kServerBlockStart;
  } else if (word == "}") {
    _state = kHttpBlockEnd;
  } else {
    _state = kDead;
  }
}

void ConfigReader::readFile() {
  std::ifstream i(filename.c_str());
  if (i.fail()) throw std::invalid_argument("cannot read file");
  std::string word;
  _state = kMainStart;

  while (i >> word) {
    switch (_state) {
      case kMainStart:
        onStart(word, i);
        break;
      case kHttpBlockStart:
        onBlockStart(word, kHttpBlockIn);
        break;
      case kServerBlockStart:
        onBlockStart(word, kServerBlockIn);
        break;
      case kLocationBlockStart:
        onBlockStart(word, kLocationBlockIn);
        break;
      case kHttpBlockIn:
        onHttpBlockIn(word, i);
        break;
      case kServerBlockIn:
        onServerBlockIn(word, i);
        break;
      case kHttpBlockEnd:
        onHttpBlockEnd(word, i);
        break;
      case kServerBlockEnd:
        onServerBlockEnd(word, i);
        break;
      case kEnd:
        if (word[0] == '#') {
          std::getline(i, word);
        } else {
          _state = kDead;
        }
        break;
      case kDead:
      default:
        throw std::runtime_error("invalid syntax");
    }
  }
  if (kEnd != _state && kHttpBlockEnd != _state)
    throw std::runtime_error("invalid syntax");
  if (_hosts.size() == 0) throw std::runtime_error("no hosts");
}

std::map<std::pair<std::string, int>, Host> ConfigReader::getHosts() {
  return _hosts;
}
Host ConfigReader::getDefaultHost() { return _default_host; }

void ConfigReader::addHost(const Host& host) {
  if (_hosts.size() == 0) _default_host = host;
  if (_hosts.find(std::make_pair(host.getName(), host.getPort())) !=
      _hosts.end())
    throw std::runtime_error("duplicate host");
  _hosts[std::make_pair(host.getName(), host.getPort())] = host;
}
