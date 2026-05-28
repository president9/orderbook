#pragma once
#include <stdint.h>
#include <boost/asio.hpp>
#include "Messages.h"
#include <memory>

using boost::asio::ip::tcp;

class Session : public std::enable_shared_from_this<Session>{
public:
    tcp::socket socket_;
    static constexpr int max_length = 1024;
    uint8_t data_[max_length];
    MessageHeader header_;

    Session(tcp::socket socket) : socket_(std::move(socket)) { }

    void start() { read_header(); }
    
    void read_header() {
        auto self(shared_from_this());
        boost::asio::async_read(socket_, boost::asio::buffer(&header_, sizeof(MessageHeader)),
            [this, self](boost::system::error_code ec, std::size_t )
            {
              if (!ec) {
                    std::cout << "Header: type=" << (int)header_.type << " length=" << header_.length << "\n";
                    read_payload();
              }
            });
    }

    void read_payload(){
        auto self(shared_from_this()); // make this a ptr so that this session stays alive until next async callback fires
        boost::asio::async_read(socket_, boost::asio::buffer(data_, header_.length)
                , [this, self](boost::system::error_code ec, std::size_t){
                if(!ec){handle_message(); read_header(); } });}

    void handle_message(){
        switch(static_cast<MsgType>(header_.type)){
            case MsgType::newOrder:
            {
                auto payload = deserialiseData<newOrderPayload>(data_);
                std::cout << "NewOrder: price=" << payload.price
                << " qty=" << payload.quantity
                << " side=" << (int)payload.priceType
                << " type=" << (int)payload.orderType << "\n";
            }
                break;
            case MsgType::modifyOrder:

                break;
            case MsgType::cancelOrder:

                break;
            case MsgType::userLogin:

                break;
            case MsgType::userLogout:

                break;
            case MsgType::orderAck:

                break;
            case MsgType::orderReject:

                break;
        }
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

