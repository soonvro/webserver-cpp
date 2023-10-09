#include "Host.hpp"

// Getter and Setter for _name
const std::string& Host::getName() const {
    return _name;
}

void Host::setName(const std::string& name) {
    _name = name;
}

// Getter and Setter for _port
int Host::getPort() const {
    return _port;
}

void Host::setPort(int port) {
    _port = port;
}

// Getter and Setter for _default_error_pages
const std::map<int, std::string>& Host::getDefaultErrorPages() const {
    return _default_error_pages;
}

void Host::setDefaultErrorPages(const std::map<int, std::string>& errorPages) {
    _default_error_pages = errorPages;
}

// Getter and Setter for _max_client_body_size
size_t Host::getMaxClientBodySize() const {
    return _max_client_body_size;
}

void Host::setMaxClientBodySize(size_t maxSize) {
    _max_client_body_size = maxSize;
}

// Getter and Setter for _route_rules
const std::map<std::string, RouteRule>& Host::getRouteRules() const {
    return _route_rules;
}

void Host::setRouteRules(const std::map<std::string, RouteRule>& routeRules) {
    _route_rules = routeRules;
}

// Getter and Setter for _redirection
const std::pair<int, std::string>& Host::getRedirection() const {
    return _redirection;
}

void Host::setRedirection(const std::pair<int, std::string>& redirection) {
    _redirection = redirection;
}
