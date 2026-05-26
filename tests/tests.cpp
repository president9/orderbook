#include <cassert>
#include <iostream>
#include <vector>
#include <random>
#include <climits>
#include "Book.h"

// ============================================================
//  Helper: count total orders across both trees (sanity check)
// ============================================================
static int count_tree_orders(const Book& book) {
    int n = 0;
    for (auto& [price, lim] : book.bidTree) {
        n += (int)lim.orders.size();
    }
    for (auto& [price, lim] : book.askTree) {
        n += (int)lim.orders.size();
    }
    return n;
}

int main() {

// ==========================================================
//  SECTION 1 — BASIC INSERT & BOOK STRUCTURE
// ==========================================================

// TEST 1: No-match insert, book structure
{
    Book book;

    Order o1(PriceType::Ask, OrderType::Limit, 50, 10);
    Order o2(PriceType::Ask, OrderType::Limit, 51, 5);
    Order o3(PriceType::Ask, OrderType::Limit, 50, 3);
    Order o4(PriceType::Bid, OrderType::Limit, 48, 7);
    Order o5(PriceType::Bid, OrderType::Limit, 49, 4);

    book.insertOrder(o1);
    book.insertOrder(o2);
    book.insertOrder(o3);
    book.insertOrder(o4);
    book.insertOrder(o5);

    assert(book.askTree.count(50) == 1);
    assert(book.askTree.count(51) == 1);
    assert(book.askTree.at(50).totalVolume == 13);
    assert(book.askTree.at(50).orders.size() == 2);
    assert(book.askTree.at(51).totalVolume == 5);

    assert(book.bidTree.count(48) == 1);
    assert(book.bidTree.count(49) == 1);
    assert(book.bidTree.at(48).totalVolume == 7);
    assert(book.bidTree.at(49).totalVolume == 4);

    assert(book.allOrders.size() == 5);
    assert(book.askTree.begin()->first == 50); // ask tree sorted ascending

    std::cout << "TEST 1 PASSED: No Match\n";
}

// TEST 2: Limit order matching
{
    Book book;
    Order ask(PriceType::Ask, OrderType::Limit, 50, 10);
    book.insertOrder(ask);
    assert(book.askTree[50].totalVolume == 10);

    Order bid(PriceType::Bid, OrderType::Limit, 50, 5);
    book.insertOrder(bid);
    assert(book.askTree[50].totalVolume == 5);
    assert(book.bidTree.empty());
    std::cout << "TEST 2 PASSED: Limit order matching\n";
}

// TEST 3: Full match clears limit
{
    Book book;
    Order ask(PriceType::Ask, OrderType::Limit, 50, 10);
    book.insertOrder(ask);

    Order bid(PriceType::Bid, OrderType::Limit, 50, 10);
    book.insertOrder(bid);
    assert(book.askTree.empty());
    assert(book.bidTree.empty());
    assert(book.allOrders.empty());
    std::cout << "TEST 3 PASSED: Full match clears limit\n";
}

// ==========================================================
//  SECTION 2 — CANCEL
// ==========================================================

// TEST 4: Cancel order
{
    Book book;
    Order o1(PriceType::Bid, OrderType::Limit, 48, 7);
    Order o2(PriceType::Bid, OrderType::Limit, 48, 3);
    book.insertOrder(o1);
    book.insertOrder(o2);
    assert(book.bidTree[48].totalVolume == 10);
    assert(book.bidTree[48].size == 2);

    book.cancelOrder(o1.id);
    assert(book.bidTree[48].totalVolume == 3);
    assert(book.bidTree[48].size == 1);
    assert(book.allOrders.find(o1.id) == book.allOrders.end());
    std::cout << "TEST 4 PASSED: Cancel order\n";
}

// TEST 5: Cancel last order removes limit
{
    Book book;
    Order o1(PriceType::Ask, OrderType::Limit, 55, 5);
    book.insertOrder(o1);
    assert(book.askTree.count(55) == 1);

    book.cancelOrder(o1.id);
    assert(book.askTree.count(55) == 0);
    assert(book.allOrders.empty());
    std::cout << "TEST 5 PASSED: Cancel last order removes limit\n";
}

// TEST 6: Cancel nonexistent order
{
    Book book;
    book.cancelOrder(9999);
    std::cout << "TEST 6 PASSED: Cancel nonexistent order\n";
}

// ==========================================================
//  SECTION 3 — MARKET ORDERS
// ==========================================================

// TEST 7: Market buy
{
    Book book;
    Order a1(PriceType::Ask, OrderType::Limit, 50, 5);
    Order a2(PriceType::Ask, OrderType::Limit, 51, 5);
    Order a3(PriceType::Ask, OrderType::Limit, 52, 5);
    book.insertOrder(a1);
    book.insertOrder(a2);
    book.insertOrder(a3);

    Order mkt(PriceType::Bid, OrderType::Market, 0, 8);
    book.insertOrder(mkt);

    assert(book.askTree.count(50) == 0);
    assert(book.askTree[51].totalVolume == 2);
    assert(book.askTree[52].totalVolume == 5);
    std::cout << "TEST 7 PASSED: Market buy\n";
}

// TEST 8: Market sell
{
    Book book;
    Order b1(PriceType::Bid, OrderType::Limit, 50, 5);
    Order b2(PriceType::Bid, OrderType::Limit, 49, 5);
    book.insertOrder(b1);
    book.insertOrder(b2);

    Order mkt(PriceType::Ask, OrderType::Market, 0, 7);
    book.insertOrder(mkt);
    assert(book.bidTree.count(50) == 0);
    assert(book.bidTree[49].totalVolume == 3);
    std::cout << "TEST 8 PASSED: Market sell\n";
}

// TEST 9: Market order on empty book
{
    Book book;
    Order mkt(PriceType::Bid, OrderType::Market, 0, 100);
    book.insertOrder(mkt);
    assert(book.bidTree.empty());
    assert(book.askTree.empty());
    assert(book.allOrders.empty());
    std::cout << "TEST 9 PASSED: Market order on empty book\n";
}

// ==========================================================
//  SECTION 4 — FIFO
// ==========================================================

// TEST 10: Multiple orders same price, FIFO
{
    Book book;
    Order a1(PriceType::Ask, OrderType::Limit, 50, 3);
    Order a2(PriceType::Ask, OrderType::Limit, 50, 7);
    book.insertOrder(a1);
    book.insertOrder(a2);

    Order bid(PriceType::Bid, OrderType::Limit, 50, 3);
    book.insertOrder(bid);

    assert(book.askTree[50].totalVolume == 7);
    assert(book.askTree[50].size == 1);
    std::cout << "TEST 10 PASSED: FIFO ordering\n";
}

// TEST 11: Bid above ask triggers match
{
    Book book;
    Order ask(PriceType::Ask, OrderType::Limit, 50, 5);
    book.insertOrder(ask);

    Order bid(PriceType::Bid, OrderType::Limit, 55, 5);
    book.insertOrder(bid);
    assert(book.askTree.empty());
    assert(book.bidTree.empty());
    std::cout << "TEST 11 PASSED: Bid above ask triggers match\n";
}

// ==========================================================
//  SECTION 5 — MODIFY
// ==========================================================

// TEST 12: Modify order — change price
{
    Book book;
    Order o1(PriceType::Bid, OrderType::Limit, 48, 10);
    book.insertOrder(o1);
    assert(book.bidTree[48].totalVolume == 10);

    book.modifyOrder(o1.id, 50, 10);
    assert(book.bidTree.count(48) == 0);
    assert(book.bidTree.count(50) == 1);
    assert(book.bidTree[50].totalVolume == 10);
    assert(book.allOrders.find(o1.id) == book.allOrders.end());
    std::cout << "TEST 12 PASSED: Modify order — change price\n";
}

// TEST 13: Modify order — change quantity
{
    Book book;
    Order o1(PriceType::Ask, OrderType::Limit, 55, 10);
    book.insertOrder(o1);
    assert(book.askTree[55].totalVolume == 10);

    book.modifyOrder(o1.id, 55, 3);
    assert(book.askTree[55].totalVolume == 3);
    assert(book.askTree[55].size == 1);
    std::cout << "TEST 13 PASSED: Modify order — change quantity\n";
}

// TEST 14: Modify order — change price and quantity
{
    Book book;
    Order o1(PriceType::Bid, OrderType::Limit, 45, 8);
    Order o2(PriceType::Bid, OrderType::Limit, 45, 5);
    book.insertOrder(o1);
    book.insertOrder(o2);
    assert(book.bidTree[45].totalVolume == 13);
    assert(book.bidTree[45].size == 2);

    book.modifyOrder(o1.id, 47, 6);
    assert(book.bidTree[45].totalVolume == 5);
    assert(book.bidTree[45].size == 1);
    assert(book.bidTree[47].totalVolume == 6);
    assert(book.bidTree[47].size == 1);
    std::cout << "TEST 14 PASSED: Modify order — change price and quantity\n";
}

// TEST 15: Modify order — triggers match
{
    Book book;
    Order ask(PriceType::Ask, OrderType::Limit, 50, 5);
    Order bid(PriceType::Bid, OrderType::Limit, 45, 5);
    book.insertOrder(ask);
    book.insertOrder(bid);

    book.modifyOrder(bid.id, 50, 5);
    assert(book.askTree.empty());
    assert(book.bidTree.empty());
    std::cout << "TEST 15 PASSED: Modify order — triggers match\n";
}

// TEST 16: Modify nonexistent order
{
    Book book;
    book.modifyOrder(9999, 50, 10);
    std::cout << "TEST 16 PASSED: Modify nonexistent order\n";
}

// ==========================================================
//  SECTION 6 — IOC
// ==========================================================

// TEST 17: IOC — partial fill
{
    Book book;
    Order a1(PriceType::Ask, OrderType::Limit, 50, 3);
    Order a2(PriceType::Ask, OrderType::Limit, 51, 5);
    book.insertOrder(a1);
    book.insertOrder(a2);

    Order ioc(PriceType::Bid, OrderType::IOC, 50, 10);
    book.insertOrder(ioc);
    assert(book.askTree.count(50) == 0);
    assert(book.askTree[51].totalVolume == 5);
    assert(book.bidTree.empty());
    std::cout << "TEST 17 PASSED: IOC — partial fill\n";
}

// TEST 18: IOC — full fill
{
    Book book;
    Order a1(PriceType::Ask, OrderType::Limit, 50, 10);
    book.insertOrder(a1);

    Order ioc(PriceType::Bid, OrderType::IOC, 50, 5);
    book.insertOrder(ioc);
    assert(book.askTree[50].totalVolume == 5);
    assert(book.bidTree.empty());
    std::cout << "TEST 18 PASSED: IOC — full fill\n";
}

// TEST 19: IOC — nothing available
{
    Book book;
    Order ioc(PriceType::Bid, OrderType::IOC, 50, 10);
    book.insertOrder(ioc);
    assert(book.bidTree.empty());
    assert(book.askTree.empty());
    std::cout << "TEST 19 PASSED: IOC — nothing available\n";
}

// TEST 20: IOC sell
{
    Book book;
    Order b1(PriceType::Bid, OrderType::Limit, 50, 5);
    Order b2(PriceType::Bid, OrderType::Limit, 49, 5);
    book.insertOrder(b1);
    book.insertOrder(b2);

    Order ioc(PriceType::Ask, OrderType::IOC, 50, 8);
    book.insertOrder(ioc);
    assert(book.bidTree.count(50) == 0);
    assert(book.bidTree[49].totalVolume == 5);
    assert(book.askTree.empty());
    std::cout << "TEST 20 PASSED: IOC sell\n";
}

// ==========================================================
//  SECTION 7 — FOK
// ==========================================================

// TEST 21: FOK — enough volume, fills entirely
{
    Book book;
    Order a1(PriceType::Ask, OrderType::Limit, 50, 5);
    Order a2(PriceType::Ask, OrderType::Limit, 51, 5);
    book.insertOrder(a1);
    book.insertOrder(a2);

    Order fok(PriceType::Bid, OrderType::FOK, 51, 8);
    book.insertOrder(fok);
    assert(book.askTree.count(50) == 0);
    assert(book.askTree[51].totalVolume == 2);
    assert(book.bidTree.empty());
    std::cout << "TEST 21 PASSED: FOK — enough volume, fills entirely\n";
}

// TEST 22: FOK — not enough volume, killed
{
    Book book;
    Order a1(PriceType::Ask, OrderType::Limit, 50, 3);
    book.insertOrder(a1);

    Order fok(PriceType::Bid, OrderType::FOK, 50, 10);
    book.insertOrder(fok);
    assert(book.askTree[50].totalVolume == 3);
    assert(book.bidTree.empty());
    std::cout << "TEST 22 PASSED: FOK — not enough volume, killed\n";
}

// TEST 23: FOK — not enough volume at acceptable price
{
    Book book;
    Order a1(PriceType::Ask, OrderType::Limit, 50, 3);
    Order a2(PriceType::Ask, OrderType::Limit, 55, 10);
    book.insertOrder(a1);
    book.insertOrder(a2);

    Order fok(PriceType::Bid, OrderType::FOK, 50, 8);
    book.insertOrder(fok);
    assert(book.askTree[50].totalVolume == 3);
    assert(book.askTree[55].totalVolume == 10);
    assert(book.bidTree.empty());
    std::cout << "TEST 23 PASSED: FOK — not enough at acceptable price\n";
}

// TEST 24: FOK — empty book
{
    Book book;
    Order fok(PriceType::Bid, OrderType::FOK, 50, 10);
    book.insertOrder(fok);
    assert(book.bidTree.empty());
    assert(book.askTree.empty());
    std::cout << "TEST 24 PASSED: FOK — empty book\n";
}

// TEST 25: FOK sell — exact fill
{
    Book book;
    Order b1(PriceType::Bid, OrderType::Limit, 50, 5);
    Order b2(PriceType::Bid, OrderType::Limit, 49, 5);
    book.insertOrder(b1);
    book.insertOrder(b2);

    Order fok(PriceType::Ask, OrderType::FOK, 49, 10);
    book.insertOrder(fok);
    assert(book.bidTree.empty());
    assert(book.askTree.empty());
    std::cout << "TEST 25 PASSED: FOK sell — exact fill\n";
}

// ==========================================================
//  SECTION 8 — TRACKING
// ==========================================================

// TEST 26: Track basic best bid/ask/spread
{
    Book book;
    Order a1(PriceType::Ask, OrderType::Limit, 52, 10);
    Order a2(PriceType::Ask, OrderType::Limit, 50, 5);
    Order b1(PriceType::Bid, OrderType::Limit, 48, 7);
    Order b2(PriceType::Bid, OrderType::Limit, 49, 3);
    book.insertOrder(a1);
    book.insertOrder(a2);
    book.insertOrder(b1);
    book.insertOrder(b2);

    book.trackLimits();
    assert(book.tracking.highestBid == 49);
    assert(book.tracking.lowestAsk == 50);
    assert(book.tracking.spread == 1);
    std::cout << "TEST 26 PASSED: Basic tracking\n";
}

// TEST 27: Single bid and ask level
{
    Book book;
    Order ask(PriceType::Ask, OrderType::Limit, 100, 10);
    Order bid(PriceType::Bid, OrderType::Limit, 95, 10);
    book.insertOrder(ask);
    book.insertOrder(bid);

    book.trackLimits();
    assert(book.tracking.highestBid == 95);
    assert(book.tracking.lowestAsk == 100);
    assert(book.tracking.spread == 5);
    std::cout << "TEST 27 PASSED: Single level tracking\n";
}

// TEST 28: Empty book tracking safe
{
    Book book;
    book.trackLimits();
    std::cout << "TEST 28 PASSED: Empty book tracking safe\n";
}

// TEST 29: Track after full match
{
    Book book;
    Order a1(PriceType::Ask, OrderType::Limit, 50, 5);
    Order b1(PriceType::Bid, OrderType::Limit, 50, 5);
    book.insertOrder(a1);
    book.insertOrder(b1);

    book.trackLimits();
    assert(book.bidTree.empty());
    assert(book.askTree.empty());
    std::cout << "TEST 29 PASSED: Track after full match\n";
}

// TEST 30: Tracking updates after new order arrival
{
    Book book;
    Order b1(PriceType::Bid, OrderType::Limit, 48, 10);
    Order a1(PriceType::Ask, OrderType::Limit, 55, 10);
    book.insertOrder(b1);
    book.insertOrder(a1);

    book.trackLimits();
    assert(book.tracking.spread == 7);

    Order b2(PriceType::Bid, OrderType::Limit, 53, 5);
    book.insertOrder(b2);
    book.trackLimits();
    assert(book.tracking.highestBid == 53);
    assert(book.tracking.spread == 2);
    std::cout << "TEST 30 PASSED: Dynamic tracking updates\n";
}

// ==========================================================
//  SECTION 9 — COMBO / STRESS (original 31–53)
// ==========================================================

// TEST 31: FOK — multiple orders at same price level
{
    Book book;
    Order a1(PriceType::Ask, OrderType::Limit, 50, 2);
    Order a2(PriceType::Ask, OrderType::Limit, 50, 3);
    Order a3(PriceType::Ask, OrderType::Limit, 50, 4);
    book.insertOrder(a1);
    book.insertOrder(a2);
    book.insertOrder(a3);

    Order fok(PriceType::Bid, OrderType::FOK, 50, 7);
    book.insertOrder(fok);
    assert(book.askTree[50].totalVolume == 2);
    std::cout << "TEST 31 PASSED: FOK multiple orders same level\n";
}

// TEST 32: Partial fill — incoming limit order stays in book
{
    Book book;
    Order ask(PriceType::Ask, OrderType::Limit, 50, 3);
    book.insertOrder(ask);

    Order bid(PriceType::Bid, OrderType::Limit, 50, 10);
    book.insertOrder(bid);
    assert(book.askTree.empty());
    assert(book.bidTree.count(50) == 1);
    assert(book.bidTree[50].totalVolume == 7);
    std::cout << "TEST 32 PASSED: Partial fill incoming limit order\n";
}

// TEST 33: Partial fill then second order fills remainder
{
    Book book;
    Order ask1(PriceType::Ask, OrderType::Limit, 50, 3);
    book.insertOrder(ask1);

    Order bid(PriceType::Bid, OrderType::Limit, 50, 10);
    book.insertOrder(bid);

    Order ask2(PriceType::Ask, OrderType::Limit, 50, 7);
    book.insertOrder(ask2);
    assert(book.askTree.empty());
    assert(book.bidTree.empty());
    std::cout << "TEST 33 PASSED: Partial fill then remainder filled\n";
}

// TEST 34: Cancel after partial fill
{
    Book book;
    Order ask(PriceType::Ask, OrderType::Limit, 50, 3);
    book.insertOrder(ask);

    Order bid(PriceType::Bid, OrderType::Limit, 50, 10);
    book.insertOrder(bid);
    assert(book.bidTree[50].totalVolume == 7);

    book.cancelOrder(bid.id);
    assert(book.bidTree.empty());
    assert(book.allOrders.empty());
    std::cout << "TEST 34 PASSED: Cancel after partial fill\n";
}

// TEST 35: Market buy eats across multiple price levels
{
    Book book;
    Order a1(PriceType::Ask, OrderType::Limit, 50, 2);
    Order a2(PriceType::Ask, OrderType::Limit, 51, 2);
    Order a3(PriceType::Ask, OrderType::Limit, 52, 2);
    Order a4(PriceType::Ask, OrderType::Limit, 53, 2);
    book.insertOrder(a1);
    book.insertOrder(a2);
    book.insertOrder(a3);
    book.insertOrder(a4);

    Order mkt(PriceType::Bid, OrderType::Market, 0, 5);
    book.insertOrder(mkt);
    assert(book.askTree.count(50) == 0);
    assert(book.askTree.count(51) == 0);
    assert(book.askTree[52].totalVolume == 1);
    assert(book.askTree[53].totalVolume == 2);
    std::cout << "TEST 35 PASSED: Market buy across levels\n";
}

// TEST 36: Market order larger than total book volume
{
    Book book;
    Order a1(PriceType::Ask, OrderType::Limit, 50, 3);
    Order a2(PriceType::Ask, OrderType::Limit, 51, 2);
    book.insertOrder(a1);
    book.insertOrder(a2);

    Order mkt(PriceType::Bid, OrderType::Market, 0, 100);
    book.insertOrder(mkt);
    assert(book.askTree.empty());
    assert(book.bidTree.empty());
    std::cout << "TEST 36 PASSED: Market order exceeds book volume\n";
}

// TEST 37: Insert after emptying book
{
    Book book;
    Order ask(PriceType::Ask, OrderType::Limit, 50, 5);
    Order bid(PriceType::Bid, OrderType::Limit, 50, 5);
    book.insertOrder(ask);
    book.insertOrder(bid);
    assert(book.askTree.empty());
    assert(book.bidTree.empty());

    Order newAsk(PriceType::Ask, OrderType::Limit, 60, 10);
    Order newBid(PriceType::Bid, OrderType::Limit, 55, 8);
    book.insertOrder(newAsk);
    book.insertOrder(newBid);
    assert(book.askTree[60].totalVolume == 10);
    assert(book.bidTree[55].totalVolume == 8);
    std::cout << "TEST 37 PASSED: Insert after empty book\n";
}

// TEST 38: IOC eats across multiple price levels
{
    Book book;
    Order a1(PriceType::Ask, OrderType::Limit, 48, 3);
    Order a2(PriceType::Ask, OrderType::Limit, 49, 3);
    Order a3(PriceType::Ask, OrderType::Limit, 50, 3);
    Order a4(PriceType::Ask, OrderType::Limit, 55, 10);
    book.insertOrder(a1);
    book.insertOrder(a2);
    book.insertOrder(a3);
    book.insertOrder(a4);

    Order ioc(PriceType::Bid, OrderType::IOC, 50, 10);
    book.insertOrder(ioc);
    assert(book.askTree.count(48) == 0);
    assert(book.askTree.count(49) == 0);
    assert(book.askTree.count(50) == 0);
    assert(book.askTree[55].totalVolume == 10);
    assert(book.bidTree.empty());
    std::cout << "TEST 38 PASSED: IOC across multiple levels\n";
}

// TEST 39: Multiple cancels in sequence
{
    Book book;
    Order o1(PriceType::Bid, OrderType::Limit, 50, 5);
    Order o2(PriceType::Bid, OrderType::Limit, 50, 5);
    Order o3(PriceType::Bid, OrderType::Limit, 50, 5);
    book.insertOrder(o1);
    book.insertOrder(o2);
    book.insertOrder(o3);
    assert(book.bidTree[50].totalVolume == 15);

    book.cancelOrder(o2.id);
    assert(book.bidTree[50].totalVolume == 10);
    assert(book.bidTree[50].size == 2);

    book.cancelOrder(o1.id);
    assert(book.bidTree[50].totalVolume == 5);
    assert(book.bidTree[50].size == 1);

    book.cancelOrder(o3.id);
    assert(book.bidTree.empty());
    std::cout << "TEST 39 PASSED: Multiple cancels\n";
}

// TEST 40: Double cancel same order
{
    Book book;
    Order o1(PriceType::Bid, OrderType::Limit, 50, 5);
    book.insertOrder(o1);
    book.cancelOrder(o1.id);
    book.cancelOrder(o1.id);
    assert(book.bidTree.empty());
    std::cout << "TEST 40 PASSED: Double cancel\n";
}

// TEST 41: FIFO across partial fills
{
    Book book;
    Order a1(PriceType::Ask, OrderType::Limit, 50, 5);
    Order a2(PriceType::Ask, OrderType::Limit, 50, 5);
    Order a3(PriceType::Ask, OrderType::Limit, 50, 5);
    book.insertOrder(a1);
    book.insertOrder(a2);
    book.insertOrder(a3);

    Order bid(PriceType::Bid, OrderType::Limit, 50, 7);
    book.insertOrder(bid);
    assert(book.askTree[50].totalVolume == 8);
    assert(book.askTree[50].orders.size() == 2);
    std::cout << "TEST 41 PASSED: FIFO across partial fills\n";
}

// TEST 42: Modify to price that triggers match
{
    Book book;
    Order ask(PriceType::Ask, OrderType::Limit, 50, 5);
    Order bid(PriceType::Bid, OrderType::Limit, 40, 10);
    book.insertOrder(ask);
    book.insertOrder(bid);

    book.modifyOrder(bid.id, 50, 10);
    assert(book.askTree.empty());
    assert(book.bidTree.begin()->second.totalVolume == 5);
    std::cout << "TEST 42 PASSED: Modify triggers partial match\n";
}

// TEST 43: Spread narrows and widens
{
    Book book;
    Order a1(PriceType::Ask, OrderType::Limit, 60, 5);
    Order b1(PriceType::Bid, OrderType::Limit, 40, 5);
    book.insertOrder(a1);
    book.insertOrder(b1);
    book.trackLimits();
    assert(book.tracking.spread == 20);

    Order a2(PriceType::Ask, OrderType::Limit, 45, 5);
    book.insertOrder(a2);
    book.trackLimits();
    assert(book.tracking.spread == 5);

    book.cancelOrder(a2.id);
    book.trackLimits();
    assert(book.tracking.spread == 20);
    std::cout << "TEST 43 PASSED: Spread narrows and widens\n";
}

// TEST 44: Cascading matches — bid triggers multiple ask levels
{
    Book book;
    Order a1(PriceType::Ask, OrderType::Limit, 50, 2);
    Order a2(PriceType::Ask, OrderType::Limit, 51, 2);
    Order a3(PriceType::Ask, OrderType::Limit, 52, 2);
    Order a4(PriceType::Ask, OrderType::Limit, 53, 2);
    Order a5(PriceType::Ask, OrderType::Limit, 54, 2);
    book.insertOrder(a1);
    book.insertOrder(a2);
    book.insertOrder(a3);
    book.insertOrder(a4);
    book.insertOrder(a5);

    Order bid(PriceType::Bid, OrderType::Limit, 54, 9);
    book.insertOrder(bid);
    assert(book.askTree.count(50) == 0);
    assert(book.askTree.count(51) == 0);
    assert(book.askTree.count(52) == 0);
    assert(book.askTree.count(53) == 0);
    assert(book.askTree[54].totalVolume == 1);
    assert(book.bidTree.empty());
    std::cout << "TEST 44 PASSED: Cascading match across 5 levels\n";
}

// TEST 45: Interleaved inserts and cancels
{
    Book book;
    Order a1(PriceType::Ask, OrderType::Limit, 50, 10);
    Order a2(PriceType::Ask, OrderType::Limit, 51, 10);
    Order b1(PriceType::Bid, OrderType::Limit, 48, 10);
    Order b2(PriceType::Bid, OrderType::Limit, 47, 10);
    book.insertOrder(a1);
    book.insertOrder(b1);
    book.insertOrder(a2);
    book.insertOrder(b2);

    book.cancelOrder(a1.id);
    book.cancelOrder(b1.id);

    assert(book.askTree.count(50) == 0);
    assert(book.bidTree.count(48) == 0);
    assert(book.askTree[51].totalVolume == 10);
    assert(book.bidTree[47].totalVolume == 10);

    Order b3(PriceType::Bid, OrderType::Limit, 50, 5);
    book.insertOrder(b3);
    assert(book.askTree[51].totalVolume == 10);
    assert(book.bidTree[50].totalVolume == 5);
    std::cout << "TEST 45 PASSED: Interleaved inserts and cancels\n";
}

// TEST 46: Modify order that was partially filled
{
    Book book;
    Order ask(PriceType::Ask, OrderType::Limit, 50, 3);
    Order bid(PriceType::Bid, OrderType::Limit, 50, 10);
    book.insertOrder(ask);
    book.insertOrder(bid);
    assert(book.bidTree[50].totalVolume == 7);

    book.modifyOrder(bid.id, 45, 5);
    assert(book.bidTree.count(50) == 0);
    assert(book.bidTree[45].totalVolume == 5);
    std::cout << "TEST 46 PASSED: Modify after partial fill\n";
}

// TEST 47: FOK across multiple levels with multiple orders per level
{
    Book book;
    Order a1(PriceType::Ask, OrderType::Limit, 50, 2);
    Order a2(PriceType::Ask, OrderType::Limit, 50, 3);
    Order a3(PriceType::Ask, OrderType::Limit, 51, 4);
    Order a4(PriceType::Ask, OrderType::Limit, 51, 1);
    Order a5(PriceType::Ask, OrderType::Limit, 52, 5);
    book.insertOrder(a1);
    book.insertOrder(a2);
    book.insertOrder(a3);
    book.insertOrder(a4);
    book.insertOrder(a5);

    // FOK buy 12 at max 51 — only 10 available, reject
    Order fok1(PriceType::Bid, OrderType::FOK, 51, 12);
    book.insertOrder(fok1);
    assert(book.askTree[50].totalVolume == 5);
    assert(book.askTree[51].totalVolume == 5);
    assert(book.bidTree.empty());

    // FOK buy 9 at max 51 — 10 available, fills
    Order fok2(PriceType::Bid, OrderType::FOK, 51, 9);
    book.insertOrder(fok2);
    assert(book.askTree.count(50) == 0);
    assert(book.askTree[51].totalVolume == 1);
    assert(book.askTree[52].totalVolume == 5);
    assert(book.bidTree.empty());
    std::cout << "TEST 47 PASSED: FOK across levels with multiple orders\n";
}

// TEST 48: Rapid fire — 100 orders alternating sides
{
    Book book;
    for (int i = 0; i < 100; ++i) {
        if (i % 2 == 0) {
            Order ask(PriceType::Ask, OrderType::Limit, 100 + (i % 10), 1);
            book.insertOrder(ask);
        } else {
            Order bid(PriceType::Bid, OrderType::Limit, 90 + (i % 10), 1);
            book.insertOrder(bid);
        }
    }
    assert(!book.bidTree.empty());
    assert(!book.askTree.empty());
    assert(book.bidTree.begin()->first <= 99);
    assert(book.askTree.begin()->first >= 100);
    std::cout << "TEST 48 PASSED: Rapid fire 100 orders\n";
}

// TEST 49: Market order eats FIFO within each level
{
    Book book;
    Order a1(PriceType::Ask, OrderType::Limit, 50, 3);
    Order a2(PriceType::Ask, OrderType::Limit, 50, 4);
    Order a3(PriceType::Ask, OrderType::Limit, 50, 5);
    Order a4(PriceType::Ask, OrderType::Limit, 51, 10);
    book.insertOrder(a1);
    book.insertOrder(a2);
    book.insertOrder(a3);
    book.insertOrder(a4);

    Order mkt(PriceType::Bid, OrderType::Market, 0, 10);
    book.insertOrder(mkt);
    assert(book.askTree[50].totalVolume == 2);
    assert(book.askTree[50].orders.size() == 1);
    assert(book.askTree[51].totalVolume == 10);
    std::cout << "TEST 49 PASSED: Market eats FIFO within level\n";
}

// TEST 50: Cancel middle order preserves others
{
    Book book;
    Order a1(PriceType::Ask, OrderType::Limit, 50, 5);
    Order a2(PriceType::Ask, OrderType::Limit, 50, 10);
    Order a3(PriceType::Ask, OrderType::Limit, 50, 5);
    book.insertOrder(a1);
    book.insertOrder(a2);
    book.insertOrder(a3);

    book.cancelOrder(a2.id);
    assert(book.askTree[50].totalVolume == 10);
    assert(book.askTree[50].size == 2);
    assert(book.askTree[50].orders.size() == 2);

    Order bid(PriceType::Bid, OrderType::Limit, 50, 7);
    book.insertOrder(bid);
    assert(book.askTree[50].totalVolume == 3);
    assert(book.askTree[50].orders.size() == 1);
    std::cout << "TEST 50 PASSED: Cancel middle preserves FIFO\n";
}

// TEST 51: Modify into a match that partially fills
{
    Book book;
    Order a1(PriceType::Ask, OrderType::Limit, 50, 3);
    Order a2(PriceType::Ask, OrderType::Limit, 51, 3);
    book.insertOrder(a1);
    book.insertOrder(a2);

    Order bid(PriceType::Bid, OrderType::Limit, 40, 10);
    book.insertOrder(bid);

    book.modifyOrder(bid.id, 50, 10);
    assert(book.askTree.count(50) == 0);
    assert(book.askTree[51].totalVolume == 3);
    assert(book.bidTree[50].totalVolume == 7);
    std::cout << "TEST 51 PASSED: Modify into partial match\n";
}

// TEST 52: IOC sell across levels
{
    Book book;
    Order b1(PriceType::Bid, OrderType::Limit, 53, 3);
    Order b2(PriceType::Bid, OrderType::Limit, 52, 3);
    Order b3(PriceType::Bid, OrderType::Limit, 51, 3);
    Order b4(PriceType::Bid, OrderType::Limit, 50, 3);
    Order b5(PriceType::Bid, OrderType::Limit, 45, 10);
    book.insertOrder(b1);
    book.insertOrder(b2);
    book.insertOrder(b3);
    book.insertOrder(b4);
    book.insertOrder(b5);

    Order ioc(PriceType::Ask, OrderType::IOC, 50, 11);
    book.insertOrder(ioc);
    assert(book.bidTree.count(53) == 0);
    assert(book.bidTree.count(52) == 0);
    assert(book.bidTree.count(51) == 0);
    assert(book.bidTree[50].totalVolume == 1);
    assert(book.bidTree[45].totalVolume == 10);
    assert(book.askTree.empty());
    std::cout << "TEST 52 PASSED: IOC sell across levels\n";
}

// TEST 53: Stress — insert, match, cancel, modify sequence
{
    Book book;
    Order a1(PriceType::Ask, OrderType::Limit, 100, 50);
    Order a2(PriceType::Ask, OrderType::Limit, 101, 50);
    Order a3(PriceType::Ask, OrderType::Limit, 102, 50);
    Order b1(PriceType::Bid, OrderType::Limit, 98, 50);
    Order b2(PriceType::Bid, OrderType::Limit, 97, 50);
    book.insertOrder(a1);
    book.insertOrder(a2);
    book.insertOrder(a3);
    book.insertOrder(b1);
    book.insertOrder(b2);

    Order mkt(PriceType::Bid, OrderType::Market, 0, 60);
    book.insertOrder(mkt);
    assert(book.askTree.count(100) == 0);
    assert(book.askTree[101].totalVolume == 40);

    book.cancelOrder(b2.id);
    assert(book.bidTree.count(97) == 0);

    book.modifyOrder(b1.id, 101, 30);
    assert(book.askTree[101].totalVolume == 10);
    assert(book.bidTree.empty());

    Order ioc(PriceType::Bid, OrderType::IOC, 102, 100);
    book.insertOrder(ioc);
    assert(book.askTree.empty());
    assert(book.bidTree.empty());
    std::cout << "TEST 53 PASSED: Full sequence stress test\n";
}

// ==========================================================
//  SECTION 10 — NEW TESTS: allOrders CONSISTENCY
// ==========================================================

// TEST 54: allOrders count always matches tree order count
{
    Book book;
    Order a1(PriceType::Ask, OrderType::Limit, 50, 5);
    Order a2(PriceType::Ask, OrderType::Limit, 51, 5);
    Order b1(PriceType::Bid, OrderType::Limit, 48, 5);
    book.insertOrder(a1);
    book.insertOrder(a2);
    book.insertOrder(b1);
    assert((int)book.allOrders.size() == count_tree_orders(book));

    book.cancelOrder(a1.id);
    assert((int)book.allOrders.size() == count_tree_orders(book));

    // match should keep them in sync
    Order b2(PriceType::Bid, OrderType::Limit, 51, 5);
    book.insertOrder(b2);
    assert((int)book.allOrders.size() == count_tree_orders(book));

    std::cout << "TEST 54 PASSED: allOrders consistency\n";
}

// TEST 55: Market order never appears in allOrders
{
    Book book;
    Order ask(PriceType::Ask, OrderType::Limit, 50, 100);
    book.insertOrder(ask);
    int before = (int)book.allOrders.size();

    Order mkt(PriceType::Bid, OrderType::Market, 0, 5);
    book.insertOrder(mkt);

    // market order should not be resting — allOrders should not grow
    // (one ask partially filled is still there, market order is gone)
    assert((int)book.allOrders.size() == before);
    assert(book.bidTree.empty());
    std::cout << "TEST 55 PASSED: Market order not in allOrders\n";
}

// TEST 56: IOC order never rests in allOrders
{
    Book book;
    Order ask(PriceType::Ask, OrderType::Limit, 50, 100);
    book.insertOrder(ask);

    Order ioc(PriceType::Bid, OrderType::IOC, 50, 5);
    book.insertOrder(ioc);

    // ioc should have filled and not be in allOrders
    assert(book.allOrders.find(ioc.id) == book.allOrders.end());
    assert(book.bidTree.empty());

    // partially filled IOC also shouldn't rest
    Order ioc2(PriceType::Bid, OrderType::IOC, 40, 5);  // price too low, no fill
    book.insertOrder(ioc2);
    assert(book.allOrders.find(ioc2.id) == book.allOrders.end());
    assert(book.bidTree.empty());
    std::cout << "TEST 56 PASSED: IOC never rests in allOrders\n";
}

// TEST 57: FOK rejected order never appears in allOrders
{
    Book book;
    Order ask(PriceType::Ask, OrderType::Limit, 50, 3);
    book.insertOrder(ask);

    Order fok(PriceType::Bid, OrderType::FOK, 50, 100);  // not enough volume
    book.insertOrder(fok);
    assert(book.allOrders.find(fok.id) == book.allOrders.end());
    assert(book.bidTree.empty());
    std::cout << "TEST 57 PASSED: Rejected FOK not in allOrders\n";
}

// ==========================================================
//  SECTION 11 — NEW TESTS: ORDER ID BEHAVIOR
// ==========================================================

// TEST 58: Order IDs are monotonically increasing
{
    Book book;
    Order o1(PriceType::Bid, OrderType::Limit, 50, 5);
    Order o2(PriceType::Ask, OrderType::Limit, 55, 5);
    Order o3(PriceType::Bid, OrderType::Limit, 49, 5);
    book.insertOrder(o1);
    book.insertOrder(o2);
    book.insertOrder(o3);

    assert(o1.id < o2.id);
    assert(o2.id < o3.id);
    std::cout << "TEST 58 PASSED: Order IDs monotonically increasing\n";
}

// TEST 59: Modify generates a new order ID (old ID is dead)
{
    Book book;
    Order o1(PriceType::Bid, OrderType::Limit, 50, 5);
    book.insertOrder(o1);
    int old_id = o1.id;

    book.modifyOrder(old_id, 51, 5);

    assert(book.allOrders.find(old_id) == book.allOrders.end());
    // the new order should have a higher ID
    assert(book.allOrders.size() == 1);
    int new_id = book.allOrders.begin()->first;
    assert(new_id > old_id);
    std::cout << "TEST 59 PASSED: Modify generates new ID\n";
}

// ==========================================================
//  SECTION 12 — NEW TESTS: EDGE VALUES
// ==========================================================

// TEST 60: Quantity 1 orders
{
    Book book;
    Order ask(PriceType::Ask, OrderType::Limit, 50, 1);
    Order bid(PriceType::Bid, OrderType::Limit, 50, 1);
    book.insertOrder(ask);
    book.insertOrder(bid);
    assert(book.askTree.empty());
    assert(book.bidTree.empty());
    assert(book.allOrders.empty());
    std::cout << "TEST 60 PASSED: Quantity 1 orders match and clear\n";
}

// TEST 61: Many orders at quantity 1, FIFO
{
    Book book;
    std::vector<int> ids;
    for (int i = 0; i < 100; i++) {
        Order o(PriceType::Ask, OrderType::Limit, 50, 1);
        book.insertOrder(o);
        ids.push_back(o.id);
    }
    assert(book.askTree[50].totalVolume == 100);
    assert(book.askTree[50].size == 100);

    // eat 50 — should eat the first 50 (FIFO)
    Order bid(PriceType::Bid, OrderType::Limit, 50, 50);
    book.insertOrder(bid);
    assert(book.askTree[50].totalVolume == 50);
    assert(book.askTree[50].size == 50);

    // verify the first 50 IDs are gone
    for (int i = 0; i < 50; i++) {
        assert(book.allOrders.find(ids[i]) == book.allOrders.end());
    }
    // and the last 50 are still there
    for (int i = 50; i < 100; i++) {
        assert(book.allOrders.find(ids[i]) != book.allOrders.end());
    }
    std::cout << "TEST 61 PASSED: 100 qty-1 orders, FIFO verified by ID\n";
}

// TEST 62: Large quantity
{
    Book book;
    Order ask(PriceType::Ask, OrderType::Limit, 50, 1'000'000);
    book.insertOrder(ask);
    assert(book.askTree[50].totalVolume == 1'000'000);

    Order bid(PriceType::Bid, OrderType::Limit, 50, 999'999);
    book.insertOrder(bid);
    assert(book.askTree[50].totalVolume == 1);
    assert(book.bidTree.empty());
    std::cout << "TEST 62 PASSED: Large quantity\n";
}

// TEST 63: Price 0
{
    Book book;
    Order ask(PriceType::Ask, OrderType::Limit, 0, 5);
    book.insertOrder(ask);
    assert(book.askTree.count(0) == 1);
    assert(book.askTree[0].totalVolume == 5);

    Order bid(PriceType::Bid, OrderType::Limit, 0, 5);
    book.insertOrder(bid);
    assert(book.askTree.empty());
    assert(book.bidTree.empty());
    std::cout << "TEST 63 PASSED: Price 0\n";
}

// TEST 64: Very high price
{
    Book book;
    Order ask(PriceType::Ask, OrderType::Limit, 999'999, 5);
    Order bid(PriceType::Bid, OrderType::Limit, 1, 5);
    book.insertOrder(ask);
    book.insertOrder(bid);

    // no match — spread is huge
    assert(book.askTree[999'999].totalVolume == 5);
    assert(book.bidTree[1].totalVolume == 5);

    book.trackLimits();
    assert(book.tracking.spread == 999'998);
    std::cout << "TEST 64 PASSED: Very high price\n";
}

// ==========================================================
//  SECTION 13 — NEW TESTS: clear()
// ==========================================================

// TEST 65: clear() resets everything
{
    Book book;
    for (int i = 0; i < 100; i++) {
        Order o(PriceType::Bid, OrderType::Limit, 50 + (i % 10), 5);
        book.insertOrder(o);
    }
    assert(!book.allOrders.empty());

    book.clear();
    assert(book.allOrders.empty());
    assert(book.bidTree.empty());
    assert(book.askTree.empty());
    assert(book.nextOrderId == 0);
    std::cout << "TEST 65 PASSED: clear() resets everything\n";
}

// TEST 66: Insert after clear works correctly
{
    Book book;
    Order o1(PriceType::Ask, OrderType::Limit, 50, 5);
    book.insertOrder(o1);

    book.clear();

    Order o2(PriceType::Bid, OrderType::Limit, 48, 10);
    book.insertOrder(o2);
    // IDs reset, so new ID should be 1
    assert(o2.id == 1);
    assert(book.bidTree[48].totalVolume == 10);
    assert(book.allOrders.size() == 1);
    std::cout << "TEST 66 PASSED: Insert after clear\n";
}

// ==========================================================
//  SECTION 14 — NEW TESTS: AGGRESSIVE ASK (ask crossing bid)
// ==========================================================

// TEST 67: Ask below best bid triggers match
{
    Book book;
    Order bid(PriceType::Bid, OrderType::Limit, 55, 5);
    book.insertOrder(bid);

    // ask at 50 < bid at 55, should match
    Order ask(PriceType::Ask, OrderType::Limit, 50, 5);
    book.insertOrder(ask);
    assert(book.bidTree.empty());
    assert(book.askTree.empty());
    std::cout << "TEST 67 PASSED: Ask below best bid triggers match\n";
}

// TEST 68: Aggressive ask sweeps multiple bid levels
{
    Book book;
    Order b1(PriceType::Bid, OrderType::Limit, 55, 2);
    Order b2(PriceType::Bid, OrderType::Limit, 54, 2);
    Order b3(PriceType::Bid, OrderType::Limit, 53, 2);
    Order b4(PriceType::Bid, OrderType::Limit, 52, 2);
    book.insertOrder(b1);
    book.insertOrder(b2);
    book.insertOrder(b3);
    book.insertOrder(b4);

    // aggressive ask at 53 eats bids at 55, 54, 53
    Order ask(PriceType::Ask, OrderType::Limit, 53, 5);
    book.insertOrder(ask);
    assert(book.bidTree.count(55) == 0);
    assert(book.bidTree.count(54) == 0);
    assert(book.bidTree[53].totalVolume == 1);  // 2-1
    assert(book.bidTree[52].totalVolume == 2);  // untouched
    assert(book.askTree.empty());
    std::cout << "TEST 68 PASSED: Aggressive ask sweeps bid levels\n";
}

// ==========================================================
//  SECTION 15 — NEW TESTS: Limit.size vs orders.size() sync
// ==========================================================

// TEST 69: Limit size field matches orders list size at all times
{
    Book book;
    Order a1(PriceType::Ask, OrderType::Limit, 50, 5);
    Order a2(PriceType::Ask, OrderType::Limit, 50, 3);
    Order a3(PriceType::Ask, OrderType::Limit, 50, 7);
    book.insertOrder(a1);
    book.insertOrder(a2);
    book.insertOrder(a3);

    auto& lim = book.askTree[50];
    assert(lim.size == (int)lim.orders.size());
    assert(lim.size == 3);

    // cancel middle
    book.cancelOrder(a2.id);
    assert(lim.size == (int)lim.orders.size());
    assert(lim.size == 2);

    // partial match eats first order
    Order bid(PriceType::Bid, OrderType::Limit, 50, 5);
    book.insertOrder(bid);
    assert(book.askTree[50].size == (int)book.askTree[50].orders.size());
    std::cout << "TEST 69 PASSED: Limit.size matches orders.size()\n";
}

// ==========================================================
//  SECTION 16 — NEW TESTS: TREE ORDERING
// ==========================================================

// TEST 70: Bid tree is sorted descending (best bid first)
{
    Book book;
    Order b1(PriceType::Bid, OrderType::Limit, 40, 1);
    Order b2(PriceType::Bid, OrderType::Limit, 50, 1);
    Order b3(PriceType::Bid, OrderType::Limit, 45, 1);
    Order b4(PriceType::Bid, OrderType::Limit, 48, 1);
    book.insertOrder(b1);
    book.insertOrder(b2);
    book.insertOrder(b3);
    book.insertOrder(b4);

    int prev = INT_MAX;
    for (auto& [price, lim] : book.bidTree) {
        assert(price < prev);
        prev = price;
    }
    assert(book.bidTree.begin()->first == 50);  // best bid
    std::cout << "TEST 70 PASSED: Bid tree sorted descending\n";
}

// TEST 71: Ask tree is sorted ascending (best ask first)
{
    Book book;
    Order a1(PriceType::Ask, OrderType::Limit, 60, 1);
    Order a2(PriceType::Ask, OrderType::Limit, 50, 1);
    Order a3(PriceType::Ask, OrderType::Limit, 55, 1);
    Order a4(PriceType::Ask, OrderType::Limit, 52, 1);
    book.insertOrder(a1);
    book.insertOrder(a2);
    book.insertOrder(a3);
    book.insertOrder(a4);

    int prev = -1;
    for (auto& [price, lim] : book.askTree) {
        assert(price > prev);
        prev = price;
    }
    assert(book.askTree.begin()->first == 50);  // best ask
    std::cout << "TEST 71 PASSED: Ask tree sorted ascending\n";
}

// ==========================================================
//  SECTION 17 — NEW TESTS: MULTIPLE MODIFIES
// ==========================================================

// TEST 72: Chain of modifies on the same logical order
{
    Book book;
    Order o(PriceType::Bid, OrderType::Limit, 50, 10);
    book.insertOrder(o);
    int id = o.id;

    // modify price up
    book.modifyOrder(id, 55, 10);
    // find new ID
    assert(book.allOrders.size() == 1);
    int id2 = book.allOrders.begin()->first;
    assert(id2 > id);
    assert(book.bidTree.count(50) == 0);
    assert(book.bidTree[55].totalVolume == 10);

    // modify again
    book.modifyOrder(id2, 60, 8);
    assert(book.allOrders.size() == 1);
    int id3 = book.allOrders.begin()->first;
    assert(id3 > id2);
    assert(book.bidTree.count(55) == 0);
    assert(book.bidTree[60].totalVolume == 8);

    // modify again
    book.modifyOrder(id3, 45, 3);
    assert(book.allOrders.size() == 1);
    assert(book.bidTree.count(60) == 0);
    assert(book.bidTree[45].totalVolume == 3);

    std::cout << "TEST 72 PASSED: Chain of modifies\n";
}

// TEST 73: Modify already-cancelled order is no-op
{
    Book book;
    Order o(PriceType::Ask, OrderType::Limit, 50, 5);
    book.insertOrder(o);
    book.cancelOrder(o.id);

    book.modifyOrder(o.id, 55, 10);  // should not crash or insert
    assert(book.askTree.empty());
    assert(book.allOrders.empty());
    std::cout << "TEST 73 PASSED: Modify cancelled order is no-op\n";
}

// ==========================================================
//  SECTION 18 — NEW TESTS: RANDOMIZED STRESS
// ==========================================================

// TEST 74: 10k random operations, verify allOrders == tree count
{
    Book book;
    std::mt19937 rng(12345);
    std::uniform_int_distribution<int> op_dist(0, 3);
    std::uniform_int_distribution<int> price_dist(90, 110);
    std::uniform_int_distribution<int> qty_dist(1, 20);
    std::uniform_int_distribution<int> side_dist(0, 1);

    std::vector<int> live_ids;

    for (int i = 0; i < 10'000; i++) {
        int op = op_dist(rng);
        PriceType side = side_dist(rng) == 0 ? PriceType::Bid : PriceType::Ask;
        int price = price_dist(rng);
        int qty = qty_dist(rng);

        if (op <= 1) {
            // insert limit
            Order o(side, OrderType::Limit, price, qty);
            book.insertOrder(o);
            if (o.quantity > 0) live_ids.push_back(o.id);
        } else if (op == 2 && !live_ids.empty()) {
            // cancel random
            std::uniform_int_distribution<int> idx(0, (int)live_ids.size() - 1);
            int j = idx(rng);
            book.cancelOrder(live_ids[j]);
            live_ids[j] = live_ids.back();
            live_ids.pop_back();
        } else if (op == 3) {
            // market order
            Order o(side, OrderType::Market, 0, qty);
            book.insertOrder(o);
        }
    }

    // invariant: allOrders count matches what's actually in the trees
    assert((int)book.allOrders.size() == count_tree_orders(book));

    // every ID in allOrders should point to a valid order
    for (auto& [id, it] : book.allOrders) {
        assert(it->id == id);
        assert(it->parentLimit != nullptr);
    }

    std::cout << "TEST 74 PASSED: 10k random ops, allOrders consistent\n";
}

// TEST 75: 10k random ops, Limit.size always matches orders.size()
{
    Book book;
    std::mt19937 rng(99999);
    std::uniform_int_distribution<int> op_dist(0, 2);
    std::uniform_int_distribution<int> price_dist(95, 105);
    std::uniform_int_distribution<int> qty_dist(1, 10);

    std::vector<int> live_ids;

    for (int i = 0; i < 10'000; i++) {
        int op = op_dist(rng);

        if (op <= 1) {
            PriceType side = (op == 0) ? PriceType::Bid : PriceType::Ask;
            Order o(side, OrderType::Limit, price_dist(rng), qty_dist(rng));
            book.insertOrder(o);
            if (o.quantity > 0) live_ids.push_back(o.id);
        } else if (!live_ids.empty()) {
            std::uniform_int_distribution<int> idx(0, (int)live_ids.size() - 1);
            int j = idx(rng);
            book.cancelOrder(live_ids[j]);
            live_ids[j] = live_ids.back();
            live_ids.pop_back();
        }
    }

    // check every limit level
    for (auto& [price, lim] : book.bidTree) {
        assert(lim.size == (int)lim.orders.size());
        int vol = 0;
        for (auto& o : lim.orders) vol += o.quantity;
        assert(vol == lim.totalVolume);
    }
    for (auto& [price, lim] : book.askTree) {
        assert(lim.size == (int)lim.orders.size());
        int vol = 0;
        for (auto& o : lim.orders) vol += o.quantity;
        assert(vol == lim.totalVolume);
    }

    std::cout << "TEST 75 PASSED: 10k random ops, Limit fields consistent\n";
}

// ==========================================================
//  SECTION 19 — NEW TESTS: SAME-SIDE NO MATCH
// ==========================================================

// TEST 76: Bids never match against bids
{
    Book book;
    Order b1(PriceType::Bid, OrderType::Limit, 50, 5);
    Order b2(PriceType::Bid, OrderType::Limit, 50, 5);
    Order b3(PriceType::Bid, OrderType::Limit, 55, 5);
    book.insertOrder(b1);
    book.insertOrder(b2);
    book.insertOrder(b3);

    // all three should coexist, no matching
    assert(book.allOrders.size() == 3);
    assert(book.bidTree[50].totalVolume == 10);
    assert(book.bidTree[55].totalVolume == 5);
    assert(book.askTree.empty());
    std::cout << "TEST 76 PASSED: Bids don't match bids\n";
}

// TEST 77: Asks never match against asks
{
    Book book;
    Order a1(PriceType::Ask, OrderType::Limit, 50, 5);
    Order a2(PriceType::Ask, OrderType::Limit, 50, 5);
    Order a3(PriceType::Ask, OrderType::Limit, 45, 5);
    book.insertOrder(a1);
    book.insertOrder(a2);
    book.insertOrder(a3);

    assert(book.allOrders.size() == 3);
    assert(book.askTree[50].totalVolume == 10);
    assert(book.askTree[45].totalVolume == 5);
    assert(book.bidTree.empty());
    std::cout << "TEST 77 PASSED: Asks don't match asks\n";
}

// ==========================================================
//  SECTION 20 — NEW TESTS: MARKET ORDER NEVER RESTS
// ==========================================================

// TEST 78: Unfilled market order does not rest in book
{
    Book book;
    // empty book — market buy has nothing to fill against
    Order mkt(PriceType::Bid, OrderType::Market, 0, 50);
    book.insertOrder(mkt);

    assert(book.bidTree.empty());
    assert(book.askTree.empty());
    assert(book.allOrders.empty());

    // partially filled market also shouldn't rest
    Order ask(PriceType::Ask, OrderType::Limit, 50, 3);
    book.insertOrder(ask);

    Order mkt2(PriceType::Bid, OrderType::Market, 0, 10);
    book.insertOrder(mkt2);
    // ate 3, 7 unfilled — should not rest
    assert(book.bidTree.empty());
    assert(book.allOrders.empty());  // ask was fully eaten too
    std::cout << "TEST 78 PASSED: Unfilled market order never rests\n";
}

// ==========================================================
//  SECTION 21 — NEW TESTS: FOK EXACT BOUNDARY
// ==========================================================

// TEST 79: FOK with exact available volume fills
{
    Book book;
    Order a1(PriceType::Ask, OrderType::Limit, 50, 5);
    Order a2(PriceType::Ask, OrderType::Limit, 51, 5);
    book.insertOrder(a1);
    book.insertOrder(a2);

    // FOK for exactly 10 at max 51 — exactly enough
    Order fok(PriceType::Bid, OrderType::FOK, 51, 10);
    book.insertOrder(fok);
    assert(book.askTree.empty());
    assert(book.bidTree.empty());
    std::cout << "TEST 79 PASSED: FOK exact volume boundary fills\n";
}

// TEST 80: FOK for 1 more than available is rejected
{
    Book book;
    Order a1(PriceType::Ask, OrderType::Limit, 50, 5);
    Order a2(PriceType::Ask, OrderType::Limit, 51, 5);
    book.insertOrder(a1);
    book.insertOrder(a2);

    Order fok(PriceType::Bid, OrderType::FOK, 51, 11);  // need 11, have 10
    book.insertOrder(fok);
    assert(book.askTree[50].totalVolume == 5);  // untouched
    assert(book.askTree[51].totalVolume == 5);  // untouched
    assert(book.bidTree.empty());
    std::cout << "TEST 80 PASSED: FOK boundary+1 rejected\n";
}

// ==========================================================
//  SECTION 22 — NEW TESTS: MATCH PRICE CORRECTNESS
// ==========================================================

// TEST 81: Bid at 55 matching ask at 50 — ask gets eaten at ask's price level
{
    // This verifies the book eats at the resting order's price,
    // not the aggressor's price (price improvement goes to aggressor)
    Book book;
    Order ask(PriceType::Ask, OrderType::Limit, 50, 5);
    book.insertOrder(ask);

    Order bid(PriceType::Bid, OrderType::Limit, 55, 3);
    book.insertOrder(bid);

    // ask at 50 should be partially filled (5 -> 2)
    assert(book.askTree[50].totalVolume == 2);
    // bid fully filled, not resting at 55
    assert(book.bidTree.empty());
    std::cout << "TEST 81 PASSED: Match happens at resting price level\n";
}

// ==========================================================
//  SECTION 23 — NEW TESTS: BOOK DEPTH SCALING
// ==========================================================

// TEST 82: 1000 price levels, verify structure
{
    Book book;
    for (int p = 1; p <= 500; p++) {
        Order bid(PriceType::Bid, OrderType::Limit, p, 1);
        book.insertOrder(bid);
    }
    for (int p = 501; p <= 1000; p++) {
        Order ask(PriceType::Ask, OrderType::Limit, p, 1);
        book.insertOrder(ask);
    }

    assert((int)book.bidTree.size() == 500);
    assert((int)book.askTree.size() == 500);
    assert(book.bidTree.begin()->first == 500);   // best bid
    assert(book.askTree.begin()->first == 501);   // best ask
    assert((int)book.allOrders.size() == 1000);

    book.trackLimits();
    assert(book.tracking.highestBid == 500);
    assert(book.tracking.lowestAsk == 501);
    assert(book.tracking.spread == 1);

    // aggressive bid eats through 10 ask levels
    Order bid(PriceType::Bid, OrderType::Limit, 510, 10);
    book.insertOrder(bid);
    assert(book.askTree.begin()->first == 511);
    assert((int)book.askTree.size() == 490);

    std::cout << "TEST 82 PASSED: 1000 price levels\n";
}

// ==========================================================
//  DONE
// ==========================================================

    std::cout << "\n============================================\n";
    std::cout << "ALL TESTS PASSED\n";
    std::cout << "============================================\n";
    return 0;
}
