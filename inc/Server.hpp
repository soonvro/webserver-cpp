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
#include <map>
#include <stdexcept>
#include <utility>
#include <vector>

#include "Client.hpp"
#include "Host.hpp"
#include "CgiHandler.hpp"
#include "SessionBlock.hpp"

#define BACKLOG 512
#define EVENT_LIST_SIZE 512
#define KEEPALIVETIMEOUT 60
#define SESSIONTIMELIMIT 1200

class Server {
 private:
  Host                                        _default_host;
  std::map<std::pair<std::string, int>, Host> _hosts;

  std::map<int, int>                          _server_sockets;  // <socket_fd, port>


  int                                         _kq;
  std::vector<struct kevent>                  _change_list;
  std::map<std::string, SessionBlock>         _session_blocks;

  std::map<int, Client>                       _clients;  // <socket_fd, Client>
  std::map<int, HttpResponse*>                _cgi_responses_on_pipe;  //<pipe_in_fd, pointer to response>
  std::map<int, HttpResponse*>                _cgi_responses_on_pid;  //<pid, pointer to response>

  void              setSocketOption(int socket_fd);

  void              changeEvents(std::vector<struct kevent> &change_list, uintptr_t ident,
                     int16_t filter, uint16_t flags, uint32_t fflags,
                     intptr_t data, void *udata);

  void              handleErrorKevent(int ident);
  void              disconnectClient(const int client_fd);
  void              connectClient(int server_socket);

  void              sendHttpResponse(int client_fd, int64_t event_size);
  void              recvHttpRequest(int client_fd, int64_t event_size);

  void              sendCgiRequest(int cgi_fd, void* req, int64_t event_size);
  void              recvCgiResponse(int cgi_fd, int64_t event_size);
  void              setCgiSetting(HttpResponse& res, const std::map<std::string, SessionBlock>::const_iterator& sbi, bool is_joined_session); 

  const RouteRule*  findRouteRule(const HttpRequest& req, const int& client_fd);

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
