#ifndef SERVER_HPP_
#define SERVER_HPP_

#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/event.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>

#include <exception>
#include <iostream>
#include <stdexcept>
#include <utility>
#include <map>
#include <vector>
#include <set>

#include "Client.hpp"
#include "Host.hpp"
#include "CgiHandler.hpp"
#include "SessionBlock.hpp"

#define BACKLOG 512
#define EVENT_LIST_SIZE 512
#define KEEPALIVETIMEOUT 600
#define SESSIONTIMELIMIT 1200

class Server {
 private:
  Host                                        _default_host;
  std::map<std::pair<std::string, int>, Host> _hosts;

  std::map<int, int>                          _server_sockets;  // <socket_fd, port>


  int                                         _kq;
  std::vector<struct kevent>                  _change_list;
  std::map<std::string, SessionBlock>         _session_blocks;

  std::set<Client>                            _clients;
  std::set<Client*>                           _clients_address;


  void              setSocketOption(int socket_fd);

  void              changeEvents(std::vector<struct kevent> &change_list, uintptr_t ident,
                     int16_t filter, uint16_t flags, uint32_t fflags,
                     intptr_t data, void *udata);

  void              handleErrorKevent(int ident, void *udata);
  void              disconnectClient(Client* client);
  void              connectClient(int server_socket);

  void              sendHttpResponse(int client_fd, Client& client, int64_t event_size);
  void              recvHttpRequest(int client_fd, Client& client, int64_t event_size);

  void              sendCgiRequest(int cgi_fd, Client& client, int64_t event_size);
  void              recvCgiResponse(int cgi_fd, Client& client, int64_t event_size);
  void              setCgiSetting(HttpResponse& res, Client& client, const std::map<std::string, SessionBlock>::const_iterator& sbi, bool is_joined_session); 

  const RouteRule*  findRouteRule(const HttpRequest& req, const int& host_port);

  time_t            getTime(void);  //return seconds
  void              checkTimeout(void);

 public:
  Server(const char *configure_file);
  ~Server();

  void init(void);
  void run(void);

  bool                                                      isJoinedSession(const std::string& session_id);
  const std::map<std::string, SessionBlock>::const_iterator getSessionBlock(const std::string& session_id);
};

#endif
