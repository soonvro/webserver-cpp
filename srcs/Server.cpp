#include <iterator>
#include <set>
#include <cstring>
#include <sys/wait.h>

#include "Server.hpp"
#include "ConfigReader.hpp"
#include "HttpEncoder.hpp"

#define DEBUGMOD 0
#define DEBUG_DETAIL_RAWDATA (DEBUGMOD & 0)
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
  while (responses.size() > 0) {
    if (!responses.front().getIsReady()) break ;
    int idx = responses.front().getEntityIdx();
    if (idx == 0) {
      std::string encoded_response = HttpEncoder::execute(responses.front());
      const char* buf = encoded_response.c_str();
      event_size -= write(client_fd, buf, std::strlen(buf));
    }
    int n = write(client_fd, &(responses.front().getBody())[idx],\
         (int64_t)responses.front().getBody().size() - idx > event_size ? event_size : responses.front().getBody().size() - idx);
    if (n < 0)  {
      disconnectClient(client_fd);
      return ;
    }
    idx += n;
    responses.front().setEntityIdx(idx);
    if ((size_t)idx != responses.front().getBody().size()) return ;
    printRes(HttpEncoder::execute(responses.front()), &(responses.front().getBody())[0], responses.front().getContentLength());
    client.popRess();
    client.popReqs();
    std::cout << "response sent: client fd : " << client_fd << " bytes: " << n << '\n' << std::endl;
  }
  if (client.getEof()) disconnectClient(client_fd);
  else  changeEvents(_change_list, client_fd, EVFILT_WRITE, EV_DISABLE, 0, 0,
                  NULL);
}

void  Server::setCgiSetting(HttpResponse& res){
  _cgi_responses_on_pipe[res.getCgiPipeIn()] = &res;
  changeEvents(_change_list, res.getCgiPipeIn(), EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL);
  changeEvents(_change_list, res.getCgiHandler().getWritePipetoCgi(),
              EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, &(res.getCgiHandler()));
  int cgi_pid = res.cgiExecute();
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
        cli.addRess(last_request, *rule).backRess().publishError(411, rule, last_request.getMethod());
        changeEvents(_change_list, client_fd, EVFILT_WRITE, EV_ENABLE, 0, 0, NULL);
        cli.setEof(true);
        printReq(last_request, cli.getBuf(), true);
        return ;
      }
      cli.addReadIdx(idx);
      if (!last_request.getEntityArrived()) return ;
      const RouteRule *rule = findRouteRule(last_request, client_fd);
      cli.addRess(last_request, *rule).backRess().publish(last_request, rule, _clients[client_fd]);
      if (cli.backRess().getIsCgi()){
        setCgiSetting(cli.backRess());
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
      printReq(req, data, false);
      try{
        idx = req.settingContent(cli.getReadIter(), cli.getEndIter());
      } catch (HttpRequest::ChunkedException& e) {
        const RouteRule *rule = findRouteRule(req, client_fd);
        cli.addRess(req, *rule).backRess().publishError(411, rule, req.getMethod());
        changeEvents(_change_list, client_fd, EVFILT_WRITE, EV_ENABLE, 0, 0, NULL);
        cli.setEof(true);
        return ;
      }
      cli.addReadIdx(idx);
      if (req.getEntityArrived()) {
        const RouteRule *rule = findRouteRule(req, client_fd);
        cli.addRess(req, *rule).backRess().publish(req, rule, _clients[client_fd]);
        if (cli.backRess().getIsCgi()){
          setCgiSetting(cli.backRess());
        }
        cli.eraseBuf();
      }
    } else {
      const RouteRule *rule = findRouteRule(req, client_fd);
      cli.addRess(req, *rule).backRess().publishError(400, rule, req.getMethod());
      cli.setEof(true);
    }
  }

  std::cout << "response size: " << cli.getRess().size() << '\n' << std::endl;
  if (cli.getRess().size() && cli.getRess().front().getIsReady()) 
    changeEvents(_change_list, client_fd, EVFILT_WRITE, EV_ENABLE, 0, 0, NULL);
}

void  Server::sendCgiRequest(int cgi_fd, void* handler, int64_t event_size){
  CgiHandler* p_handler = static_cast<CgiHandler*>(handler);

  int n = 0;
  int idx = p_handler->getCgiReqEntityIdx();

  if (DEBUGMOD && DEBUG_DETAIL_KEVENT)  std::cout << "Cgi request send idx : " << idx << std::endl;
  
  n = write(cgi_fd, &(p_handler->getRequest().getEntity())[idx], \
    (int64_t)p_handler->getRequest().getEntity().size() - idx > event_size ? event_size : p_handler->getRequest().getEntity().size() - idx);
  if (n < 0) {
    disconnectClient(p_handler->getClientFd());
    return ;
  }
  idx += n;
  
  if (DEBUGMOD && DEBUG_DETAIL_KEVENT)  std::cout << "send end"  << std::endl;
  

  p_handler->setCgiReqEntityIdx(idx);
  if ((size_t)idx >= p_handler->getRequest().getEntity().size()) close(cgi_fd);
}


void  Server::recvCgiResponse(int cgi_fd, int64_t event_size) {
  char    *buf = new char[event_size];
  HttpResponse& res = *_cgi_responses_on_pipe[cgi_fd];
  CgiHandler& cgi_handler = res.getCgiHandler();

  int n = read(cgi_fd, buf, event_size);
  if (n > 0) cgi_handler.addBuf(buf, n);
  delete[] buf;
  if (n < 0)  {
    disconnectClient(cgi_handler.getClientFd());
    return ;
  }
  if (n != 0)  return ;

  cgi_handler.closeReadPipe();
  if (n == 0 && cgi_handler.getBuf().size() == 0) return ;
  res.setIsReady(true);
  //enable write event
  //delete cgi_handler from _cgi_handler
  _cgi_responses_on_pipe.erase(cgi_fd);

  std::vector<char>::const_iterator it = cgi_handler.getBuf().begin();
  std::string key;
  std::string value;
  std::vector<char>::const_iterator start = it;
  while (it != cgi_handler.getBuf().end()) {
    if (*it == ':'){
      key = std::string(start, it);
      start = it + 1;
    }else if (*it == '\n'){
      if (*(start - 1) != ':' || *(start - 1) == '\n') { ++it; break ; }
      if (*(it - 1) == '\r') value = std::string(start, it - 1);
      else value = std::string(start, it);
      res.setHeader(key, value);
      start = it + 1;
    }
    ++it;
  }
  res.setBody(it, cgi_handler.getBuf().end());
  const std::map<std::string, std::string>& headers = res.getHeader();
  int status;
  CgiType cgi_type;
  if (headers.find("Location") != headers.end()){
    const std::string& location = headers.find("Location")->second;
    if (location[0] == '/'){
      status = 300;
      cgi_type = kLocalRedir;
    }else{
      status = 302;
      cgi_type = kClientRedir;
    }
  } else if (headers.find("Content-type") != headers.end() || headers.find("Content-Type") != headers.end()) {
      status = 200;
      cgi_type = kDocument;
  } else {
      status = 404;
      cgi_type = kError;
  }   
  if (cgi_type == kClientRedir && res.getBody().size() > 0){
    cgi_type = kClientRedirDoc;
  }
  res.setStatus(status);
  if (cgi_type == kDocument){
    res.setStatusMessage("OK");
  } else if ( cgi_type == kClientRedirDoc || cgi_type == kClientRedir){
    res.setStatusMessage("Found");
  } else if (cgi_type == kLocalRedir){
    res.setIsReady(false);
    HttpRequest& req = const_cast<HttpRequest&> (cgi_handler.getRequest());
    Client& cli = _clients[cgi_handler.getClientFd()];
    req.setQueries("");
    req.setLocation(headers.find("Location")->second);
    res.initializeCgiProcess(req, cgi_handler.getRouteRule(), req.getHost(), cli.getPort(), cgi_handler.getClientFd());
    res.setIsCgi(true);
    setCgiSetting(res);
    return ;
  } else {
    const RouteRule& rule = cgi_handler.getRouteRule();
    if (rule.hasErrorPage(404)) {
        try{
          res.readFile(rule.getRoot() + "/" + rule.getErrorPage(404));
        } catch (HttpResponse::FileNotFoundException &e){
          std::cout << "configured error page not found" << std::endl;
          std::cout << e.what() << std::endl;
          res.publishError(404, 0, res.getMethod());
        }
    } else {
      res.publishError(404,  0, res.getMethod());
    }
  
  }
  res.setHeader("Connection", "keep-alive");
  res.addContentLength();
  changeEvents(_change_list, cgi_handler.getClientFd(), EVFILT_WRITE, EV_ENABLE, 0, 0, NULL);
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
          changeEvents(_change_list, res.getCgiHandler().getClientFd(), EVFILT_WRITE, EV_ENABLE, 0, 0, NULL);
        }
      // socket disconnect event
      } else if ((curr_event->flags & EV_EOF) && _clients.count(curr_event->ident)) {
        disconnectClient(curr_event->ident);
      } else if (curr_event->filter == EVFILT_TIMER) {  // timer event
        checkTimeout();
      } else if (curr_event->filter == EVFILT_READ) {
        if (_server_sockets.count(curr_event->ident)) {  // socket read event
          connectClient(curr_event->ident);
        } else if (_clients.count(curr_event->ident)) {  // client read event
          struct timespec a, b;
          _clients[curr_event->ident].setLastRequestTime(getTime());
          clock_gettime(CLOCK_REALTIME, &a);
          try{
            recvHttpRequest(curr_event->ident, curr_event->data);
          } catch (std::exception& e) {
            std::cout << e.what() << std::endl;
            _clients[curr_event->ident].setEof(true);;          
            HttpResponse& res = *_cgi_responses_on_pid[curr_event->ident];
            res.publishError(502, 0, res.getMethod());
            changeEvents(_change_list, res.getCgiHandler().getClientFd(), EVFILT_WRITE, EV_ENABLE, 0, 0, NULL);
          }
          clock_gettime(CLOCK_REALTIME, &b);
         std::cout << "recvHttpRequest time : " << (double)(b.tv_sec - a.tv_sec) * 1000000 + (double)(b.tv_nsec - a.tv_nsec) / 1000 << std::endl;
        } else if (_cgi_responses_on_pipe.count(curr_event->ident)) {  // cgi read event
          struct timespec a, b;
          clock_gettime(CLOCK_REALTIME, &a);
          recvCgiResponse(curr_event->ident, curr_event->data);
          clock_gettime(CLOCK_REALTIME, &b);
          std::cout << "recvCgiResponse time : " << (double)(b.tv_sec - a.tv_sec) * 1000000 + (double)(b.tv_nsec - a.tv_nsec) / 1000 << std::endl;
        }
      } else if (curr_event->filter == EVFILT_WRITE) {  //write event
        if (_clients.count(curr_event->ident)){
          struct timespec a, b;
          clock_gettime(CLOCK_REALTIME, &a);
          sendHttpResponse(curr_event->ident, curr_event->data);
          clock_gettime(CLOCK_REALTIME, &b);
          std::cout << "sendHttpResponse time : " << (double)(b.tv_sec - a.tv_sec) * 1000000 + (double)(b.tv_nsec - a.tv_nsec) / 1000 << std::endl;
        } else {
          struct timespec a, b;
          clock_gettime(CLOCK_REALTIME, &a);
          sendCgiRequest(curr_event->ident, curr_event->udata, curr_event->data);
          clock_gettime(CLOCK_REALTIME, &b);
          std::cout << "sendCgiRequest time : " << (double)(b.tv_sec - a.tv_sec) * 1000000 + (double)(b.tv_nsec - a.tv_nsec) / 1000 << std::endl;
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

  for (size_t i = 0; i < disconnect_list.size(); i++)
    disconnectClient(disconnect_list[i]);
}
