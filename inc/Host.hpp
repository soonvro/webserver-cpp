#ifndef HOST_H_
#define HOST_H_

#include <string>
#include <map>
#include <utility>
#include "RouteRule.hpp"

class Host{
  private:
  std::string _name;
  
  int       _port;

  std::map<int, std::string>  _default_error_pages;

  size_t _max_client_body_size;

  std::map<std::string, RouteRule>  _route_rules;

  std::pair<int, std::string>  _redirection;

  public:
  const std::string& getName() const;
  void setName(const std::string& name);

  int getPort() const;
  void setPort(int port);

  const std::map<int, std::string>& getDefaultErrorPages() const;
  void setDefaultErrorPages(const std::map<int, std::string>& errorPages);

  size_t getMaxClientBodySize() const;
  void setMaxClientBodySize(size_t maxSize);

  const std::map<std::string, RouteRule>& getRouteRules() const;
  void setRouteRules(const std::map<std::string, RouteRule>& routeRules);

  const std::pair<int, std::string>& getRedirection() const;
  void setRedirection(const std::pair<int, std::string>& redirection);
};

#endif