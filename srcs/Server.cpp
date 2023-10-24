#include "Server.hpp"

#include <iterator>
#include <set>

#include "ConfigReader.hpp"
#include "Encoder.hpp"

Server::Server(const char* configure_file) {
  std::cout << "Server constructing : " << configure_file << std::endl;
  ConfigReader reader(configure_file);
  reader.readFile();
  _hosts = reader.getHosts();
  _default_host = reader.getDefaultHost();
}

Server::~Server() {}

void Server::change_events(std::vector<struct kevent>& change_list,
                           uintptr_t ident, int16_t filter, uint16_t flags,
                           uint32_t fflags, intptr_t data, void* udata) {
  struct kevent tmp;

  EV_SET(&tmp, ident, filter, flags, fflags, data, udata);
  change_list.push_back(tmp);
}

void Server::handle_error_kevent(int fd) {
  if (_server_sockets.find(fd) != _server_sockets.end())
    throw std::runtime_error("Error: server socket error.");
  std::cerr << "client socket error" << std::endl;
  disconnect_client(fd);
}

void Server::disconnect_client(const int client_fd) {
  std::cout << "Client disconnected: " << client_fd << std::endl;
  close(client_fd);
  _clients.erase(client_fd);
}

void Server::connectClient(int server_socket) {
  int client_socket;

  if ((client_socket = accept(server_socket, NULL, NULL)) == -1)
    throw std::runtime_error("Error: accept fail");
  fcntl(client_socket, F_SETFL, O_NONBLOCK, FD_CLOEXEC);

  std::cout << "accept new client: " << client_socket << std::endl;

  change_events(_change_list, client_socket, EVFILT_READ, EV_ADD | EV_ENABLE, 0,
                0, NULL);
  change_events(_change_list, client_socket, EVFILT_WRITE, EV_ADD | EV_DISABLE,
                0, 0, NULL);
  _clients[client_socket] = Client(_server_sockets[server_socket]);
}

void Server::sendHttpResponse(int client_fd) {
  Client& client = _clients[client_fd];
  const std::vector<HttpResponse>& responses = client.getRess();

  for (size_t i = 0; i < responses.size(); i++) {
    std::string encoded_response = Encoder::execute(responses[i]);
    const char* buf = encoded_response.c_str();
    write(client_fd, buf, strlen(buf));
    buf = responses[i].getBody();
    write(client_fd, buf, responses[i].getBodySize());
    std::cout << "send" << std::endl;
    delete[] buf;
  }
  client.clearRess();
  if (client.getHasEof()) {
    std::cout << "send eof" << std::endl;
    disconnect_client(client_fd);
  } else {
    std::cout << "send alive" << std::endl;
    change_events(_change_list, client_fd, EVFILT_WRITE, EV_DISABLE, 0, 0,
                  NULL);
  }
}

void Server::recvHttpRequest(int client_fd) {

  char buf[BUF_SIZE];
  int read_size = read(client_fd, buf, BUF_SIZE);
  write(1, buf, read_size);


  std::string path = "/a/index.html";
  std::string host = "host88";
  Client& client = _clients[client_fd];
  const int& port = client.getPort();
  std::cout << "client port : " << port << std::endl;


  Host a = _hosts[std::make_pair(host, port)];
  std::cout << "host name : " << a.getName() << std::endl;
  std::cout << "host port : " << a.getPort() << std::endl;
  std::map<std::string, RouteRule> route_rules = a.getRouteRules();
  RouteRule r;

  if (route_rules.find("/a") == route_rules.end())  {
    throw std::runtime_error("Error: route rule not found.");
  }else{
    r = route_rules["/a"];
    std::string loc = r.getLocation() + path;
    std::ifstream ifs(loc.c_str());
    if (ifs.fail()) {
      throw std::runtime_error("Error: file open fail.");
    }else{
      std::string c;
      ifs >> c;
      write(client_fd, c.c_str(), c.size());
    }
  }
  
}

void Server::recvCgiResponse(int cgi_fd) {
  cgi_fd++;
  std::cout << cgi_fd;
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
    int option = 1;
    setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
    if (bind(socket_fd, (struct sockaddr*)&sock_addr, sizeof(sock_addr)) == -1)
      throw std::runtime_error("Error: bind failed.");
    if (listen(socket_fd, BACKLOG) == -1)
      throw std::runtime_error("Error: listen fail.");
    change_events(_change_list, socket_fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0,
                  0, NULL);
    it++;
  }
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
        handle_error_kevent(curr_event->ident);
      } else if (curr_event->filter == EVFILT_READ) {
        if (_server_sockets.find(curr_event->ident) !=
            _server_sockets.end()) {  // socket read event
          connectClient(curr_event->ident);
        } else if (_clients.find(curr_event->ident) !=
                   _clients.end()) {  // client read event
          recvHttpRequest(curr_event->ident);
        } else if (_cgi.find(curr_event->ident) !=
                   _cgi.end()) {  // cgi read event
          recvCgiResponse(curr_event->ident);
        }
      } else if (curr_event->filter == EVFILT_WRITE) {  // client write event
        sendHttpResponse(curr_event->ident);
      }
    }
  }
}