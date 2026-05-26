#pragma once
#include "Types.h"
#include <cstddef>
#include <iostream>

struct Limit; 

struct Order {
    Limit* parentLimit;
    size_t quantity;
    int id;
    int price;
    int entryTime;
    int eventTime;

    PriceType side;
    OrderType type;
    bool dontSkipIfFok = false;

    Order(PriceType side, int price, size_t quantity) :
        quantity(quantity),
        price(price),
        side(side)
        { }

    Order(PriceType side, OrderType type, int price, size_t quantity) :
        quantity(quantity),
        price(price),
        side(side),
        type(type)
        { }

    void parseOrder() const {
        std::cout << "Order Id: " << id << '\n';

        std::cout << "PriceType: ";
        if (side == PriceType::Bid) {
            std::cout << "Bid\n";
        } else {
            std::cout << "Ask\n";
        }

        std::cout << "OrderType: ";
        if (type == OrderType::Limit) {
            std::cout << "Limit\n";
        } else if (type == OrderType::Market) {
            std::cout << "Market\n";
        } else if (type == OrderType::IOC) {
            std::cout << "IOC\n";
        } else {
            std::cout << "FOK\n";
        }

        std::cout << "Price: " << price << '\n';
        std::cout << "Quantity: " << quantity << '\n';
        }
};
