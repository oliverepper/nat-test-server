//
//  Server.cpp
//  nat-test-server
//
//  Created by Oliver Epper on 21.04.22.
//

#include "Server.hpp"

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <iostream>
#include <arpa/inet.h>
#include <future>
#include <sys/event.h>

std::ostream& operator<<(std::ostream& os, const sockaddr_in& addr) {
    char buf[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &addr.sin_addr, buf, INET_ADDRSTRLEN);
    return os << buf;
}

int create_listen_socket(std::optional<std::string> host, unsigned int port) {
    // create socket
    auto listen_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listen_socket == -1) {
        std::cout << "Error creating socket";
        exit(EXIT_FAILURE);
    }

    // server info
    addrinfo hints {
        .ai_flags = AI_PASSIVE,
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM
    };
    addrinfo *server_info;
    auto error = getaddrinfo(host ? host.value().c_str() : nullptr, std::to_string(port).c_str(), &hints, &server_info);
    if (error < 0) {
        std::cout << "Error getting address info: " << gai_strerror(error) << std::endl;
        exit(EXIT_FAILURE);
    }

    // set options
    int enable = 1;
    error = setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
    if (error < 0) {
        std::cout << "Failed to set socket options: " << strerror(error) << std::endl;
        exit(EXIT_FAILURE);
    }

    // bind socket
    error = bind(listen_socket, server_info->ai_addr, server_info->ai_addrlen);
    if (error < 0) {
        std::cout << "Error bindung socket to address: " << strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }

    // start listening
    error = listen(listen_socket, 1);
    if (error < 0) {
        std::cout << "Error setting up socket to listen: " << strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }

    std::cout << "Listening on: "
        << *(sockaddr_in *)server_info->ai_addr << ":"
        << ntohs((*(sockaddr_in *)server_info->ai_addr).sin_port)
        << std::endl;

    return listen_socket;
}

Server::Server() : Server(std::nullopt, 1989) {}

Server::Server(unsigned int port) : Server(std::nullopt, port) {}

Server::Server(std::optional<std::string> host, unsigned int port) : m_listen_socket(create_listen_socket(host, port)) {}


#define MAX_EVENTS 32
void Server::run_event_loop() {
    struct kevent event_set;
    struct kevent event_list[MAX_EVENTS];
    while (true) {
        int num_events = kevent(m_kq, NULL, 0, event_list, MAX_EVENTS, NULL);
        for (int i = 0; i < num_events; i++) {
            if (event_list[i].ident == m_listen_socket) {
                sockaddr_in client_addr{};
                socklen_t client_addr_length = sizeof(client_addr);
                auto connection_socket = accept(m_listen_socket, (sockaddr *)&client_addr, &client_addr_length);
                if (connection_socket < 0) {
                    std::cout << "Error accepting connection: " << strerror(errno) << std::endl;
                }

                m_clients.emplace_back(connection_socket);

                EV_SET(&event_set, connection_socket, EVFILT_READ, EV_ADD, 0, 0, NULL);
                kevent(m_kq, &event_set, 1, NULL, 0, NULL);

                std::cout << "Client connected: "
                    << client_addr << ":"
                    << ntohs(client_addr.sin_port)
                    << std::endl;

            } else if (event_list[i].flags & EV_EOF) {
                uintptr_t fd = event_list[i].ident;
                auto iter = find_if(m_clients.begin(), m_clients.end(), [fd](const Client& c) -> bool {
                    return c.has_fd(fd);
                });
                m_clients.erase(iter, m_clients.end());
                EV_SET(&event_set, fd, EVFILT_READ, EV_DELETE, 0, 0, NULL);
                kevent(m_kq, &event_set, 1, NULL, 0, NULL);
                std::cout << "Client removed" << std::endl;

            } else if (event_list[i].filter == EVFILT_READ) {
                uintptr_t fd = event_list[i].ident;
                auto client = find_if(m_clients.begin(), m_clients.end(), [fd](const Client& c) -> bool {
                    return c.has_fd(fd);
                });
                client->read();
            }
        }
    }
}

void Server::heartbeat() {
    for (auto &c: m_clients) {
        c.heartbeat();
    }
}

void Server::run() {
    auto heartbeat = std::async([this](){
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            this->heartbeat();
        }
    });

    m_kq = kqueue();
    struct kevent event_set;
    EV_SET(&event_set, m_listen_socket, EVFILT_READ, EV_ADD, 0, 0, NULL);
    kevent(m_kq, &event_set, 1, NULL, 0, NULL);

    run_event_loop();

    exit(EXIT_SUCCESS);
}

void Server::shutdown() {
    std::cout << "Clients: " << m_clients.size() << std::endl;
    exit(EXIT_SUCCESS);
}
