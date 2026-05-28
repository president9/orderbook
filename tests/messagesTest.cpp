#include <cassert>
#include "Messages.h"
#include "iostream"
#include <limits>

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

    // ===== BOUNDARY VALUES =====
    {
        auto p = newOrderPayload{
            std::numeric_limits<uint32_t>::max(),
            std::numeric_limits<uint32_t>::max(),
            static_cast<uint8_t>(PriceType::Bid),
            static_cast<uint8_t>(OrderType::FOK)
        };
        auto buffer = serialise(MsgType::newOrder, p);
        auto d = deserialisePayload<newOrderPayload>(buffer);
        assert(d.price == std::numeric_limits<uint32_t>::max());
        assert(d.quantity == std::numeric_limits<uint32_t>::max());
        std::cout << "TEST 8 PASSED: Max uint32 price and quantity\n";
    }

    {
        auto p = newOrderPayload{0, 0, static_cast<uint8_t>(PriceType::Bid), static_cast<uint8_t>(OrderType::Market)};
        auto buffer = serialise(MsgType::newOrder, p);
        auto d = deserialisePayload<newOrderPayload>(buffer);
        assert(d.price == 0);
        assert(d.quantity == 0);
        std::cout << "TEST 9 PASSED: Zero price and quantity\n";
    }

    {
        auto p = modifyOrderPayload{std::numeric_limits<uint32_t>::max(), 999, 500};
        auto buffer = serialise(MsgType::modifyOrder, p);
        auto d = deserialisePayload<modifyOrderPayload>(buffer);
        assert(d.orderId == std::numeric_limits<uint32_t>::max());
        std::cout << "TEST 10 PASSED: Max order ID in modify\n";
    }

    {
        auto p = cancelOrderPayload{0};
        auto buffer = serialise(MsgType::cancelOrder, p);
        auto d = deserialisePayload<cancelOrderPayload>(buffer);
        assert(d.orderId == 0);
        std::cout << "TEST 11 PASSED: Cancel order ID 0\n";
    }

    // ===== ALL SIDE/TYPE COMBINATIONS =====
    {
        uint8_t sides[] = {static_cast<uint8_t>(PriceType::Bid), static_cast<uint8_t>(PriceType::Ask)};
        uint8_t types[] = {
            static_cast<uint8_t>(OrderType::Limit),
            static_cast<uint8_t>(OrderType::Market),
            static_cast<uint8_t>(OrderType::IOC),
            static_cast<uint8_t>(OrderType::FOK)
        };
        int count = 0;
        for (auto side : sides) {
            for (auto type : types) {
                auto p = newOrderPayload{50, 10, side, type};
                auto buffer = serialise(MsgType::newOrder, p);
                auto d = deserialisePayload<newOrderPayload>(buffer);
                assert(d.priceType == side);
                assert(d.orderType == type);
                count++;
            }
        }
        assert(count == 8);
        std::cout << "TEST 12 PASSED: All 8 side/type combinations\n";
    }

    // ===== USERNAME EDGE CASES =====
    {
        auto p = userLoginPayload{};
        std::memset(p.username, 0, 32);
        auto buffer = serialise(MsgType::userLogin, p);
        auto d = deserialisePayload<userLoginPayload>(buffer);
        assert(d.username[0] == '\0');
        std::cout << "TEST 13 PASSED: Empty username\n";
    }

    {
        auto p = userLoginPayload{};
        std::memset(p.username, 0, 32);
        std::memset(p.username, 'A', 31);
        auto buffer = serialise(MsgType::userLogin, p);
        auto d = deserialisePayload<userLoginPayload>(buffer);
        assert(strlen(d.username) == 31);
        assert(d.username[31] == '\0');
        std::cout << "TEST 14 PASSED: Max length username (31 chars)\n";
    }

    // ===== STRUCT SIZE SANITY =====
    {
        assert(sizeof(MessageHeader) == 5);
        assert(sizeof(newOrderPayload) == 10);
        assert(sizeof(modifyOrderPayload) == 12);
        assert(sizeof(cancelOrderPayload) == 4);
        assert(sizeof(userLoginPayload) == 32);
        assert(sizeof(userLogoutPayload) == 1);
        assert(sizeof(orderAckPayload) == 9);
        assert(sizeof(orderRejectPayload) == 5);
        std::cout << "TEST 15 PASSED: All struct sizes match packed layout\n";
    }

    // ===== SEQUENTIAL STREAM PARSING =====
    {
        auto buf1 = serialise(MsgType::newOrder, newOrderPayload{100, 50, static_cast<uint8_t>(PriceType::Bid), static_cast<uint8_t>(OrderType::Limit)});
        auto buf2 = serialise(MsgType::newOrder, newOrderPayload{200, 75, static_cast<uint8_t>(PriceType::Ask), static_cast<uint8_t>(OrderType::IOC)});
        auto buf3 = serialise(MsgType::cancelOrder, cancelOrderPayload{1});

        std::vector<uint8_t> stream;
        stream.insert(stream.end(), buf1.begin(), buf1.end());
        stream.insert(stream.end(), buf2.begin(), buf2.end());
        stream.insert(stream.end(), buf3.begin(), buf3.end());

        size_t offset = 0;

        MessageHeader h1;
        std::memcpy(&h1, stream.data() + offset, sizeof(MessageHeader));
        assert(h1.type == static_cast<uint8_t>(MsgType::newOrder));
        newOrderPayload d1;
        std::memcpy(&d1, stream.data() + offset + sizeof(MessageHeader), h1.length);
        assert(d1.price == 100 && d1.quantity == 50);
        offset += sizeof(MessageHeader) + h1.length;

        MessageHeader h2;
        std::memcpy(&h2, stream.data() + offset, sizeof(MessageHeader));
        newOrderPayload d2;
        std::memcpy(&d2, stream.data() + offset + sizeof(MessageHeader), h2.length);
        assert(d2.price == 200 && d2.quantity == 75);
        offset += sizeof(MessageHeader) + h2.length;

        MessageHeader h3;
        std::memcpy(&h3, stream.data() + offset, sizeof(MessageHeader));
        assert(h3.type == static_cast<uint8_t>(MsgType::cancelOrder));
        cancelOrderPayload d3;
        std::memcpy(&d3, stream.data() + offset + sizeof(MessageHeader), h3.length);
        assert(d3.orderId == 1);
        offset += sizeof(MessageHeader) + h3.length;

        assert(offset == stream.size());
        std::cout << "TEST 16 PASSED: Sequential message stream parsing\n";
    }

    // ==========================================================
    //  DONE
    // ==========================================================

    std::cout << "\n============================================\n";
    std::cout << "ALL TESTS PASSED\n";
    std::cout << "============================================\n";
    return 0;
}

