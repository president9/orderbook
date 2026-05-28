#pragma once
#include <stdint.h>
#include <boost/asio.hpp>
#include "Client.h"
#include <memory>

using boost::asio::ip::tcp;

class Session : public std::enable_shared_from_this<Session>{
public:
    tcp::socket socket_;
    static constexpr int max_length = 1024;
    uint8_t data_[max_length];

    Session(tcp::socket socket) : socket_(std::move(socket)) { }

    void start() { do_read(); }

    void do_read() {
        auto self(shared_from_this());
        socket_.async_read_some(boost::asio::buffer(data_, max_length),
            [this, self](boost::system::error_code ec, std::size_t length)
            {
              if (!ec)
              {
                do_write(length);
              }
            });
    }

    void do_write(std::size_t length) {
        auto self(shared_from_this());
        boost::asio::async_write(socket_, boost::asio::buffer(data_, length),
            [this, self](boost::system::error_code ec, std::size_t /*length*/)
            {
              if (!ec)
              {
                do_read();
              }
            });
    }
};

class Server{
public: 
    boost::asio::io_context& io_context_;
    boost::asio::ip::tcp::acceptor acceptor_;
    

    Server(boost::asio::io_context& io_context, uint16_t port)
        : io_context_(io_context), acceptor_(io_context, tcp::endpoint(tcp::v4(), port=9003))
    {
        start_accept();
    }

    void start_accept() {
        acceptor_.async_accept(
            [this](boost::system::error_code ec, tcp::socket socket) {
              if (!ec) std::make_shared<Session>(std::move(socket))->start();
              start_accept();
            });
    }

};

