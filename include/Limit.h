#pragma once
#include <list>
#include "Order.h"

struct Order; 

struct Limit {
    std::list<Order> orders; 
    // should i make a variable for total volume below this limit too?
    int limitPrice = 0;
    int size = 0;
    int totalVolume = 0;

    void parseLimit() const{
        std::cout << "Limit price: ";
        std::cout << limitPrice << ", Number of orders:" << size << ", Volume: " << totalVolume << '\n';
        for(auto& order : orders){
            order.parseOrder();
        }
    }
};
