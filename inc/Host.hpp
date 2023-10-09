#ifndef HOST_H_
#define HOST_H_

#include <string>
#include <map>
#include <utility>
#include <RouteRule>

class Host{
  private:
  std::string _name;
  
  int       _port;
  int       _socket;
  struct sockaddr_in    _sock_addr;


  std::map<int, std::string>  _default_error_pages;

  size_t _max_client_body_size;

  std::map<std::string, RouteRule>  _route_rules;

  std::pair<int, std::string>  _redirection;
};

#endif