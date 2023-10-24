#ifndef HOST_H_
#define HOST_H_

#include <map>
#include <string>
#include <utility>

#include "RouteRule.hpp"

class Host {
 private:
  std::string _name;

  int _port;

  std::map<std::string, RouteRule> _route_rules;

 public:
  Host();

  const std::string& getName() const;
  void setName(const std::string& name);

  int getPort() const;
  void setPort(int port);

  const std::map<std::string, RouteRule>& getRouteRules() const;
  void setRouteRules(const std::map<std::string, RouteRule>& routeRules);

  void addRouteRule(const std::string& route, const RouteRule& rule);

  const RouteRule& getRouteRule(const std::string& route) const;
};

#endif