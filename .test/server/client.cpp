/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: inskim <inskim@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/09/12 12:14:45 by sokwon            #+#    #+#             */
/*   Updated: 2023/10/06 16:22:39 by inskim           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h> //sockaddr_in, htons
#include <arpa/inet.h> //inet_addr
#include <unistd.h> //close
#include <string>

void handle_error(const char* msg) {
    std::cerr << msg << std::endl;
    exit(EXIT_FAILURE);
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cout << "Usage: " << argv[0] << " <IP> <port>" << std::endl;
        exit(1);
    }

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
    std::cout << "Input message(Q to quit): " << std::endl;

    char        msg[4096];
    std::string temp;
    ssize_t     send_len;
    ssize_t     recv_len;
    ssize_t     recv_cnt;
    while (true) {
        std::memset(msg, 0, sizeof(msg));
        std::cout << "> " << std::flush;
        std::getline(std::cin, temp, '\n');
        if (temp.length() == 0) continue;
        if (temp == "Q") break;

        std::strcpy(msg, temp.c_str());
        send_len = write(sock, msg, temp.length());
        std::cout << "sending..." << std::endl;

        recv_len = 0;
        while (recv_len < send_len) {
            std::cout << "in while" << std::endl;
            recv_cnt = read(sock, &msg[recv_len], sizeof(msg) - 1);
            if (recv_cnt == -1) handle_error("error: read() error");
            std::cout << "recv_cnt: " << recv_cnt << std::endl;
            recv_len += recv_cnt;
        }
        msg[recv_len] = '\0';
        std::cout << msg << std::endl;
    }

    close(sock);
    return 0;
}