//
//  main.cpp
//  nat-test-server
//
//  Created by Oliver Epper on 21.04.22.
//

#include <iostream>
#include "Client.hpp"
#include "Server.hpp"

using namespace std;

Server s;

void signal_handler(int signal) {
    s.shutdown();
}

int main(int argc, const char * argv[]) {
    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);

    s.run();

    return EXIT_SUCCESS;
}
