#include "Server.hpp"

#include <iterator>
#include <set>
#include <cstring>

#include "ConfigReader.hpp"
#include "Encoder.hpp"


// #include <iterator>
// void  __test_printReq(HttpRequest& r){
//   std::cout << r.getMethod() << " " << r.getHost() << ": "<< r.getLocation() << "?" << r.getQueries() << " HTTP" << r.getHttpMajor() << "/" << r.getHttpMinor() << std::endl;
//   std::map<std::string, std::string>::const_iterator i = r.getHeaders().begin();
//   for(; i != r.getHeaders().end(); i++){
//     std::cout << i->first << " : " << i->second << std::endl;
//   }
//   const std::vector<char> j  = r.getEntity();
//   for (size_t k = 0; k < j.size(); k++){
//     std::cout << j[k];
//   }
//   std::cout << std::endl;
// }



Server::Server(const char* configure_file) {
  std::cout << "Server constructing : " << configure_file << std::endl;
  ConfigReader reader(configure_file);
  reader.readFile();
  _hosts = reader.getHosts();
  _default_host = reader.getDefaultHost();
}

Server::~Server() {}

void Server::setSocketOption(int socket_fd) {
    int is_reuseaddr = 1;
    setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &is_reuseaddr, sizeof(is_reuseaddr));

    struct linger linger_opt;
    linger_opt.l_onoff = 1;
    linger_opt.l_linger = 0;
    setsockopt(socket_fd, SOL_SOCKET, SO_LINGER, &linger_opt, sizeof(linger_opt));
}

void Server::changeEvents(std::vector<struct kevent>& change_list,
                           uintptr_t ident, int16_t filter, uint16_t flags,
                           uint32_t fflags, intptr_t data, void* udata) {
  struct kevent tmp;

  EV_SET(&tmp, ident, filter, flags, fflags, data, udata);
  change_list.push_back(tmp);
}

void Server::handleErrorKevent(int fd) {
  if (_server_sockets.find(fd) != _server_sockets.end())
    throw std::runtime_error("Error: server socket error.");
  std::cerr << "client socket error" << std::endl;
  disconnectClient(fd);
}

void Server::disconnectClient(const int client_fd) {
  std::cout << "Client disconnected: " << client_fd << std::endl;
  close(client_fd);
  _clients.erase(client_fd);
}

void Server::connectClient(int server_socket) {
  int client_socket;

  if ((client_socket = accept(server_socket, NULL, NULL)) == -1)
    throw std::runtime_error("Error: accept fail");
  fcntl(client_socket, F_SETFL, O_NONBLOCK, FD_CLOEXEC);

  setSocketOption(client_socket);
  std::cout << "accept new client: " << client_socket << std::endl;

  changeEvents(_change_list, client_socket, EVFILT_READ, EV_ADD | EV_ENABLE, 0,
                0, NULL);
  changeEvents(_change_list, client_socket, EVFILT_WRITE, EV_ADD | EV_DISABLE,
                0, 0, NULL);
  _clients[client_socket] = Client(_server_sockets[server_socket], getTime(), KEEPALIVETIMEOUT);
}

void Server::sendHttpResponse(int client_fd) {
  Client& client = _clients[client_fd];
  const std::queue<HttpResponse>& responses = client.getRess();

  while (responses.size() > 0) {
    if (!responses.front().getIsReady()) break ;
    std::string encoded_response = Encoder::execute(responses.front());
    const char* buf = encoded_response.c_str();
    write(client_fd, buf, std::strlen(buf));
    buf = &(responses.front().getBody())[0];
    write(client_fd, buf, responses.front().getContentLength());
    client.popRess();
    std::cout << "response sent: client fd : [" << client_fd << "]\nHeader \n" << encoded_response.c_str() << std::endl;
    if (buf) std::cout << buf << std::endl;
  }
  changeEvents(_change_list, client_fd, EVFILT_WRITE, EV_DISABLE, 0, 0,
                  NULL);
  if (client.getEof()) disconnectClient(client_fd);
}

void Server::recvHttpRequest(int client_fd) {
  char    buf[BUF_SIZE] = {0,};
  Client& cli = _clients[client_fd];

  int n;
  while ((n = read(client_fd, buf, BUF_SIZE)) != 0) {
    if (n == -1) {
      if (errno == EWOULDBLOCK || errno == EAGAIN) break ; // NON-BLOCK socket read buff 비어있을 때
      throw std::runtime_error("Error: read error.");
    }
    if (n > 0) cli.addBuf(buf, n);
  }
  if (n == 0) cli.setEof(true);

  int idx;
  if (cli.getReqs().size() > 0) {
    HttpRequest&  last_request = cli.backRequest();

    if (!last_request.getEntityArrived()) {
      idx = last_request.settingContent(cli.subBuf(cli.getReadIdx(), cli.getBuf().size()));
      cli.addReadIdx(idx);
      if (!last_request.getEntityArrived()) return ;
      HttpResponse res;
      try{
        RouteRule rule = findRouteRule(last_request, client_fd);
        if (rule.getIsCgi()) {
          CgiHandler cgi_handler(last_request, rule);
          _cgi_handler[cgi_handler.getReadPipe()] = cgi_handler;
          changeEvents(_change_list, cgi_handler.getReadPipe(), EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL);
          cgi_handler.execute();
        } else if (last_request.getMethod() == HPS::kGET || last_request.getMethod() == HPS::kHEAD){
          res.publish(last_request, rule);
        } else {
          res.publishError(405);
        }
      } catch (Host::NoRouteRuleException &e) { 
        res.publishError(404);
        std::cout << e.what() << std::endl;
      }
      cli.addRess(res);
      cli.popReqs();
      cli.addReadIdx(idx);
    }
  }

  while ((idx = cli.headerEndIdx(cli.getReadIdx())) >= 0) { // header 읽기 (\r\n\r\n)
    HttpRequest             req;
    HttpDecoder             hd;
    size_t                  size = static_cast<size_t>(idx);
    const std::vector<char> data = cli.subBuf(cli.getReadIdx(), cli.getReadIdx() + size);
    cli.addReadIdx(size);
    hd.setCallback(
        NULL, HttpRequest::sParseUrl,
        NULL, HttpRequest::sSaveHeaderField,
        HttpRequest::sParseHeaderValue, HttpRequest::sSaveRquestData,
        NULL, NULL,
        NULL, NULL);
    hd.setDataSpace(static_cast<void*>(&req));

    if (hd.execute(&(data)[0], size) == size) {
      idx = req.settingContent(cli.subBuf(cli.getReadIdx(), cli.getBuf().size()));

      cli.addReqs(req);
      cli.addReadIdx(idx);
      if (req.getEntityArrived()) {
        HttpResponse res;
        try{
          RouteRule rule = findRouteRule(req, client_fd);
          if (rule.getIsCgi()) {
            CgiHandler cgi_handler(req, rule);
            _cgi_handler[cgi_handler.getReadPipe()] = cgi_handler;
            changeEvents(_change_list, cgi_handler.getReadPipe(), EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL);
            cgi_handler.execute();
          } else if (req.getMethod() == HPS::kGET || req.getMethod() == HPS::kHEAD){
            res.publish(req, rule);
          } else {
            res.publishError(405);
          }
        } catch (Host::NoRouteRuleException &e) {
          res.publishError(404);
          std::cout << e.what() << std::endl;
        } 
        cli.popReqs();
        cli.addRess(res);
      }
    } else {
      HttpResponse res;
      cli.setEof(true);
      res.publishError(400);
      cli.addRess(res);
    }
  }
  std::cout << "response size: " << cli.getRess().size() << std::endl;
  if (cli.getRess().front().getIsReady()) changeEvents(_change_list, client_fd, EVFILT_WRITE, EV_ENABLE, 0, 0, NULL);
}

void Server::recvCgiResponse(int cgi_fd) {
  cgi_fd++;
  std::cout << cgi_fd;

  //read pipe

  //if eof
  //close pipe

  //make response
  //set response done true
  
  //delete cgi_handler from _cgi_handler
  //enable write event
  //else return

}

void Server::init(void) {
  _kq = kqueue();
  if (_kq == -1) throw std::runtime_error("Error: kqueue fail.");

  std::map<std::pair<std::string, int>, Host>::iterator it = _hosts.begin();
  std::set<int> ports;
  while (it != _hosts.end()) {
    Host& h = it->second;
    if (ports.find(h.getPort()) != ports.end()) {
      it++;
      continue;
    }
    int socket_fd = socket(PF_INET, SOCK_STREAM, 0);
    if (socket_fd == -1) throw std::runtime_error("Error: socket failed.");
    fcntl(socket_fd, F_SETFL, O_NONBLOCK, FD_CLOEXEC);
    _server_sockets[socket_fd] = h.getPort();
    ports.insert(h.getPort());

    struct sockaddr_in sock_addr;
    sock_addr.sin_family = AF_INET;
    sock_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    sock_addr.sin_port = htons(h.getPort());
    setSocketOption(socket_fd);
    if (bind(socket_fd, (struct sockaddr*)&sock_addr, sizeof(sock_addr)) == -1)
      throw std::runtime_error("Error: bind failed.");
    if (listen(socket_fd, BACKLOG) == -1)
      throw std::runtime_error("Error: listen fail.");
    changeEvents(_change_list, socket_fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0,
                  0, NULL);
    it++;
  }
  changeEvents(_change_list, 0, EVFILT_TIMER, EV_ADD | EV_ENABLE, 0, 1000, NULL);
}

void Server::run(void) {
  std::cout << "Server started." << std::endl;
  int new_event;
  struct kevent event_list[EVENT_LIST_SIZE];
  struct kevent* curr_event;
  while (1) {
    new_event = kevent(_kq, &_change_list[0], _change_list.size(), event_list,
                       EVENT_LIST_SIZE, NULL);
    if (new_event == -1) throw std::runtime_error("Error: kevent fail.");
    _change_list.clear();
    for (int i = 0; i < new_event; ++i) {
      curr_event = &event_list[i];
      if (curr_event->flags & EV_ERROR) {  // error event
        handleErrorKevent(curr_event->ident);
      } else if (curr_event->flags & EV_EOF) {  
        disconnectClient(curr_event->ident);
      } else if (curr_event->filter == EVFILT_TIMER) {  // timer event
        checkTimeout();
      } else if (curr_event->filter == EVFILT_READ) {
        if (_server_sockets.count(curr_event->ident)) {  // socket read event
          connectClient(curr_event->ident);
        } else if (_clients.count(curr_event->ident)) {  // client read event
          _clients[curr_event->ident].setLastRequestTime(getTime());
          recvHttpRequest(curr_event->ident);
        } else if (_cgi_handler.count(curr_event->ident)) {  // cgi read event
          recvCgiResponse(curr_event->ident);
        }
      } else if (curr_event->filter == EVFILT_WRITE) {  // client write event
        sendHttpResponse(curr_event->ident);
      }
    }
  }
}

RouteRule Server::findRouteRule(const HttpRequest& req, const int& client_fd) {
  std::pair<std::string, int> key(req.getHost(), _clients[client_fd].getPort());
  if (_hosts.count(key))
    return (_hosts[key].getRouteRule(req.getLocation()));
  else
    return ( _default_host.getRouteRule(req.getLocation()));
}

time_t Server::getTime(void) {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec;
}

void      Server::checkTimeout(void){
  std::vector<int> disconnect_list;

  std::map<int, Client>::iterator it = _clients.begin();
  for (; it != _clients.end(); it++) {
    if (getTime() - it->second.getLastRequestTime() > it->second.getTimeoutInterval()) {
      disconnect_list.push_back(it->first);
    }
  }

  for (size_t i = 0; i < disconnect_list.size(); i++)
    disconnectClient(disconnect_list[i]);
}