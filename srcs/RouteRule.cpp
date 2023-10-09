#include "RouteRule.hpp"

const std::string& RouteRule::getRoute() const {
    return _route;
}

void RouteRule::setRoute(const std::string& route) {
    _route = route;
}

const std::string& RouteRule::getLocation() const {
    return _location;
}

void RouteRule::setLocation(const std::string& location) {
    _location = location;
}

int RouteRule::getAcceptedMethods() const {
    return _accepted_methods;
}

void RouteRule::setAcceptedMethods(int methods) {
    _accepted_methods = methods;
}

const std::pair<int, std::string>& RouteRule::getRedirection() const {
    return _redirection;
}

void RouteRule::setRedirection(const std::pair<int, std::string>& redirection) {
    _redirection = redirection;
}

bool RouteRule::isDirectoryListingEnabled() const {
    return _directory_listing;
}

void RouteRule::setDirectoryListing(bool enable) {
    _directory_listing = enable;
}

const std::string& RouteRule::getDefaultResponseForDirectory() const {
    return _default_response_for_directory;
}

void RouteRule::setDefaultResponseForDirectory(const std::string& response) {
    _default_response_for_directory = response;
}
