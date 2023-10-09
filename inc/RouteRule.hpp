#ifndef HOST_H_
#define HOST_H_

#include <string>
#include <utility>

class RouteRule{
  private:
  std::string _route;
  std::string _location;

  int _accepted_methods;

  std::pair<int, std::string>  _redirection;

  bool  _directory_listing;

  std::string _default_response_for_directory;

  //CGI
}

#endif