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
#include <exception>
#include <stdexcept>

class Server {
  private:
  int       _port;
  int       _socket;
  int       _kq;

  struct sockaddr_in    _server_addr;
  std::map<int, std::string>  _clients;
  std::vector<struct kevent>  _change_list;

  Server(const Server& origin);
  Server& operator=(const Server& origin);

  const int&  getPort(void) const;

  void  change_events(std::vector<struct kevent>& change_list, uintptr_t ident, \
      int16_t filter, uint16_t flags, uint32_t fflags, intptr_t data, void *udata);
  void  disconnect_client(int client_fd);
  public:
  Server(int port = 80);
  ~Server();

  void  init(void);
  void  run(void);
};

#endif