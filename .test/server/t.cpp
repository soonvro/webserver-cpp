/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   t.cpp                                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: inskim <inskim@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/09/12 12:14:45 by sokwon            #+#    #+#             */
/*   Updated: 2023/10/11 15:38:42 by inskim           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h> //sockaddr_in, htons
#include <arpa/inet.h> //inet_addr
#include <unistd.h> //close
#include <string>
#include <fcntl.h>

void handle_error(const char* msg) {
    std::cerr << msg << std::endl;
    exit(EXIT_FAILURE);
}

int  server(int server_socket){
      /* init server socket and listen */
   server_socket = socket(PF_INET, SOCK_STREAM, 0);
    if (server_socket == -1)
        handle_error("error: socket() error");

    struct sockaddr_in server_addr;
    std::memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(std::atoi("80"));

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1)
        handle_error("error: bind() error");
    if (listen(server_socket, 1024) == -1)
        handle_error("error: listen() error");
    fcntl(server_socket, F_SETFL, O_NONBLOCK, FD_CLOEXEC);
    return server_socket;
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cout << "Usage: " << argv[0] << " <IP> <port>" << std::endl;
        exit(1);
    }
    int s;
    s = server(s);


    int sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == -1) handle_error("error: socket() error");

    struct sockaddr_in server_addr;
    std::memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(argv[1]);
    server_addr.sin_port = htons(std::atoi(argv[2]));

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1)
        handle_error("error: connect() error");

    std::cout << "Connected........." << std::endl;
    int client_socket = accept(s, NULL, NULL);
    if (client_socket == -1)
        handle_error("error: accept() error");
    std::cout << "accept new client: " << client_socket << std::endl;
    fcntl(client_socket, F_SETFL, O_NONBLOCK);
    
    char k[100] = {0,};
    write(sock, k, 1);
    int a = read(client_socket, k, 100);
    std::cout << a << std::endl;
    std::cout << errno << std::endl;
    close(sock);
    a = read(client_socket, k, 100);
    std::cout << a << std::endl;
    std::cout << errno << std::endl;
    close(s);
    close(sock);
    return 0;
}