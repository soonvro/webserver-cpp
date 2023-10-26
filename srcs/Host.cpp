#include "Host.hpp"

Host::Host() : _name(""), _port(-1) {}

const std::string& Host::getName() const { return _name; }

void Host::setName(const std::string& name) { _name = name; }

int Host::getPort() const { return _port; }

void Host::setPort(int port) { _port = port; }

const std::map<std::string, RouteRule>& Host::getRouteRules() const {
  return _route_rules;
}

void Host::setRouteRules(const std::map<std::string, RouteRule>& routeRules) {
  _route_rules = routeRules;
}

void Host::addRouteRule(const std::string& route, const RouteRule& rule) {
  _route_rules[route] = rule;
}

const RouteRule& Host::getRouteRule(const std::string& route) const {
  //prefix
  std::map<std::string, RouteRule>::const_iterator it = _route_rules.begin();
  const RouteRule* rule = 0;
  while (it != _route_rules.end()) {
    if (route.compare(0, it->first.size(), it->first) == 0 \
    && (!rule || rule->getRoute().size() < it->first.size())) {
      rule = &(it->second);
    }
    it++;
  }

  //suffix
  it = _route_rules.begin();
  while (it != _route_rules.end()) {
    if (it->first.size() <= route.size() && \
    route.compare(route.size() - it->first.size(), it->first.size(), it->first) == 0 \
    && (!rule || rule->getRoute().size() < it->first.size() || !rule->getIsCgi())) {
      rule = &(it->second);
    }
    it++;
  }
  if (rule) return *rule;
  throw std::runtime_error("Error: route rule not found.");
}