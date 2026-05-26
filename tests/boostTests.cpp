#include <boost/asio.hpp>
#include <iostream>

int main() {
    boost::asio::io_context io;
    boost::asio::steady_timer timer(io, std::chrono::seconds(2));

    std::cout << "Waiting 2 seconds...\n";
    timer.wait();
    std::cout << "Boost.Asio is working.\n";
}
