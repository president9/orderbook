#include "Server.h"
#include "Book.h"
#include <boost/asio.hpp>
#include <thread>


int main(){
    boost::asio::io_context io;
    Server server(io, 9003);

    std::jthread matching_thread([&](std::stop_token st){
        Order order;
        while (!st.stop_requested()) {
            while (server.order_queue_.pop(order)) {
                std::cout << "Executing order\n";
            
                server.book.insertOrder(order);
                server.book.parseBook();
            }
            std::this_thread::yield();
        }
    });

    boost::asio::signal_set signals(io, SIGINT, SIGTERM);
    signals.async_wait([&](boost::system::error_code, int){
        io.stop();
    });

    io.run();
}
