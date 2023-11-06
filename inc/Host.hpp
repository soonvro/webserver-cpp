#ifndef HOST_HPP_
#define HOST_HPP_

#include <map>
#include <string>
#include <utility>
#include <stdexcept>

#include "RouteRule.hpp"

class Host {
 private:
  std::string                       _name;

  int                               _port;

  std::map<std::string, RouteRule>  _route_rules;



 public:
  Host();

  const std::string&                      getName() const;
  int                                     getPort() const;
  const std::map<std::string, RouteRule>& getRouteRules() const;
  const RouteRule*                        getRouteRule(const std::string& route) const;

  void                                    setName(const std::string& name);
  void                                    setPort(int port);

  void setRouteRules(const std::map<std::string, RouteRule>& routeRules);

  void addRouteRule(const std::string& route, const RouteRule& rule);
};

#endif
