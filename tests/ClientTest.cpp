#include "Client.h"
#include "Messages.h"
#include <iostream>

int main() {
    Client client;
    client.connect("127.0.0.1", 9003);

    auto payload = newOrderPayload{100, 50, static_cast<uint8_t>(PriceType::Bid), static_cast<uint8_t>(OrderType::Limit)};
    auto buffer = serialise(MsgType::newOrder, payload);
    client.send(buffer);
    std::cout << "Sent order: price=100, qty=50, Bid, Limit\n";

    return 0;
}
