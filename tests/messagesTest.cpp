#include <cassert>
#include "Messages.h"
#include "iostream"

int main() {
    // create a buy order
    auto newOrderPayload1 = newOrderPayload(100,1, static_cast<uint8_t>(PriceType::Ask), static_cast<uint8_t>(OrderType::Limit));
    auto buffer = serialise(MsgType::newOrder, newOrderPayload1);

    auto header_decoded = deserialiseHeader(buffer);
    assert(header_decoded.type == 1);
    assert(header_decoded.length == sizeof(newOrderPayload));

    bool all_pass = false;
    if(header_decoded.type == 1){
        auto payload_decoded = deserialisePayload<newOrderPayload>(buffer);
        assert(payload_decoded.price == 100);
        assert(payload_decoded.quantity == 1);
        assert(payload_decoded.priceType == 2);
        assert(payload_decoded.orderType == 1);
        all_pass = true;
    }
    if(all_pass) std::cout << "TEST 1 PASSED: Basic new order payload serial test\n";
    return 0;
}

