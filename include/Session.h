#pragma once
#include <stdint.h>
#include <boost/asio.hpp>
#include "Messages.h"
#include <memory>
#include <boost/lockfree/spsc_queue.hpp>
#include "Server.h"


using boost::asio::ip::tcp;


class Session : public std::enable_shared_from_this<Session>{
public:
    tcp::socket socket_;
    static constexpr int max_length = 1024;
    uint8_t data_[max_length];
    MessageHeader header_;
    Server& server_;

    Session(tcp::socket socket, Server& server) : 
        socket_(std::move(socket)), server_(server) {}

    void start() { read_header(); }
    
    void read_header() { // is there any point of making self when "this" gets 
                         // passed as a pointer
        auto self(shared_from_this());
        boost::asio::async_read(socket_, boost::asio::buffer(&header_, sizeof(MessageHeader)),
            [this, self](boost::system::error_code ec, std::size_t )
            {
              if (!ec) {
                read_payload();
              }
              else{
                server_.remove_session(self);
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
            {
                auto payload = deserialiseData<modifyOrderPayload>(data_);
                std::cout << "ModifyOrder: id=" << payload.orderId
                << " price=" << (int)payload.price
                << " quantity=" << (int)payload.quantity << "\n";

            }
                break;
            case MsgType::cancelOrder:
            {
                auto payload = deserialiseData<cancelOrderPayload>(data_);
                std::cout << "CancelOrder: id=" << payload.orderId;
            }
                break;
            case MsgType::userLogin:
            {
                auto payload = deserialiseData<userLoginPayload>(data_);
                std::cout << "userLogin: username=" << payload.username;

            }
                break;
            case MsgType::userLogout:
            {
                auto payload = deserialiseData<userLogoutPayload>(data_);
                std::cout << "userLogout: logout code=" << payload.logoutCode;
            }
                break;
            case MsgType::orderAck:
            {
                auto payload = deserialiseData<orderAckPayload>(data_);
                std::cout << "orderRejectPayload: id=" << payload.orderId
                << " filledQuantity=" << (int)payload.filledQuantity
                << " status=" << payload.status << "\n";
            }
                break;
            case MsgType::orderReject:
            {
                auto payload = deserialiseData<orderRejectPayload>(data_);
                std::cout << "orderRejectPayload: id=" << payload.orderId
                << " reason=" << payload.reason << "\n";
            }
                break;
        }
    }

};
