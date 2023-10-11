#include "Server.hpp"
#include <iterator>
#include "Encoder.hpp"

Server::Server(const char *configure_file) {
	std::cout << "Server constructing : " << configure_file << std::endl;
	//configure 파일 파싱 후 _hosts, _default_host 설정해야함.
	//test host
	Host h;

	h.setName("localhost");
	h.setPort(80);

	RouteRule	r;
	std::string path = "/";
	r.setRoute(path);
	r.setLocation(path);
	r.setAcceptedMethods(1);
	std::map<std::string, RouteRule> rm;
	rm[path] = r;

	h.setRouteRules(rm);

	_default_host = h;
	_hosts[make_pair(h.getName(), h.getPort())] = h;
}

Server::~Server() {} 

void	Server::change_events(std::vector<struct kevent>& change_list, uintptr_t ident, \
						int16_t filter, uint16_t flags, uint32_t fflags, intptr_t data, void *udata) {
	struct kevent	tmp;

	EV_SET(&tmp, ident, filter, flags, fflags, data, udata);
	change_list.push_back(tmp);
}

void  Server::handle_error_kevent(int fd){
	if (_server_sockets.find(fd) != _server_sockets.end()) 
		throw std::runtime_error("Error: server socket error.");
	std::cerr << "client socket error" << std::endl;
	disconnect_client(fd);
}

void	Server::disconnect_client(const int client_fd) {
	std::cout << "Client disconnected: " << client_fd << std::endl;
	close(client_fd);
	_clients.erase(client_fd);
}

void Server::connectClient(int server_socket){
	int	client_socket;

	if ((client_socket = accept(server_socket, NULL, NULL)) == -1) throw std::runtime_error("Error: accept fail");
	fcntl(client_socket, F_SETFL, O_NONBLOCK, FD_CLOEXEC);
	
	std::cout << "accept new client: " << client_socket << std::endl;
					
	change_events(_change_list, client_socket, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL);
	change_events(_change_list, client_socket, EVFILT_WRITE, EV_ADD | EV_DISABLE, 0, 0, NULL);
	_clients[client_socket] = Client();
}

void	Server::sendHttpResponse(int client_fd){
	Client& client = _clients[client_fd];
	const std::vector<HttpResponse>& responses = client.getRess();

	for (size_t i = 0; i < responses.size(); i++){
		std::string encoded_response = Encoder::execute(responses[i]);
		const char* buf = encoded_response.c_str();
		write(client_fd, buf, strlen(buf));
		buf = responses[i].getBody();
		write(client_fd, buf, responses[i].getBodySize());
		std::cout << "send" << std::endl;
		delete[] buf;
	}
	client.clearRess();
	if (client.getHasEof()){
		std::cout << "send eof" << std::endl;
		disconnect_client(client_fd);
	} else {
		std::cout << "send alive" << std::endl;
		change_events(_change_list, client_fd, EVFILT_WRITE, EV_DISABLE, 0, 0, NULL);
	}
}

void	Server::recvHttpRequest(int client_fd){
	char b[1000] = {0,};
	int a = read(client_fd, b, 1000);
	std::cout << "read: " << a << std::endl;


	Client& client = _clients[client_fd];
	HttpResponse res;
	res.setHttpMajor(1);
	res.setHttpMinor(1);
	res.setStatus(200);
	res.setStatusMessage("OK");
	std::map<std::string, std::string> headers;
	headers["Content-Type"] = "text/html";
	int fd = open("index.html", O_RDONLY);
	char *buf = new char[1000];
	int size = read(fd, buf, 1000);
	headers["Content-Length"] = std::to_string(size);
	res.setHeaders(headers);
	res.setBody(buf);
	res.setBodySize(size);
	client.addRess(res);
	change_events(_change_list, client_fd, EVFILT_WRITE, EV_ENABLE, 0, 0, NULL);
}

void	Server::recvCgiResponse(int cgi_fd){
	cgi_fd++;
}

void	Server::init(void) {
	_kq = kqueue();
	if (_kq == -1) throw std::runtime_error("Error: kqueue fail.");

	std::map<std::pair<std::string, int>, Host>::iterator it = _hosts.begin();
	while (it != _hosts.end()){
		Host &h = it->second;
		if (_server_sockets.find(h.getPort()) != _server_sockets.end()){
			it++;
			continue ;
		}
		int socket_fd = socket(PF_INET, SOCK_STREAM, 0);
		if (socket_fd == -1) throw std::runtime_error("Error: socket failed.");
		fcntl(socket_fd, F_SETFL, O_NONBLOCK, FD_CLOEXEC);
		_server_sockets.insert(socket_fd);
	
		struct sockaddr_in    sock_addr;
		sock_addr.sin_family = AF_INET;
		sock_addr.sin_addr.s_addr = htonl(INADDR_ANY);
		sock_addr.sin_port = htons(h.getPort());
		int option = 1;
		setsockopt( socket_fd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option) );
		if (bind(socket_fd, (struct sockaddr*)&sock_addr, sizeof(sock_addr)) == -1) throw std::runtime_error("Error: bind failed.");
		if (listen(socket_fd, 512) == -1) throw std::runtime_error("Error: listen fail.");
		change_events(_change_list, socket_fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL);
		it++;
	}
}

void	Server::run(void) {
	std::cout << "Server started." << std::endl;
	int	new_event;
	struct kevent	event_list[256];
	struct kevent*	curr_event;
	while (1) {
		new_event = kevent(_kq, &_change_list[0], _change_list.size(), event_list, 256, NULL);
		if (new_event == -1) throw std::runtime_error("Error: kevent fail.");
		_change_list.clear();
		for (int i = 0; i < new_event; ++i) {
			curr_event = &event_list[i];
			if (curr_event->flags & EV_ERROR) {//error event
				handle_error_kevent(curr_event->ident);
			} else if (curr_event->filter == EVFILT_READ) {
				if (_server_sockets.find(curr_event->ident) != _server_sockets.end()) {//socket read event
					connectClient(curr_event->ident);
				} else if (_clients.find(curr_event->ident) != _clients.end()) {//client read event
					recvHttpRequest(curr_event->ident);
				} else if (_cgi.find(curr_event->ident) != _cgi.end()) {//cgi read event
					recvCgiResponse(curr_event->ident);
				}
			} else if (curr_event->filter == EVFILT_WRITE) { //client write event
				sendHttpResponse(curr_event->ident);
			} 
		}
	}
}