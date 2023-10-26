#ifndef SERVER_HPP_
#define SERVER_HPP_

#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/event.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <exception>
#include <iostream>
#include <map>
#include <stdexcept>
#include <utility>
#include <vector>

#include "Client.hpp"
#include "Host.hpp"

#define BUF_SIZE 4096
#define BACKLOG 512
#define EVENT_LIST_SIZE 512
#define KEEPALIVETIMEOUT 65

class Server {
 private:
  Host                                        _default_host;
  std::map<std::pair<std::string, int>, Host> _hosts;

  std::map<int, int> _server_sockets;  // <socket_fd, port>


  int                                         _kq;
  std::vector<struct kevent>                  _change_list;

  std::map<int, Client>                       _clients;
  std::map<int, std::string>                  _cgi;  // value 바꿔야함.

  void setSocketOption(int socket_fd);

  void change_events(std::vector<struct kevent> &change_list, uintptr_t ident,
                     int16_t filter, uint16_t flags, uint32_t fflags,
                     intptr_t data, void *udata);

  void handle_error_kevent(int ident);
  void disconnect_client(const int client_fd);

  void connectClient(int server_socket);

  void sendHttpResponse(int client_fd);

  void recvHttpRequest(int client_fd);

  void recvCgiResponse(int cgi_fd);

 public:
  Server(const char *configure_file);
  ~Server();

  void init(void);
  void run(void);
};

#endif
