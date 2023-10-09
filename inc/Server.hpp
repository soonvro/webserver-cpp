#ifndef SERVER_HPP_
#define SERVER_HPP_

#include <sys/types.h>
#include <sys/event.h>
#include <sys/socket.h>

#include <arpa/inet.h>

#include <unistd.h>

#include <fcntl.h>

#include <iostream>
#include <map>
#include <vector>
#include <set>
#include <utility>

#include <exception>
#include <stdexcept>

#include "Client.hpp"
#include "Host.hpp"

/*
  _hosts
    socket fd, host name으로 구별
  _clients
    socket fd로 구별
*/
class Server {
  private:
  Host _default_host; 
  std::map<std::pair<std::string, int>, Host>  _hosts;
  std::set<int> _server_sockets;
  
  int       _kq;
  std::vector<struct kevent>  _change_list;

  std::map<int, Client>  _clients;

  void  change_events(std::vector<struct kevent>& change_list, uintptr_t ident, \
      int16_t filter, uint16_t flags, uint32_t fflags, intptr_t data, void *udata);
  
  void  handle_error_kevent(int ident);
  void  disconnect_client(const int client_fd);
  
  void Server::connectClient(int server_socket);

  void	Server::sendHttpResponse(int client_fd);
  
  void	Server::recvHttpRequest(int client_fd);
  
  void	Server::recvCgiRequest(int cgi_fd);


  public:
  Server(const char *configure_file);
  ~Server();

  void  init(void);
  void  run(void);
};

#endif