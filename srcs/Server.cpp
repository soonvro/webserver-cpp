#include <iterator>
#include <set>
#include <cstring>
#include <sys/wait.h>

#include "Server.hpp"
#include "ConfigReader.hpp"
#include "HttpEncoder.hpp"

#define DEBUGMOD 0
#define DEBUG_DETAIL_RAWDATA (DEBUGMOD & 1)
#define DEBUG_DETAIL_KEVENT  (DEBUGMOD & 1)

void printKeventLog(const int& new_event, const int& i, const struct kevent* curr_event) {
  if (!DEBUG_DETAIL_KEVENT) return;
  if (curr_event->filter == EVFILT_TIMER) return;

  std::string evfilt_str;
  if (curr_event->filter == EVFILT_READ) evfilt_str = "READ";
  else if (curr_event->filter == EVFILT_WRITE) evfilt_str = "WRITE";
  else if (curr_event->filter == EVFILT_PROC) evfilt_str = "PROCESS";
  std::cout << "EVFILT: " << evfilt_str << std::endl ;

  if (curr_event->flags & EV_ERROR) std::cout << "EV_ERROR!" << std::endl;
  if (curr_event->flags & EV_EOF) std::cout << "EV_EOF!" << std::endl;

  std::cout << "New kevent :" << new_event << " cur idx: " << i << std::endl;
  std::cout << "curr ident: " << curr_event->ident << ", data: " << curr_event->data << std::endl;
  std::cout << std::endl;
}

void  printReq(const HttpRequest& req, const std::vector<char>& data, bool force_print){
  if (!DEBUGMOD) return;
  if (req.getIsChunked() && !req.getEntityArrived() && !force_print) { // if not complete chunked
    std::cout << "Now receiving chunked data\n" << std::endl;
    return;
  }
  std::map<std::string, std::string>::const_iterator i = req.getHeaders().begin();
  std::cout << ">>>> REQUEST MESSAGE >>>>\n";
  const char* methods[] = {"GET", "HEAD", "POST", "DELETE"};
  std::cout << methods[req.getMethod() - 1] << ' ' <<
               req.getLocation() <<
               (req.getQueries().empty() ? "" : "?" + req.getQueries()) <<
               " HTTP/1.1" <<std::endl;
  std::cout << "Content-Length: " << req.getContentLength() << std::endl;
  if (req.getIsChunked()) std::cout << "Transfer-Encoding: chunked" << std::endl;
  while (i != req.getHeaders().end()){
    std::cout << i->first << ": " << i->second << std::endl;
    i++;
  }
  std::cout << "Host: " << req.getHost() << std::endl;
  std::cout << ">>>>>>>>>>>>>>>>>>>>>>>>>" << std::endl;

  if (!DEBUG_DETAIL_RAWDATA) return;
  std::cout << ">>>>----------------------------------" << std::endl;
  std::cout << std::string(data.begin(), data.end()) << std::endl;
  std::cout << ">>>>----------------------------------\n" << std::endl;
}

void  printRes(const std::string& header, const char* body, size_t body_size){
  if (!DEBUGMOD) return;
  std::cout << "<<<< RESPONSE MESSAGE <<<<\n" << header << "\n\n" << std::endl;
  std::cout << "<<<<<<<<<<<<<<<<<<<<<<<<<<" << std::endl;

  if (!DEBUG_DETAIL_RAWDATA || body_size == 0) return;
  std::cout << "<<<<----------------------------------" << std::endl;
  std::cout << std::string(body, body + body_size) << std::endl;
  std::cout << "<<<<----------------------------------\n" << std::endl;
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
  if (_clients.count(fd)){
    std::cout << "Client socket kevent error fd :" << fd << std::endl;
    disconnectClient(fd);
  } else if (_server_sockets.count(fd)){
    std::cout << "Server socket kevent error fd :" << fd << std::endl;
  } else if (_cgi_responses_on_pipe.count(fd)){
    std::cout << "Cgi read pipe kevent error fd :" << fd << std::endl;
  } else if (_cgi_responses_on_pid.count(fd)){
    std::cout << "Cgi process kevent error fd :" << fd << std::endl;
  } else {
    std::cout << "Kvent Error(Timer or Cgi write pipe) fd :" << fd << std::endl;
    return ;
  }
}

void Server::disconnectClient(const int client_fd) {
  std::cout << "Client disconnected: " << client_fd << "... " << std::flush;
  close(client_fd);
  std::queue<HttpResponse>& ress = (_clients[client_fd]).getRess();
  while (!ress.empty()) {
    close(ress.front().getCgiHandler().getWritePipetoCgi());
    close(ress.front().getCgiHandler().getReadPipeFromCgi());
    ress.pop();
  }
  _clients.erase(client_fd);
  std::cout << "Done." << std::endl;
}

void Server::connectClient(int server_socket) {
  int client_socket;
  sockaddr_in client_address;
  socklen_t client_len = sizeof(client_address);

  if ((client_socket = accept(server_socket, (struct sockaddr*)&client_address, &client_len)) == -1){
    std::cout << "accept error" << std::endl;
    return ;
  }
  fcntl(client_socket, F_SETFL, O_NONBLOCK, FD_CLOEXEC);
  char client_ip[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &client_address.sin_addr, client_ip, INET_ADDRSTRLEN);
  uint16_t client_port = ntohs(client_address.sin_port);
  setSocketOption(client_socket);

  std::cout << "Accepted connection from " << client_ip << ":" << client_port <<
               ", Client fd: " << client_socket << '\n' << std::endl;

  changeEvents(_change_list, client_socket, EVFILT_READ, EV_ADD | EV_ENABLE, 0,
                0, NULL);
  changeEvents(_change_list, client_socket, EVFILT_WRITE, EV_ADD | EV_DISABLE,
                0, 0, NULL);
  _clients[client_socket] = Client(client_socket, _server_sockets[server_socket], getTime(), KEEPALIVETIMEOUT);
}

void Server::sendHttpResponse(int client_fd, int64_t event_size) {
  Client& client = _clients[client_fd];
  std::queue<HttpResponse>& responses = client.getRess();
  const char* buf;
  int         idx;
  int         write_size;
  if (responses.front().getIsHeaderSent()){
    buf = &(responses.front().getBody())[0];
    idx = responses.front().getEntityIdx();
    write_size = (int)responses.front().getBody().size() - idx > event_size ? event_size : responses.front().getBody().size() - idx;
  } else {
    buf = HttpEncoder::execute(responses.front()).c_str();
    idx = responses.front().getHeaderIdx();
    write_size = (int)std::strlen(buf) - idx > event_size ? event_size : std::strlen(buf) - idx;
  }
  int n = write(client_fd, &buf[idx], write_size);
  if (n < 0){
    disconnectClient(client_fd);
    return ;
  }
  idx += n;
  if (responses.front().getIsHeaderSent()){
    responses.front().setEntityIdx(idx);
    if (idx != (int)responses.front().getBody().size()) return ;
  } else {
    responses.front().setHeaderIdx(idx);
    if (idx >= (int)std::strlen(buf)) responses.front().setIsHeaderSent(true);
    return ;
  }
  printRes(HttpEncoder::execute(responses.front()), &(responses.front().getBody())[0], responses.front().getContentLength());
  client.popRess();
  client.popReqs();
  std::cout << "response sent: client fd : " << client_fd << " bytes: " << n << std::endl;
  if (client.getEof()) disconnectClient(client_fd);
  if (responses.empty() || !responses.front().getIsReady()) changeEvents(_change_list, client_fd, EVFILT_WRITE, EV_DISABLE, 0, 0, NULL);
}

void  Server::setCgiSetting(HttpResponse& res, const std::map<std::string, SessionBlock>::const_iterator& sbi, bool is_joined_session) {
  _cgi_responses_on_pipe[res.getCgiPipeIn()] = &res;
  changeEvents(_change_list, res.getCgiPipeIn(), EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL);
  changeEvents(_change_list, res.getCgiHandler().getWritePipetoCgi(),
              EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, &(res.getCgiHandler()));
  int cgi_pid = res.cgiExecute(sbi, is_joined_session);
  _cgi_responses_on_pid[cgi_pid] = &res;
  changeEvents(_change_list, cgi_pid, EVFILT_PROC, EV_ADD | EV_ONESHOT, NOTE_EXIT, 0, NULL);
  std::cout << "create cgi process, pid: " << cgi_pid  << ", pipe_fd: " << res.getCgiPipeIn() << std::endl;
}

void Server::recvHttpRequest(int client_fd, int64_t event_size) {
  char    *buf = new char[event_size];
  Client& cli = _clients[client_fd];

  int n = read(client_fd, buf, event_size);

  if (n > 0)  cli.addBuf(buf, n);
  delete[] buf;
  if (n == 0) cli.setEof(true);
  if ((n == 0 && cli.getBuf().empty()) || n < 0){
    disconnectClient(client_fd);
    return ;
  }

  int idx;
  if (cli.getReqs().size() > 0) {
    HttpRequest& last_request = cli.backRequest();

    if (!last_request.getEntityArrived()) {
      printReq(last_request, cli.getBuf(), false);
      try{
        idx = last_request.settingContent(cli.getReadIter(), cli.getEndIter());
      } catch (HttpRequest::ChunkedException& e) {
        const RouteRule *rule = findRouteRule(last_request, client_fd);
        cli.addRess(last_request, rule).backRess().publishError(411, rule, last_request.getMethod());
        changeEvents(_change_list, client_fd, EVFILT_WRITE, EV_ENABLE, 0, 0, NULL);
        cli.setEof(true);
        printReq(last_request, cli.getBuf(), true);
        return ;
      }
      cli.addReadIdx(idx);
      if (!last_request.getEntityArrived()) return ;
      const RouteRule *rule = findRouteRule(last_request, client_fd);
      cli.addRess(last_request, rule).backRess().publish(last_request, rule, _clients[client_fd]);
      if (cli.backRess().getIsCgi()) {
        setCgiSetting(cli.backRess(), getSessionBlock(last_request.getSessionId()), isJoinedSession(last_request.getSessionId()));
      }
      printReq(last_request, cli.getBuf(), false);
      cli.eraseBuf();
    }
  }

  while ((idx = cli.headerEndIdx(cli.getReadIdx())) >= 0) { // header 읽기 (\r\n\r\n)
    HttpDecoder             hd;
    HttpRequest&            req = cli.addReqs().backRequest();
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
      req.setSessionId();
      printReq(req, data, false);
      try{
        idx = req.settingContent(cli.getReadIter(), cli.getEndIter());
      } catch (HttpRequest::ChunkedException& e) {
        const RouteRule *rule = findRouteRule(req, client_fd);
        cli.addRess(req, rule).backRess().publishError(411, rule, req.getMethod());
        changeEvents(_change_list, client_fd, EVFILT_WRITE, EV_ENABLE, 0, 0, NULL);
        cli.setEof(true);
        return ;
      }
      cli.addReadIdx(idx);
      if (req.getEntityArrived()) {
        const RouteRule *rule = findRouteRule(req, client_fd);
        cli.addRess(req, rule).backRess().publish(req, rule, _clients[client_fd]);
        if (cli.backRess().getIsCgi()) {
          setCgiSetting(cli.backRess(), getSessionBlock(req.getSessionId()), isJoinedSession(req.getSessionId()));
        }
        cli.eraseBuf();
      }
    } else {
      const RouteRule *rule = findRouteRule(req, client_fd);
      cli.addRess(req, rule).backRess().publishError(400, rule, req.getMethod());
      cli.setEof(true);
    }
  }

  if (cli.getRess().size() && cli.getRess().front().getIsReady()) 
    changeEvents(_change_list, client_fd, EVFILT_WRITE, EV_ENABLE, 0, 0, NULL);
}

void  Server::sendCgiRequest(int cgi_fd, void* handler, int64_t event_size){
  CgiHandler* p_handler = static_cast<CgiHandler*>(handler);
  if (!p_handler) {  // client socket closed
    close(cgi_fd);
    return;
  }

  int n = 0;
  int idx = p_handler->getCgiReqEntityIdx();

  if (DEBUGMOD && DEBUG_DETAIL_KEVENT)  std::cout << "Cgi request send idx : " << idx << std::endl;
  
  n = write(cgi_fd, &(p_handler->getRequest().getEntity())[idx], \
    (int64_t)p_handler->getRequest().getEntity().size() - idx > event_size ? event_size : p_handler->getRequest().getEntity().size() - idx);
  if (n < 0) {
    close(cgi_fd);
    return ;
  }
  idx += n;
  
  if (DEBUGMOD && DEBUG_DETAIL_KEVENT)  std::cout << "send end"  << std::endl;
  

  p_handler->setCgiReqEntityIdx(idx);
  if ((size_t)idx >= p_handler->getRequest().getEntity().size()) close(cgi_fd);
}


void  Server::recvCgiResponse(int cgi_fd, int64_t event_size) {
  char    *buf = new char[event_size];
  if (_cgi_responses_on_pipe.count(cgi_fd) == 0)  { delete[] buf; return ;}
  
  HttpResponse& res = *_cgi_responses_on_pipe[cgi_fd];
  CgiHandler& cgi_handler = res.getCgiHandler();

  int n = read(cgi_fd, buf, event_size);
  if (n > 0) cgi_handler.addBuf(buf, n);
  delete[] buf;
  if (n < 0)  {
    res.publishError(503, &cgi_handler.getRouteRule(), res.getMethod());
    changeEvents(_change_list, cgi_handler.getClientFd(), EVFILT_WRITE, EV_ENABLE, 0, 0, NULL);
    return ;
  }
  if (n == 0 && cgi_handler.getBuf().empty()) {
    close(cgi_fd);
    return;
  }
  if (n != 0)  return ;

  cgi_handler.closeReadPipe();
  _cgi_responses_on_pipe.erase(cgi_fd);

  try{
    res.publishCgi(cgi_handler.getBuf().begin(), cgi_handler.getBuf().end(), cgi_handler.getRouteRule(), res.getMethod());

    if (res.getIsSessionBlock()) {
      const SessionBlock& sb = res.getSessionBlock();
      _session_blocks[sb.getId()] = sb;
    } else if (res.getIsLogoutRequest() && isJoinedSession(res.getSessionBlock().getId())) {
      _session_blocks.erase(res.getSessionBlock().getId());
    }
  } catch (HttpResponse::LocalReDirException e){//local redir
    std::cout << "!!!!!!" << e.what() << std::endl;
    HttpRequest& req = const_cast<HttpRequest&> (cgi_handler.getRequest());
    Client& cli = _clients[cgi_handler.getClientFd()];
    req.setSessionId();
    req.setQueries("");
    req.setLocation(res.getHeader().find("Location")->second);
    res.initializeCgiProcess(req, cgi_handler.getRouteRule(), req.getHost(), cli.getPort(), cgi_handler.getClientFd());
    res.setIsCgi(true);
    setCgiSetting(cli.backRess(), getSessionBlock(req.getSessionId()), isJoinedSession(req.getSessionId()));
  }
  changeEvents(_change_list, cgi_handler.getClientFd(), EVFILT_WRITE, EV_ENABLE, 0, 0, NULL);
}

void Server::init(void) {
  signal(SIGPIPE, SIG_IGN); 
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
  std::cout << "Server started." << '\n' << std::endl;
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
      if (DEBUGMOD) printKeventLog(new_event, i, curr_event);
      // error event
      if (curr_event->flags & EV_ERROR) {
        handleErrorKevent(curr_event->ident);
      // cgi process exit event
      } else if (curr_event->filter == EVFILT_PROC && (curr_event->fflags & NOTE_EXIT)) {
        int status = -1;
        waitpid(curr_event->ident, &status, WNOHANG);
        std::cout << "waitpid :" << curr_event->ident << " status: " << WEXITSTATUS(status) << std::endl;
        if (WEXITSTATUS(status) != 0) {
          HttpResponse& res = *_cgi_responses_on_pid[curr_event->ident];
          res.publishError(503, 0, res.getMethod());
          _clients[curr_event->ident].setEof(true);
          changeEvents(_change_list, res.getCgiHandler().getClientFd(), EVFILT_WRITE, EV_ENABLE, 0, 0, NULL);
        }
      // socket disconnect event
      } else if ((curr_event->flags & EV_EOF) && _clients.count(curr_event->ident)) {
        if(DEBUG_DETAIL_KEVENT) std::cout << "+ Socket disconnect event" << std::endl;
        disconnectClient(curr_event->ident);
      } else if (curr_event->filter == EVFILT_TIMER) {  // timer event
        checkTimeout();
      } else if (curr_event->filter == EVFILT_READ) {
        if (_server_sockets.count(curr_event->ident)) {  // socket read event
          connectClient(curr_event->ident);
        } else if (_clients.count(curr_event->ident)) {  // client read event
          _clients[curr_event->ident].setLastRequestTime(getTime());
          try{
            recvHttpRequest(curr_event->ident, curr_event->data);
          } catch (std::exception& e) {
            std::cout << e.what() << std::endl;
            _clients[curr_event->ident].setEof(true);
            HttpResponse& res = *_cgi_responses_on_pid[curr_event->ident];
            res.publishError(502, 0, res.getMethod());
            changeEvents(_change_list, res.getCgiHandler().getClientFd(), EVFILT_WRITE, EV_ENABLE, 0, 0, NULL);
          }
        } else if (_cgi_responses_on_pipe.count(curr_event->ident)) {  // cgi read event
          recvCgiResponse(curr_event->ident, curr_event->data);
        }
      } else if (curr_event->filter == EVFILT_WRITE) {  //write event
        if (_clients.count(curr_event->ident)){
          sendHttpResponse(curr_event->ident, curr_event->data);
        } else {
          sendCgiRequest(curr_event->ident, curr_event->udata, curr_event->data);
        }
      } else {
        std::cout << "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXX Who you are??? XXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" << std::endl;
      }
    }
  }
}

const RouteRule* Server::findRouteRule(const HttpRequest& req, const int& client_fd) {
  std::pair<std::string, int> key(req.getHost(), _clients[client_fd].getPort());
  if (_hosts.count(key))  
    return (_hosts[key].getRouteRule(req.getLocation()));
  else
    return (_default_host.getRouteRule(req.getLocation()));
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

  for (size_t i = 0; i < disconnect_list.size(); i++){
   int client_fd = disconnect_list[i];
    _clients[client_fd].setEof(true);
    const HttpRequest& req = _clients[client_fd].addReqs().backRequest();
    _clients[client_fd].addRess(req, 0).backRess().publishError(408, NULL, HPS::kHEAD);
    changeEvents(_change_list, client_fd, EVFILT_WRITE, EV_ENABLE, 0, 0, NULL);
  }
}

bool                                                       Server::isJoinedSession(const std::string& session_id) { return _session_blocks.find(session_id) != _session_blocks.end(); }
const std::map<std::string, SessionBlock>::const_iterator  Server::getSessionBlock(const std::string& session_id) { return _session_blocks.find(session_id); }