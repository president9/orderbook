#pragma once
#include <boost/asio.hpp>
#include <vector>
#include <iostream>

using boost::asio::ip::tcp;

class Client{
public:
    boost::asio::io_context io_context_;
    tcp::socket socket_;

    Client() : socket_(io_context_) {}

    void connect(const std::string& host, uint16_t port = 9003){
        tcp::resolver resolver(io_context_);
        boost::system::error_code ec;
        auto endpoints = resolver.resolve(host, std::to_string(port), ec);
        if (ec) {
            std::cerr << "Resolve failed: " << ec.message() << "\n";
            return;
        }
        boost::asio::connect(socket_, endpoints);
        std::cout << "Connected to " << host << ":" << port << "\n";
    }

    void send(std::vector<uint8_t>& buffer){
        boost::asio::write(socket_, boost::asio::buffer(buffer));
    }

    std::vector<uint8_t> recieve(size_t length){
        std::vector<uint8_t> buffer(length);
        boost::asio::read(socket_, boost::asio::buffer(buffer));
        return buffer;
    }
};
