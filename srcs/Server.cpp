#include "Server.hpp"
#include "Client.hpp"
#include "host.hpp"

#include <iterator>

Server::Server(const char *configure_file) {
	std::cout << "Server constructing : " << configure_file << std::endl;
}

Server::~Server() {} 

void	Server::change_events(std::vector<struct kevent>& change_list, uintptr_t ident, \
						int16_t filter, uint16_t flags, uint32_t fflags, intptr_t data, void *udata) {
	struct kevent	tmp;

	EV_SET(&tmp, ident, filter, flags, fflags, data, udata);
	change_list.push_back(tmp);
}

void  Server::handle_error_kevent(int ident){

	std::map<std::pair<std::string, int>, Host>::iterator it = _hosts.begin();

	while (it != _hosts.end()){
		Host &h = it->second;
		if (h.getSocket() == ident) throw std::runtime_error("Error: server socket error.");
		it++;
	}
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
					
	std::cout << "accept new client: " << client_socket << std::endl;
					
	change_events(_change_list, client_socket, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL);
	change_events(_change_list, client_socket, EVFILT_WRITE, EV_ADD | EV_DISABLE, 0, 0, NULL);
	_clients[client_socket] = Client();
}

void	Server::sendHttpResponse(int client_fd){
	//response만들때 꼭 client 있는지 확인 해야함.
	//event error 로 disconnect됬을수도있음. 
	client_fd++;
}

void	Server::recvHttpRequest(int client_fd){
	client_fd++;
}

void	Server::recvCgiRequest(int cgi_fd){
	cgi_fd++;
}

/*
	host 별로 소켓 생성 등록.
	kqueue 생성
*/
void	Server::init(void) {
	std::map<std::pair<std::string, int>, Host>::iterator it = _hosts.begin();

	while (it != _hosts.end()){
		Host &h = it->second;
		
		int socket = socket(PF_INET, SOCK_STREAM, 0);
		if (socket == -1) throw std::runtime_error("Error: socket failed.");
		fcntl(socket, F_SETFL, O_NONBLOCK, FD_CLOEXEC);
		_server_sockets.insert(socket);
	
		struct sockaddr_in    sock_addr;
		socket_addr.sin_family = AF_INET;
		socket_addr.sin_addr.s_addr = htonl(INADDR_ANY);
		socket_addr.sin_port = htons(_port);
		h.setSocket(socket);
		h.setSocketAddr(socket_addr);
		it++;
	}

	_kq = kqueue();
	if (_kq == -1) throw std::runtime_error("Error: kqueue fail.");
}

/*
	EV_ERROR 이벤트 발생시 처리 함수
*/



void	Server::run(void) {
	std::map<std::pair<std::string, int>, Host>::iterator it = _hosts.begin();
	//주소 바인드, listen, change_list 등록
	while (it != _hosts.end()){
		Host &h = it->second;
		if (bind(h.getSocket(), (struct sockaddr*)&(h.getSocketAddr()), sizeof(struct sockaddr)) == -1) throw std::runtime_error("Error: bind failed.");
		if (listen(h.getSocket(), 512) == -1) throw std::runtime_error("Error: listen fail.");
		change_events(_change_list, h.getSocket(), EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL);
		it++;
	}

	std::cout << "Server started." << std::endl;

	int	new_event;
	struct kevent	event_list[256];
	struct kevent*	curr_event;
	//루프 실행
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
				} else if (_cgi.find(curr_event->ident != _cgi.end())) {//cgi read event
					recvCgiResponse(curr_event->ident);
				}
			} else if (curr_event->filter == EVFILT_WRITE) { //client write event
				sendHttpResponse(curr_event->ident);
			} 
		}
	}
}