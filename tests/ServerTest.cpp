#include "Server.h"
#include <iostream>

int main() {
    boost::asio::io_context io_context;
    Server server(io_context, 9003);
    std::cout << "Server listening on port 9003\n";
    io_context.run();
    return 0;
}
