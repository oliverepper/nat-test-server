//
//  Client.cpp
//  nat-test-server
//
//  Created by Oliver Epper on 21.04.22.
//

#include "Client.hpp"
#include <sys/socket.h>

std::ostream& operator<<(std::ostream& os, const Client& c) {
    os << "Client <" << c.m_fd << ">";
    if (c.m_interval.has_value()) {
        os << " interval: " << c.m_interval.value();
        os << " counter: " << c.m_threshold.value();
    }
    return os;
}

bool Client::has_fd(const uintptr_t fd) const {
    return m_fd == fd;
}

void Client::read() {
    char buffer[32];
    bzero(buffer, 32);
    auto read_bytes = recv(m_fd, &buffer, 32, 0);
    if (read_bytes > 1) {
        m_interval = std::stoi(buffer);
        m_threshold = m_interval;
        m_count = 0;
    }
}

void Client::heartbeat() {
    if (m_interval.has_value()) {
        if (m_threshold.value() <= 1) {
            auto msg = std::to_string(++m_count * m_interval.value()) + ", ";
            send(m_fd, msg.c_str(), msg.length(), 0);
            m_threshold.emplace(m_interval.value());
        } else {
            m_threshold.emplace(--m_threshold.value());
        }
    }
}
