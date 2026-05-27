#pragma once

#include <stdint.h>
#include <Types.h>
#include <vector>
#include <cstring>
#include <iostream>


#pragma pack(push, 1)

struct MessageHeader {
    uint8_t type;       // what kind of message
    uint32_t length;    
};

enum class MsgType : uint8_t {
    newOrder = 1,
    modifyOrder = 2,
    cancelOrder = 3,
    userLogin = 4,
    userLogout = 5,
    orderAck = 6,
    orderReject = 7,
};

struct newOrderPayload {
    uint32_t price;                                
    uint32_t quantity;                                
    uint8_t priceType; // bid,ask
    uint8_t orderType; // limit, market, ioc, fok
    
    void parseOrder(){
        std::cout << price << '\n';
        std::cout << quantity << '\n';
        std::cout << static_cast<int>(priceType) << '\n';
        std::cout << static_cast<int>(orderType) << '\n';
    }
};

struct modifyOrderPayload {
    uint32_t orderId;
    uint32_t price;                                
    uint32_t quantity;                                
};

struct cancelOrderPayload {
    uint32_t orderId;
};

struct userLoginPayload {
    char username[32];
};

struct userLogoutPayload {
    // doesnt need username since socekt already associates it 
    // when userLoginPayload is invoked
    uint8_t logoutCode; // 1 = logout, 0 = timeout/etc
};

struct orderAckPayload {
    uint32_t orderId;
    uint32_t filledQuantity;
    uint8_t status; // 1 = ok, 0 = fail
};


struct orderRejectPayload {
    uint32_t orderId;
    uint8_t reason;
};

#pragma pack(pop)


template<typename Payload>
// this entire packet holds the header and data
std::vector<uint8_t> serialise(MsgType type, const Payload& payload){
    MessageHeader header;
    header.type = static_cast<uint8_t>(type);
    header.length = sizeof(payload);

    std::vector<uint8_t> buffer(sizeof(MessageHeader) + sizeof(Payload));
    std::memcpy(buffer.data(), &header, sizeof(MessageHeader));
    std::memcpy(buffer.data() + sizeof(MessageHeader), &payload, sizeof(Payload));
    return buffer;
}

// consumes the serialised data
// This one just decodes the shit made in above function. Not that it only 
// memcpy's from beginning of nbuffer data but uses siz of message header
MessageHeader deserialiseHeader(const std::vector<uint8_t>& buffer) {
    MessageHeader header;
    std::memcpy(&header, buffer.data(), sizeof(MessageHeader));
    return header;
}


template<typename Payload>
// and this one offsets data.begin to message header then copies
// same exact size for payload
Payload deserialisePayload(const std::vector<uint8_t>& buffer){
    Payload payload;
    std::memcpy(&payload, buffer.data() + sizeof(MessageHeader), sizeof(Payload));
    return payload;
}

