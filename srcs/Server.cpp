#include "Server.hpp"

#include <iterator>
#include <set>
#include <cstring>

#include "ConfigReader.hpp"
#include "HttpEncoder.hpp"
#include "CgiResponse.hpp"


#define DEBUGMOD 1

void  printReq(const HttpRequest& req){
  if (!DEBUGMOD){
    return ;
  }
  std::map<std::string, std::string>::const_iterator i = req.getHeaders().begin();
  std::cout << "<< REQUEST MESSAGE >>\n";
  if (req.getMethod() == HPS::kGET){
    std::cout << "GET ";
  }
  if (req.getMethod() == HPS::kHEAD){
    std::cout << "HEAD ";
  }if (req.getMethod() == HPS::kPOST){
    std::cout << "POST ";
  }if (req.getMethod() == HPS::kDELETE){
    std::cout << "DELETE ";
  }
  std::cout  << req.getHost() << \
  req.getLocation() << "?" << req.getQueries()<< " HTTP/1.1" <<std::endl;
  while (i != req.getHeaders().end()){
    std::cout << i->first << ": " << i->second << std::endl;
    i++;
  }
  std::cout << "Host: " << std::endl;
  const std::vector<char>& entity = req.getEntity();
  std::vector<char>::const_iterator j = entity.begin();
  while (j != req.getEntity().end()){
    std::cout << *j;
  }
  std::cout<< std::endl;
}

void  printRes(std::string header, const char* data, size_t size){
  if (!DEBUGMOD){
    return ;
  }
  std::string res(data, data + size);
  std::cout << "<< RESPONSE MESSAGE >>\n" << header << "\n\n" << res << '\n' << std::endl;
}

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
  if (_server_sockets.find(fd) == _server_sockets.end()){
    std::cout << "kqueue Error fd :" << fd << std::endl;
    return ;
  }
  std::cout << "Client socket error" << std::endl;
  disconnectClient(fd);
}

void Server::disconnectClient(const int client_fd) {
  std::cout << "Client disconnected: " << client_fd << std::endl;
  close(client_fd);
  _clients.erase(client_fd);
}

void Server::connectClient(int server_socket) {
  int client_socket;
  sockaddr_in client_address;
  socklen_t client_len = sizeof(client_address);

  if ((client_socket = accept(server_socket, (struct sockaddr*)&client_address, &client_len)) == -1)
    throw std::runtime_error("Error: accept fail");
  fcntl(client_socket, F_SETFL, O_NONBLOCK, FD_CLOEXEC);
  char client_ip[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &client_address.sin_addr, client_ip, INET_ADDRSTRLEN);
  uint16_t client_port = ntohs(client_address.sin_port);
  std::cout << "Accepted connection from " << client_ip << ":" << client_port << std::endl;
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
    std::string encoded_response = HttpEncoder::execute(responses.front());
    const char* buf = encoded_response.c_str();
    write(client_fd, buf, std::strlen(buf));
    buf = &(responses.front().getBody())[0];
    printRes(encoded_response, &(responses.front().getBody())[0], responses.front().getContentLength());
    write(client_fd, buf, responses.front().getContentLength());
    client.popRess();
    std::cout << "response sent: client fd : " << client_fd << std::endl;
  }
  if (client.getEof()) disconnectClient(client_fd);
  else  changeEvents(_change_list, client_fd, EVFILT_WRITE, EV_DISABLE, 0, 0,
                  NULL);
}

void Server::executeCgi(HttpResponse& res, HttpRequest& req, RouteRule& rule, int client_fd) {
  res.initializeCgiProcess(req, rule, req.getHost(), _clients[client_fd].getPort(), client_fd);
  _cgi_responses_on_pipe[res.getCgiPipeIn()] = &res;
  changeEvents(_change_list, res.getCgiPipeIn(), EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL);
  int cgi_pid = res.cgiExecute();
  _cgi_responses_on_pid[cgi_pid] = &res;
  changeEvents(_change_list, cgi_pid, EVFILT_PROC, EV_ADD | EV_ONESHOT, NOTE_EXIT, 0, NULL);
  std::cout << "create cgi process, pid: " << cgi_pid  << ", pipe_fd: " << res.getCgiPipeIn() << std::endl;
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
  if (n == 0 && cli.getBuf().empty()){
    disconnectClient(client_fd);
    return ;
  }

  int idx;
  if (cli.getReqs().size() > 0) {
    HttpRequest& last_request = cli.backRequest();

    if (!last_request.getEntityArrived()) {
      printReq(last_request);

      try {
        idx = last_request.settingContent(cli.subBuf(cli.getReadIdx(), cli.getBuf().size()));
        cli.addReadIdx(idx);
        if (!last_request.getEntityArrived()) return ;
        HttpResponse& res = cli.addRess().backRess();
        try{
          RouteRule rule = findRouteRule(last_request, client_fd);
          if (rule.getIsCgi()) {
            executeCgi(res, last_request, rule, client_fd);
          } else if (last_request.getMethod() == HPS::kGET){
            res.publish(last_request, rule);
          } else {
            res.publishError(405);
          }
        } catch (Host::NoRouteRuleException& e) { 
          res.publishError(404);
          std::cout << e.what() << std::endl;
        } catch (std::runtime_error& e) { 
          res.publishError(500);
          std::cout << e.what() << std::endl;
        }
        cli.eraseBuf();
        cli.popReqs();
      } catch (HttpRequest::ChunkedException& e) {
        HttpResponse& res = cli.addRess().backRess();

        cli.setEof(true);
        res.publishError(411);
        std::cout << e.what() << std::endl;
      }
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
      try {
        idx = req.settingContent(cli.subBuf(cli.getReadIdx(), cli.getBuf().size()));

        cli.addReqs(req);
        cli.addReadIdx(idx);
        if (req.getEntityArrived()) {
          HttpResponse& res = cli.addRess().backRess();
          printReq(req);
          try {
            RouteRule rule = findRouteRule(req, client_fd);
            if (rule.getIsCgi()) {
              executeCgi(res, req, rule, client_fd);
            } else if (req.getMethod() == HPS::kGET){
              res.publish(req, rule);
            } else {
              res.publishError(405);
            }
          } catch (Host::NoRouteRuleException& e) {
            res.publishError(404);
            std::cout << e.what() << std::endl;
          } catch (std::runtime_error &e) { 
            res.publishError(500);
            std::cout << e.what() << std::endl;
          }
          cli.popReqs();
        }
      } catch (HttpRequest::ChunkedException& e) {
        HttpResponse& res = cli.addRess().backRess();

        cli.setEof(true);
        res.publishError(411);
        std::cout << e.what() << std::endl;
      }
    } else {
      HttpResponse& res = cli.addRess().backRess();
      cli.setEof(true);
      res.publishError(400);
    }
    cli.eraseBuf();
  }
  std::cout << "response size: " << cli.getRess().size() << std::endl;
  if (cli.getRess().size() && cli.getRess().front().getIsReady()) 
    changeEvents(_change_list, client_fd, EVFILT_WRITE, EV_ENABLE, 0, 0, NULL);
}

void Server::recvCgiResponse(int cgi_fd) {
  char    buf[BUF_SIZE] = {0,};
  HttpResponse& res = *_cgi_responses_on_pipe[cgi_fd];
  CgiHandler& cgi_handler = res.getCgiHandler();

  int n;
  while ((n = read(cgi_fd, buf, BUF_SIZE)) != 0) {
    if (n == -1) {
      if (errno == EWOULDBLOCK || errno == EAGAIN) break ; // NON-BLOCK socket read buff 비어있을 때
      throw std::runtime_error("Error: read error.");
    }
    if (n > 0) cgi_handler.addBuf(buf, n);
  }
  if (n != 0) return ;
  cgi_handler.closeReadPipe();
  if (n == 0 && cgi_handler.getBuf().size() == 0) return ;
  res.setIsReady(true);
  //enable write event
  //delete cgi_handler from _cgi_handler
  changeEvents(_change_list, cgi_handler.getClientFd(), EVFILT_WRITE, EV_ENABLE, 0, 0, NULL);
  _cgi_responses_on_pipe.erase(cgi_fd);

  //cgi response 생성
  buf[0] = 0;
  cgi_handler.addBuf(buf, 1);
  std::string cgi_response_str(&(cgi_handler.getBuf())[0]);
  CgiResponse cgi_response(cgi_response_str);
  const CgiType &cgi_type = cgi_response.getType();
  if (cgi_type == kDocument){
    res.setStatusMessage("OK");
  } else if ( cgi_type == kClientRedirDoc || cgi_type == kClientRedir){
    res.setStatusMessage("Found");
  } else if (cgi_type == kLocalRedir){
    // res.setIsReady(false);
    // changeEvents(_change_list, cgi_handler.getClientFd(), EVFILT_WRITE, EV_DISABLE, 0, 0, NULL);
    // res.initializeCgiProcess();
    // _cgi_responses_on_pipe[res.getCgiPipeIn()] = &res;
    // changeEvents(_change_list, res.getCgiPipeIn(), EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL);
    // res.cgiExecute();
    return ;
  } else {
    const RouteRule& rule = cgi_handler.getRouteRule();
    if (rule.hasErrorPage(404)) {
        try{
          res.readFile(rule.getRoot() + "/" + rule.getErrorPage(404));
        } catch (HttpResponse::FileNotFoundException &e){
          std::cout << "configured error page not found" << std::endl;
          std::cout << e.what() << std::endl;
          res.publishError(404);
        }
    } else {
      res.publishError(404);
    }
  
  }
  res.setStatus(cgi_response.getStatus());
  res.setBody(cgi_response.getBody());
  res.addContentLength();
  const std::map<std::string, std::string>& headers = cgi_response.getHeaders();
  if (headers.find("Content-Type") != headers.end())
    res.setHeader("Content-Type", headers.find("Content-Type")->second);
  if (headers.find("Location") != headers.end())
    res.setHeader("Location", headers.find("Location")->second);
  res.setHeader("Connection", "keep-alive");
  res.addContentLength();
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
    std::cout << "socket fd: " << socket_fd << std::endl;
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
    new_event = kevent(_kq, &(_change_list[0]), _change_list.size(), event_list,
                       EVENT_LIST_SIZE, NULL);
    if (new_event == -1) throw std::runtime_error("Error: kevent fail.");
    _change_list.clear();
    for (int i = 0; i < new_event; ++i) {
      curr_event = &event_list[i];
      if (curr_event -> ident != 0){
        std::cout << "New kevent :" << new_event << " cur idx: " << i << std::endl;
        std::cout << "curr ident: " << curr_event->ident << ", data: " << curr_event->data << std::endl;
        std::cout << "EVFILT: " << curr_event->filter << "\n" << std::endl ;
      }
      if (curr_event->flags & EV_ERROR) {  // error event
        handleErrorKevent(curr_event->ident);
      } else if (curr_event->fflags & NOTE_EXIT) { //  cgi process exit event
        int status = -1;
        waitpid(curr_event->ident, &status, WNOHANG);
        std::cout << "waitpid :" << curr_event->ident << " status: " << WEXITSTATUS(status) << std::endl;
        if (WEXITSTATUS(status) != 0) {
          std::cout << "EXIT_FAILURE:" << curr_event->ident << std::endl;
          HttpResponse& res = *_cgi_responses_on_pid[curr_event->ident];
          res.publishError(502);
          changeEvents(_change_list, res.getCgiHandler().getClientFd(), EVFILT_WRITE, EV_ENABLE, 0, 0, NULL);
        }
      } else if (curr_event->flags & EV_EOF && _server_sockets.count(curr_event->ident)) {  // socket disconnect event
        disconnectClient(curr_event->ident);
      } else if (curr_event->filter == EVFILT_TIMER) {  // timer event
        checkTimeout();
      } else if (curr_event->filter == EVFILT_READ) {
        if (_server_sockets.count(curr_event->ident)) {  // socket read event
          connectClient(curr_event->ident);
        } else if (_clients.count(curr_event->ident)) {  // client read event
          _clients[curr_event->ident].setLastRequestTime(getTime());
          recvHttpRequest(curr_event->ident);
        } else if (_cgi_responses_on_pipe.count(curr_event->ident)) {  // cgi read event
          recvCgiResponse(curr_event->ident);
        }
      } else if (curr_event->filter == EVFILT_WRITE) {  // client write event
        sendHttpResponse(curr_event->ident);
      } else std::cout << "Who you are???" << std::endl;
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
