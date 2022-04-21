//
//  Server.hpp
//  nat-test-server
//
//  Created by Oliver Epper on 21.04.22.
//

#ifndef Server_hpp
#define Server_hpp

#include <vector>
#include <optional>
#include "Client.hpp"

class Server {
public:
    Server();
    Server(unsigned int port);
    Server(std::optional<std::string> host, unsigned int port);

    void run();
    void shutdown();
    
private:
    void run_event_loop();
    void heartbeat();
    int m_listen_socket;
    int m_kq;
    std::vector<Client> m_clients;
};

#endif /* Server_hpp */
