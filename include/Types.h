#pragma once
#include <stdint.h>

enum class PriceType : uint8_t {Bid = 1, Ask = 2};
enum class OrderType : uint8_t { Limit = 1, Market = 2, IOC = 3, FOK = 4};
