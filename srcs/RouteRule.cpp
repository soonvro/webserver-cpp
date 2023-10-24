#include "RouteRule.hpp"

#include "HttpDecoderEnums.h"

RouteRule::RouteRule()
    : _accepted_methods(0xFFFFFFFF),
      _autoIndex(false),
      _isClientBodySizeSet(false),
      _max_client_body_size(0),
      _isCgi(false) {}

const std::string& RouteRule::getRoute() const { return _route; }

void RouteRule::setRoute(const std::string& route) { _route = route; }

const std::string& RouteRule::getLocation() const { return _location; }

void RouteRule::setLocation(const std::string& location) {
  _location = location;
}

int RouteRule::getAcceptedMethods() const { return _accepted_methods; }

void RouteRule::setAcceptedMethods(int methods) { _accepted_methods = methods; }

bool RouteRule::getIsClientBodySizeSet() const { return _isClientBodySizeSet; }

void RouteRule::setIsClientBodySizeSet(bool isSet) {
  _isClientBodySizeSet = isSet;
}

size_t RouteRule::getMaxClientBodySize() const { return _max_client_body_size; }

void RouteRule::setMaxClientBodySize(size_t maxSize) {
  _max_client_body_size = maxSize;
}

const std::map<int, std::string>& RouteRule::getRedirection() const {
  return _redirection;
}

void RouteRule::setRedirection(const std::map<int, std::string>& redirection) {
  _redirection = redirection;
}

bool RouteRule::hasRedirection(int code) const {
  return _redirection.find(code) != _redirection.end();
}

void RouteRule::addRedirection(int code, const std::string& url) {
  _redirection[code] = url;
}

bool RouteRule::getAutoIndex() const { return _autoIndex; }

void RouteRule::setAutoIndex(bool enable) { _autoIndex = enable; }

const std::string& RouteRule::getIndexPage() const { return _index_page; }

void RouteRule::setIndexPage(const std::string& index) { _index_page = index; }

bool RouteRule::hasErrorPage(int code) const {
  return _error_pages.find(code) != _error_pages.end();
}

void RouteRule::addErrorPage(int code, const std::string& url) {
  _error_pages[code] = url;
}

void RouteRule::setIsCgi(bool isCgi) { _isCgi = isCgi; }

bool RouteRule::getIsCgi() const { return _isCgi; }

void RouteRule::setCgiPath(const std::string& cgiPath) { _cgiPath = cgiPath; }

const std::string& RouteRule::getCgiPath() const { return _cgiPath; }