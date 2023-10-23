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

bool Host::hasRouteRule(const std::string& route) const {
  return _route_rules.find(route) != _route_rules.end();
}