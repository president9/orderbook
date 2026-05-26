#pragma once
#include <iostream>
#include <cstddef>
#include <map>
#include <unordered_map>
#include <list>
#include "Limit.h"
#include <iostream>

struct Limit; 

struct TrackBook{
    int highestBid = 0;
    int lowestAsk = 0;
    int spread = 0;
};

struct Book{
    TrackBook tracking;
    int nextOrderId = 0;

    std::unordered_map<int, std::list<Order>::iterator> allOrders;
    std::map<int, Limit, std::greater<>> bidTree;
    std::map<int, Limit, std::less<>> askTree;

    void clear() {
        allOrders.clear();
        bidTree.clear();
        askTree.clear();
        nextOrderId = 0;
    }

    void trackLimits() {
        if(!bidTree.empty() && !askTree.empty()){
            tracking.highestBid = bidTree.begin()->first; // accessing so always do nullchecks
            tracking.lowestAsk = askTree.begin()->first;
            tracking.spread = askTree.begin()->first - bidTree.begin()->first;
        }
    }

    void insertOrder(Order& order){
        // push_back makes copy which is expensive
        order.id = ++nextOrderId;
        executeOrder(order);
        // ok issue rn is that order does quantity is not being updated in executeOrder
        if(order.quantity > 0 and order.type == OrderType::Limit){
            if(order.side == PriceType::Bid) {
                auto& limit = bidTree[order.price];
                order.parentLimit = &limit;
                limit.limitPrice = order.price;
                limit.orders.emplace_back(order);
                allOrders[order.id] = std::next(limit.orders.end(), -1);
                limit.size++;
                limit.totalVolume += order.quantity;
            }
            else{
                auto& limit = askTree[order.price];
                order.parentLimit = &limit;
                limit.limitPrice = order.price;
                limit.orders.emplace_back(order);
                allOrders[order.id] = std::next(limit.orders.end(), -1);
                limit.size++;
                limit.totalVolume += order.quantity;
            }
        }
    }

    void executeOrder(Order& order){
        if(order.side == PriceType::Bid){ // have to do this cant lambda cos the types have different comparators
            fillAgainst(order, askTree);
        }
        else{ 
            fillAgainst(order, bidTree);
        }
    }

    template<typename Tree>
    void fillAgainst(Order& order, Tree& tree){
        while(order.quantity > 0 && !tree.empty()){
            auto& bestLimit = tree.begin()->second;
            switch(order.type){
                case OrderType::Limit:
                    if(order.side == PriceType::Ask and order.price > bestLimit.limitPrice) return;
                    if(order.side == PriceType::Bid and order.price < bestLimit.limitPrice) return;
                    break;
                case OrderType::Market:
                    break; // fill normally
                case OrderType::IOC: // basically market but with a price limit
                    if(order.side == PriceType::Ask and order.price > bestLimit.limitPrice) return;
                    if(order.side == PriceType::Bid and order.price < bestLimit.limitPrice) return;
                    break;
                case OrderType::FOK:
                    {
                        if(order.side == PriceType::Ask and order.price > bestLimit.limitPrice) return;
                        if(order.side == PriceType::Bid and order.price < bestLimit.limitPrice) return;
                        
                        if(order.dontSkipIfFok == false){
                        // if(1){
                            if(order.side == PriceType::Bid){
                                size_t volumeUnderPrice = 0;
                                for(auto& x : tree){
                                    if(x.first > order.price) break;
                                    volumeUnderPrice += x.second.totalVolume;
                                }
                                if(volumeUnderPrice < order.quantity) return;
                                
                            }
                            else {
                                size_t volumeUnderPrice = 0;
                                for(auto& x : tree){
                                    if(x.first < order.price) break;
                                    volumeUnderPrice += x.second.totalVolume;
                                }
                                if(volumeUnderPrice < order.quantity) return;
                            }
                            order.dontSkipIfFok = true;
                        }
                    }
            }
            auto& orderToEat = bestLimit.orders.front(); 
            auto matched = std::min(orderToEat.quantity, order.quantity);
            orderToEat.quantity -= matched;
            order.quantity -= matched;
            bestLimit.totalVolume -= matched; 
            if(orderToEat.quantity == 0){
                fillOrder(orderToEat.id);
            }

            // previous erase the bestLimit was here but fillOrder does that now
            // if(order.quantity == 0){
            //     fillOrder(order.id); // no need to run this cos its not even in the tree since
            //     insertion happens after executeOrder();
            // }
        }
    }

    void modifyOrder(int orderId, int newPrice, int newQuantity){
        auto currentOrderIt = allOrders.find(orderId);
        if(currentOrderIt == allOrders.end()) return;
        // auto newOrder = *allOrders[orderId]; to avoid doing using find and []
        auto NewOrder = *(currentOrderIt->second); // copy the order not the iterator
                                                   // or else it will crash
        // An iterator is just an address. Copying an address doesn't duplicate the house.
        cancelOrder(orderId);
        NewOrder.quantity = newQuantity;
        NewOrder.price = newPrice;
        insertOrder(NewOrder);
    }

    void fillOrder(int orderId){ // currently same as cancel. but will be useful for logs
        auto it = allOrders.find(orderId);
        if(it == allOrders.end()) return;

        auto orderIt = it->second;
        auto owningLimit = orderIt->parentLimit;
        PriceType side = orderIt->side;

        auto quantity = orderIt->quantity;
        auto price = orderIt->price; // make copies cos im abt to invalidate the iterator

        // remove order from limit 
        owningLimit->totalVolume -= quantity;
        --(owningLimit->size); // decreases since order dies out
        owningLimit->orders.erase(orderIt);

        // remove the limit from the bid/ask tree
        if(owningLimit->orders.empty() or owningLimit->size == 0){
            if(side == PriceType::Bid){
                auto& tree = bidTree;
                tree.erase(price);
            }
            else{
                auto& tree = askTree;
                tree.erase(price);
            }
        }
        allOrders.erase(it);
        return;
    }


    void cancelOrder(int orderId){
        auto it = allOrders.find(orderId);
        if(it == allOrders.end()) return;

        auto orderIt = it->second;
        auto owningLimit = orderIt->parentLimit;
        PriceType side = orderIt->side;

        auto quantity = orderIt->quantity;
        auto price = orderIt->price; // make copies cos im abt to invalidate the iterator

        // remove order from limit 
        owningLimit->totalVolume -= quantity;
        --(owningLimit->size); // decreases since order dies out
        owningLimit->orders.erase(orderIt);

        // remove the limit from the bid/ask tree
        if(owningLimit->orders.empty() or owningLimit->size == 0){
            if(side == PriceType::Bid){
                auto& tree = bidTree;
                tree.erase(price);
            }
            else{
                auto& tree = askTree;
                tree.erase(price);
            }
        }
        allOrders.erase(it);
        return;
    }

    void parseBook() const{
        std::cout << "\n********** STARTING BOOK PARSE ***********\n";
        std::cout << "========== AskTree ==========\n";
        for(auto& limit : askTree){
            limit.second.parseLimit();
        }
        std::cout << "========== BidTree ==========\n";
        for(auto& limit : bidTree){
            limit.second.parseLimit();
        }
        std::cout << "********** ENDING BOOK PARSE ***********\n";
    }
};


std::ostream& operator<<(std::ostream& os, PriceType side) {
    switch (side) {
        case PriceType::Bid: return os << "Bid";
        case PriceType::Ask: return os << "Ask";
    }
    return os;
}

std::ostream& operator<<(std::ostream& os, OrderType type) {
    switch (type) {
        case OrderType::Limit:  return os << "Limit";
        case OrderType::Market: return os << "Market";
        case OrderType::IOC:    return os << "IOC";
        case OrderType::FOK:    return os << "FOK";
    }
    return os;
}

