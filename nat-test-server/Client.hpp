//
//  Client.hpp
//  nat-test-server
//
//  Created by Oliver Epper on 21.04.22.
//

#ifndef Client_hpp
#define Client_hpp

#include <iostream>
#include <optional>

class Client {
public:
    Client(int fd) : m_fd{fd}, m_count{0} {}
    bool has_fd(const uintptr_t fd) const;
    void read();
    void heartbeat();
    friend std::ostream& operator<<(std::ostream& os, const Client& c);
private:
    int m_fd;
    std::optional<int> m_interval;
    std::optional<int> m_threshold;
    int m_count;
};

#endif /* Client_hpp */
