#include <cassert>
#include "Messages.h"
#include "iostream"

int main() {
    {
        auto newOrderPayload1 = newOrderPayload(100,1, static_cast<uint8_t>(PriceType::Ask), static_cast<uint8_t>(OrderType::Limit));
        auto buffer = serialise(MsgType::newOrder, newOrderPayload1);
        assert(buffer.size() == sizeof(MessageHeader) + sizeof(newOrderPayload));

        auto header_decoded = deserialiseHeader(buffer);
        assert(header_decoded.type == static_cast<uint8_t>(MsgType::newOrder));
        assert(header_decoded.length == sizeof(newOrderPayload));

        auto payload_decoded = deserialisePayload<newOrderPayload>(buffer);
        assert(payload_decoded.price == 100);
        assert(payload_decoded.quantity == 1);
        assert(payload_decoded.priceType == static_cast<uint8_t>(PriceType::Ask));
        assert(payload_decoded.orderType == 1);
        std::cout << "TEST 1 PASSED: Basic new limit order payload serial test\n";
    }

    {
        auto modifyOrderPayload1 = modifyOrderPayload(1,100, 100);
        auto buffer = serialise(MsgType::modifyOrder, modifyOrderPayload1);
        assert(buffer.size() == sizeof(MessageHeader) + sizeof(modifyOrderPayload));

        auto header_decoded = deserialiseHeader(buffer);
        assert(header_decoded.type == static_cast<uint8_t>(MsgType::modifyOrder));
        assert(header_decoded.length == sizeof(modifyOrderPayload));

        auto payload_decoded = deserialisePayload<modifyOrderPayload>(buffer);
        assert(payload_decoded.orderId == 1);
        assert(payload_decoded.price == 100);
        assert(payload_decoded.quantity == 100);
        std::cout << "TEST 2 PASSED: Basic modify order payload serial test\n";
    }

    {
        auto cancelOrderPayload1 = cancelOrderPayload(1);
        auto buffer = serialise(MsgType::cancelOrder, cancelOrderPayload1);
        assert(buffer.size() == sizeof(MessageHeader) + sizeof(cancelOrderPayload));

        auto header_decoded = deserialiseHeader(buffer);
        assert(header_decoded.type == static_cast<uint8_t>(MsgType::cancelOrder));
        assert(header_decoded.length == sizeof(cancelOrderPayload));

        auto payload_decoded = deserialisePayload<cancelOrderPayload>(buffer);
        assert(payload_decoded.orderId == 1);
        std::cout << "TEST 3 PASSED: Basic cancel order payload serial test\n";
    }

    {
        auto userLoginPayload1 = userLoginPayload("user");
        auto buffer = serialise(MsgType::userLogin ,userLoginPayload1);
        assert(buffer.size() == sizeof(MessageHeader) + sizeof(userLoginPayload));

        auto header_decoded = deserialiseHeader(buffer);
        assert(header_decoded.type == static_cast<uint8_t>(MsgType::userLogin));
        assert(header_decoded.length == sizeof(userLoginPayload));

        auto payload_decoded = deserialisePayload<userLoginPayload>(buffer);
        assert(strcmp(payload_decoded.username, "user") == 0);
        std::cout << "TEST 4 PASSED: Basic login payload serial test\n";
    }

    {
        auto userLogoutPayload1 = userLogoutPayload(1);
        auto buffer = serialise(MsgType::userLogout ,userLogoutPayload1);
        assert(buffer.size() == sizeof(MessageHeader) + sizeof(userLogoutPayload));

        auto header_decoded = deserialiseHeader(buffer);
        assert(header_decoded.type == static_cast<uint8_t>(MsgType::userLogout));
        assert(header_decoded.length == sizeof(userLogoutPayload));

        auto payload_decoded = deserialisePayload<userLogoutPayload>(buffer);
        assert(payload_decoded.logoutCode == 1);
        std::cout << "TEST 5 PASSED: Basic logout payload serial test\n";
    }


    {
        auto orderAckPayload1 = orderAckPayload(1, 1, 1);
        auto buffer = serialise(MsgType::orderAck, orderAckPayload1);
        assert(buffer.size() == sizeof(MessageHeader) + sizeof(orderAckPayload));

        auto header_decoded = deserialiseHeader(buffer);
        assert(header_decoded.type == static_cast<uint8_t>(MsgType::orderAck));
        assert(header_decoded.length == sizeof(orderAckPayload));

        auto payload_decoded = deserialisePayload<orderAckPayload>(buffer);
        assert(payload_decoded.orderId == 1);
        assert(payload_decoded.filledQuantity == 1);
        assert(payload_decoded.status == 1);
        std::cout << "TEST 6 PASSED: Basic orderAck payload serial test\n";
    }

    {
        auto orderRejectPayload1 = orderRejectPayload(1, 1);
        auto buffer = serialise(MsgType::orderReject ,orderRejectPayload1);
        assert(buffer.size() == sizeof(MessageHeader) + sizeof(orderRejectPayload));

        auto header_decoded = deserialiseHeader(buffer);
        assert(header_decoded.type == static_cast<uint8_t>(MsgType::orderReject));
        assert(header_decoded.length == sizeof(orderRejectPayload));

        auto payload_decoded = deserialisePayload<orderRejectPayload>(buffer);
        assert(payload_decoded.orderId == 1);
        assert(payload_decoded.reason == 1);
        std::cout << "TEST 7 PASSED: Basic orderReject payload serial test\n";
    }
    return 0;
}

