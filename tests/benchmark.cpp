#include <chrono>
#include <iostream>
#include <vector>
#include <algorithm>
#include <numeric>
#include <random>
#include <iomanip>
#include <cstring>
#include <fstream>
#include <cmath>
#include <functional>
#include <cstdint>
#include <thread>
#include <unistd.h>
#include "Book.h"


// -------------------------------------------------
// Cycle counter — rdtsc on x86, cntvct_el0 on ARM
// -------------------------------------------------
//
// x86: rdtsc ticks at the CPU's invariant TSC frequency (usually
//      close to the base clock). lfence before rdtsc and rdtscp
//      after give start/end serialisation.
//
// ARM: cntvct_el0 is the generic timer counter. It does NOT tick
//      at core frequency — it runs at a fixed rate (CNTFRQ_EL0,
//      often 24 MHz on many SoCs). This gives ~42 ns resolution,
//      which is coarser than x86's sub-ns TSC. For sub-100ns
//      operations on ARM you may need to batch iterations and
//      amortise. The calibration step handles the freq difference.
//

#if defined(__x86_64__) || defined(_M_X64)

  static inline uint64_t tsc_start() {
      uint64_t lo, hi;
      asm volatile(
          "lfence\n\t"
          "rdtsc"
          : "=a"(lo), "=d"(hi)
          :
          : "memory"
      );
      return (hi << 32) | lo;
  }

  static inline uint64_t tsc_end() {
      uint64_t lo, hi;
      unsigned aux;
      asm volatile(
          "rdtscp"
          : "=a"(lo), "=d"(hi), "=c"(aux)
          :
          : "memory"
      );
      asm volatile("lfence" ::: "memory");
      return (hi << 32) | lo;
  }

  #define ARCH_NAME "x86_64 (rdtsc)"

#elif defined(__aarch64__) || defined(_M_ARM64)

  static inline uint64_t tsc_start() {
      uint64_t val;
      asm volatile("isb" ::: "memory");
      asm volatile("mrs %0, cntvct_el0" : "=r"(val));
      return val;
  }

  static inline uint64_t tsc_end() {
      uint64_t val;
      asm volatile("mrs %0, cntvct_el0" : "=r"(val));
      asm volatile("isb" ::: "memory");
      return val;
  }

  // Read the ARM generic timer frequency register (usually accessible from EL0).
  // Returns 0 if not available, in which case we fall back to calibration.
  static inline uint64_t arm_cntfrq() {
      uint64_t freq;
      asm volatile("mrs %0, cntfrq_el0" : "=r"(freq));
      return freq;
  }

  #define ARCH_NAME "ARM64 (cntvct_el0)"

#else
  #error "Unsupported architecture — need x86_64 or aarch64"
#endif

// -------------------------------------------------
// Calibration: ticks -> nanoseconds
// -------------------------------------------------

static double g_ticks_per_ns = 0.0;
static double g_ns_per_tick  = 0.0;

static void calibrate_tsc() {
    using namespace std::chrono;

#if defined(__aarch64__) || defined(_M_ARM64)
    // Try reading CNTFRQ_EL0 first — it's exact and avoids sleep jitter
    uint64_t freq = arm_cntfrq();
    if (freq > 0) {
        g_ticks_per_ns = (double)freq / 1e9;
        g_ns_per_tick  = 1e9 / (double)freq;
        return;
    }
#endif

    // Fallback: wall-clock calibration (works on x86 and ARM)
    constexpr int CAL_MS = 50;
    constexpr int ROUNDS = 5;
    double best_ratio = 0.0;

    for (int r = 0; r < ROUNDS; r++) {
        auto wall_start = steady_clock::now();
        uint64_t t0 = tsc_start();

        std::this_thread::sleep_for(milliseconds(CAL_MS));

        uint64_t t1 = tsc_end();
        auto wall_end = steady_clock::now();

        double wall_ns  = (double)duration_cast<nanoseconds>(wall_end - wall_start).count();
        double tsc_diff = (double)(t1 - t0);

        double ratio = tsc_diff / wall_ns;
        if (ratio > best_ratio) best_ratio = ratio; // least sleep overshoot
    }

    g_ticks_per_ns = best_ratio;
    g_ns_per_tick  = 1.0 / best_ratio;
}

static inline double ticks_to_ns(double ticks) {
    return ticks * g_ns_per_tick;
}

// -------------------------------------------------
// Stats (in ticks, converted to ns for display)
// -------------------------------------------------

struct Stats {
    double mean;
    double min;
    double max;
    double p50;
    double p95;
    double p99;
    double p999;
    double stdev;
    double total;
    int    count;
};

Stats compute_stats(std::vector<uint64_t>& v) {
    std::sort(v.begin(), v.end());
    Stats s;
    s.count = (int)v.size();
    s.min   = (double)v.front();
    s.max   = (double)v.back();
    double sum = 0;
    for (auto x : v) sum += (double)x;
    s.total = sum;
    s.mean  = sum / (double)v.size();
    s.p50   = (double)v[(size_t)(v.size() * 0.50)];
    s.p95   = (double)v[(size_t)(v.size() * 0.95)];
    s.p99   = (double)v[(size_t)(v.size() * 0.99)];
    s.p999  = (double)v[(size_t)(v.size() * 0.999)];

    double sq_sum = 0;
    for (auto x : v) {
        double d = (double)x - s.mean;
        sq_sum += d * d;
    }
    s.stdev = std::sqrt(sq_sum / (double)v.size());
    return s;
}

void print_stats(const char* label, Stats& s) {
    double total_ns = ticks_to_ns(s.total);
    double ops_sec  = (total_ns > 0)
        ? (double)s.count / (total_ns * 1e-9) : 0.0;

    auto fmt = [](double ticks) -> std::string {
        char buf[128];
        std::snprintf(buf, sizeof(buf), "%8.1f ticks  (%6.0f ns)",
                      ticks, ticks_to_ns(ticks));
        return buf;
    };

    std::cout << label << ":\n";
    std::cout << "  mean:  " << fmt(s.mean)  << "\n";
    std::cout << "  p50:   " << fmt(s.p50)   << "\n";
    std::cout << "  p95:   " << fmt(s.p95)   << "\n";
    std::cout << "  p99:   " << fmt(s.p99)   << "\n";
    std::cout << "  p99.9: " << fmt(s.p999)  << "\n";
    std::cout << "  stdev: " << fmt(s.stdev) << "\n";
    std::cout << "  min:   " << std::fixed << std::setprecision(1)
              << s.min << "  max: " << s.max << " ticks\n";
    std::cout << "  ops/s: " << std::setprecision(0) << ops_sec << "\n";
}

#if defined(__APPLE__)
#include <mach/mach.h>
static size_t get_rss_bytes() {
    mach_task_basic_info_data_t info;
    mach_msg_type_number_t count = MACH_TASK_BASIC_INFO_COUNT;
    if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO,
                  (task_info_t)&info, &count) != KERN_SUCCESS)
        return 0;
    return info.resident_size;
}
#elif defined(__linux__)
static size_t get_rss_bytes() {
    std::ifstream f("/proc/self/statm");
    if (!f.is_open()) return 0;
    size_t sz, rss;
    f >> sz >> rss;
    long page_sz = sysconf(_SC_PAGESIZE);
    return rss * (page_sz > 0 ? (size_t)page_sz : 4096);
}
#else
static size_t get_rss_bytes() { return 0; }
#endif

// -------------------------------------------------
// Core bench loop
// -------------------------------------------------
template<typename F>
std::vector<uint64_t> bench(F&& fn, int iterations) {
    std::vector<uint64_t> samples;
    samples.reserve(iterations);
    for (int i = 0; i < iterations; i++) {
        uint64_t t0 = tsc_start();
        fn(i);
        uint64_t t1 = tsc_end();
        samples.push_back(t1 - t0);
    }
    return samples;
}

template<typename F>
void warmup(F&& fn, int iterations) {
    for (int i = 0; i < iterations; i++) fn(i);
}

// -------------------------------------------------
// Portable do_not_optimize
// -------------------------------------------------
#if defined(_MSC_VER)
  #pragma optimize("", off)
  template<typename T>
  void do_not_optimize(T&& val) {
      static volatile char sink;
      sink = static_cast<char>(reinterpret_cast<uintptr_t>(&val) & 0xFF);
  }
  #pragma optimize("", on)
#elif defined(__GNUC__) || defined(__clang__)
  template<typename T>
  inline void do_not_optimize(T const& val) {
      asm volatile("" : : "g"(val) : "memory");
  }
#else
  template<typename T>
  void do_not_optimize(T&& val) {
      static volatile T sink;
      sink = val;
  }
#endif

#if defined(__GNUC__) || defined(__clang__)
  #define BENCH_NOINLINE __attribute__((noinline))
#elif defined(_MSC_VER)
  #define BENCH_NOINLINE __declspec(noinline)
#else
  #define BENCH_NOINLINE
#endif

constexpr int ITERS  = 500'000;
constexpr int WARMUP = 10'000;

// ============================================================
// 1. INSERT — passive
// ============================================================
BENCH_NOINLINE
void bench_insert_passive() {
    Book book;

    // Pre-fill with 2M orders across 1000 price levels to stress the tree
    for (int i = 0; i < 2'000'000; i++) {
        Order o(PriceType::Bid, OrderType::Limit, 1000 + (i % 1000), 10);
        book.insertOrder(o);
    }

    warmup([&](int i) {
        Order o(PriceType::Bid, OrderType::Limit, 1000 + (i % 1000), 10);
        book.insertOrder(o);
    }, WARMUP);

    auto samples = bench([&](int i) {
        Order o(PriceType::Bid, OrderType::Limit, 1000 + (i % 1000), 10);
        book.insertOrder(o);
        do_not_optimize(book.allOrders.size());
    }, ITERS);

    auto s = compute_stats(samples);
    print_stats("1. Insert (passive, 2M+ deep book, 1000 levels)", s);
    std::cout << "   Book depth: " << book.allOrders.size() << " orders\n\n";
}

// ============================================================
// 2. INSERT — aggressive (1-level match)
// ============================================================
BENCH_NOINLINE
void bench_insert_aggressive_1level() {
    Book book;

    // Pre-fill 500k orders on each side across wide range so tree is deep
    for (int i = 0; i < 500'000; i++) {
        Order bid(PriceType::Bid, OrderType::Limit, 50 + (i % 500), 10);
        book.insertOrder(bid);
        Order ask(PriceType::Ask, OrderType::Limit, 600 + (i % 500), 10);
        book.insertOrder(ask);
    }

    warmup([&](int) {
        Order ask(PriceType::Ask, OrderType::Limit, 550, 10);
        book.insertOrder(ask);
        Order bid(PriceType::Bid, OrderType::Limit, 550, 10);
        book.insertOrder(bid);
    }, WARMUP);

    std::vector<uint64_t> samples;
    samples.reserve(ITERS);
    for (int i = 0; i < ITERS; i++) {
        Order ask(PriceType::Ask, OrderType::Limit, 550, 10);
        book.insertOrder(ask);

        uint64_t t0 = tsc_start();
        Order bid(PriceType::Bid, OrderType::Limit, 550, 10);
        book.insertOrder(bid);
        uint64_t t1 = tsc_end();

        do_not_optimize(book.allOrders.size());
        samples.push_back(t1 - t0);
    }

    auto s = compute_stats(samples);
    print_stats("2. Insert (aggressive, 1-level match, 1M background)", s);
    std::cout << "\n";
}

// ============================================================
// 3. INSERT — aggressive (5-level sweep)
// ============================================================
BENCH_NOINLINE
void bench_insert_aggressive_5level() {
    Book book;

    // Pre-fill deep background book
    for (int i = 0; i < 500'000; i++) {
        Order bid(PriceType::Bid, OrderType::Limit, 50 + (i % 500), 10);
        book.insertOrder(bid);
        Order ask(PriceType::Ask, OrderType::Limit, 600 + (i % 500), 10);
        book.insertOrder(ask);
    }

    warmup([&](int) {
        for (int p = 550; p <= 554; p++) {
            Order ask(PriceType::Ask, OrderType::Limit, p, 2);
            book.insertOrder(ask);
        }
        Order bid(PriceType::Bid, OrderType::Limit, 554, 10);
        book.insertOrder(bid);
    }, WARMUP);

    std::vector<uint64_t> samples;
    samples.reserve(ITERS);
    for (int i = 0; i < ITERS; i++) {
        for (int p = 550; p <= 554; p++) {
            Order ask(PriceType::Ask, OrderType::Limit, p, 2);
            book.insertOrder(ask);
        }

        uint64_t t0 = tsc_start();
        Order bid(PriceType::Bid, OrderType::Limit, 554, 10);
        book.insertOrder(bid);
        uint64_t t1 = tsc_end();

        do_not_optimize(book.allOrders.size());
        samples.push_back(t1 - t0);
    }

    auto s = compute_stats(samples);
    print_stats("3. Insert (aggressive, 5-level sweep, 1M background)", s);
    std::cout << "\n";
}

// ============================================================
// 4. CANCEL — random, constant-size book
// ============================================================
BENCH_NOINLINE
void bench_cancel(std::mt19937& rng) {
    Book book;

    constexpr int BOOK_SIZE = 1'000'000;
    std::vector<int> live_ids;
    live_ids.reserve(BOOK_SIZE);

    for (int i = 0; i < BOOK_SIZE; i++) {
        PriceType side = (i % 2 == 0) ? PriceType::Bid : PriceType::Ask;
        int price = (side == PriceType::Bid) ? 100 + (i % 500) : 700 + (i % 500);
        Order o(side, OrderType::Limit, price, 10);
        book.insertOrder(o);
        live_ids.push_back(o.id);
    }

    std::shuffle(live_ids.begin(), live_ids.end(), rng);

    std::uniform_int_distribution<int> bid_price_dist(100, 599);
    std::uniform_int_distribution<int> ask_price_dist(700, 1199);
    int total_needed = ITERS + WARMUP;

    // Pre-generate reinsert orders with alternating sides
    struct ReinsertInfo { PriceType side; int price; };
    std::vector<ReinsertInfo> reinserts(total_needed);
    for (int i = 0; i < total_needed; i++) {
        if (i % 2 == 0) {
            reinserts[i] = { PriceType::Bid, bid_price_dist(rng) };
        } else {
            reinserts[i] = { PriceType::Ask, ask_price_dist(rng) };
        }
    }

    int cancel_idx = 0;
    int total = std::min(total_needed, (int)live_ids.size());

    for (int i = 0; i < std::min(WARMUP, total); i++) {
        book.cancelOrder(live_ids[cancel_idx]);
        Order o(reinserts[i].side, OrderType::Limit, reinserts[i].price, 10);
        book.insertOrder(o);
        live_ids[cancel_idx] = o.id;
        cancel_idx++;
    }

    int bench_iters = total - std::min(WARMUP, total);
    auto samples = bench([&](int i) {
        if (cancel_idx < (int)live_ids.size()) {
            book.cancelOrder(live_ids[cancel_idx]);
            do_not_optimize(book.allOrders.size());
            auto& ri = reinserts[cancel_idx % total_needed];
            Order o(ri.side, OrderType::Limit, ri.price, 10);
            book.insertOrder(o);
            live_ids[cancel_idx] = o.id;
            cancel_idx++;
        }
    }, bench_iters);

    auto s = compute_stats(samples);
    print_stats("4. Cancel (random, deep book ~1M, 1000 levels)", s);
    std::cout << "   Book size: " << book.allOrders.size() << " orders\n\n";
}

// ============================================================
// 5. MODIFY
// ============================================================
BENCH_NOINLINE
void bench_modify(std::mt19937& rng) {
    Book book;

    std::vector<int> live_ids;
    for (int i = 0; i < 500'000; i++) {
        Order o(PriceType::Bid, OrderType::Limit, 100 + (i % 100), 10);
        book.insertOrder(o);
        live_ids.push_back(o.id);
    }

    std::shuffle(live_ids.begin(), live_ids.end(), rng);

    std::uniform_int_distribution<int> price_dist(80, 120);
    std::vector<int> prices(live_ids.size());
    for (auto& p : prices) p = price_dist(rng);

    int mod_count = std::min(ITERS, (int)live_ids.size());

    int wi = 0;
    for (; wi < std::min(WARMUP, mod_count); wi++)
        book.modifyOrder(live_ids[wi], prices[wi], 10);

    auto samples = bench([&](int i) {
        int idx = wi + i;
        if (idx < mod_count) {
            book.modifyOrder(live_ids[idx], prices[idx], 10);
            do_not_optimize(book.allOrders.size());
        }
    }, mod_count - wi);

    auto s = compute_stats(samples);
    print_stats("5. Modify (price change)", s);
    std::cout << "\n";
}

// ============================================================
// 6. MIXED WORKLOAD
// ============================================================
struct MixedOp {
    uint8_t op;
    int     price;
    int     qty;
};

BENCH_NOINLINE
void bench_mixed(std::mt19937& rng) {
    Book book;

    std::vector<int> live_ids;
    live_ids.reserve(ITERS);
    for (int i = 0; i < 100'000; i++) {
        PriceType side = (i % 2 == 0) ? PriceType::Bid : PriceType::Ask;
        int price = (side == PriceType::Bid) ? 500 + (i % 500) : 1001 + (i % 500);
        Order o(side, OrderType::Limit, price, 5);
        book.insertOrder(o);
        live_ids.push_back(o.id);
    }

    std::uniform_int_distribution<int> op_dist(0, 99);
    std::uniform_int_distribution<int> bid_price(500, 999);
    std::uniform_int_distribution<int> ask_price(1001, 1500);
    std::uniform_int_distribution<int> agg_price(980, 1020);
    std::uniform_int_distribution<int> qty_dist(1, 20);

    int total = ITERS + WARMUP;
    std::vector<MixedOp> ops(total);
    for (auto& m : ops) {
        m.op  = (uint8_t)op_dist(rng);
        m.qty = qty_dist(rng);
        if (m.op < 35)       m.price = bid_price(rng);
        else if (m.op < 70)  m.price = ask_price(rng);
        else if (m.op < 85)  m.price = agg_price(rng);
        else                 m.price = 0;
    }

    auto run_op = [&](int i) {
        auto& m = ops[i];
        if (m.op < 35) {
            Order o(PriceType::Bid, OrderType::Limit, m.price, m.qty);
            book.insertOrder(o);
            live_ids.push_back(o.id);
        } else if (m.op < 70) {
            Order o(PriceType::Ask, OrderType::Limit, m.price, m.qty);
            book.insertOrder(o);
            live_ids.push_back(o.id);
        } else if (m.op < 85) {
            Order o(PriceType::Bid, OrderType::Limit, m.price, m.qty);
            book.insertOrder(o);
            if (o.quantity > 0) live_ids.push_back(o.id);
        } else {
            if (!live_ids.empty()) {
                int idx = (int)(rng() % live_ids.size());
                book.cancelOrder(live_ids[idx]);
                live_ids[idx] = live_ids.back();
                live_ids.pop_back();
            }
        }
        do_not_optimize(book.allOrders.size());
    };

    for (int i = 0; i < WARMUP; i++) run_op(i);

    auto samples = bench([&](int i) {
        run_op(WARMUP + i);
    }, ITERS);

    auto s = compute_stats(samples);
    print_stats("6. Mixed workload (70/15/15)", s);
    std::cout << "   Final book depth: " << book.allOrders.size() << " orders\n\n";
}

// ============================================================
// 7. MARKET ORDER
// ============================================================
BENCH_NOINLINE
void bench_market_order() {
    constexpr int MARKET_ITERS = 1'000;
    constexpr int LEVELS       = 10;
    constexpr int PER_LEVEL    = 100;
    constexpr int TOTAL_FILLS  = LEVELS * PER_LEVEL;
    constexpr int TOTAL_QTY    = TOTAL_FILLS * 10;

    for (int w = 0; w < 50; w++) {
        Book book;
        for (int p = 100; p < 100 + LEVELS; p++)
            for (int j = 0; j < PER_LEVEL; j++) {
                Order o(PriceType::Ask, OrderType::Limit, p, 10);
                book.insertOrder(o);
            }
        Order mkt(PriceType::Bid, OrderType::Market, 0, TOTAL_QTY);
        book.insertOrder(mkt);
    }

    std::vector<uint64_t> samples;
    samples.reserve(MARKET_ITERS);

    for (int i = 0; i < MARKET_ITERS; i++) {
        Book book;
        for (int p = 100; p < 100 + LEVELS; p++)
            for (int j = 0; j < PER_LEVEL; j++) {
                Order o(PriceType::Ask, OrderType::Limit, p, 10);
                book.insertOrder(o);
            }

        uint64_t t0 = tsc_start();
        Order mkt(PriceType::Bid, OrderType::Market, 0, TOTAL_QTY);
        book.insertOrder(mkt);
        uint64_t t1 = tsc_end();

        do_not_optimize(book.allOrders.size());
        samples.push_back(t1 - t0);
    }

    auto s = compute_stats(samples);
    print_stats("7. Market order (eat 1000 orders across 10 levels)", s);
    std::cout << "   per fill: " << std::fixed << std::setprecision(1)
              << s.mean / (double)TOTAL_FILLS << " ticks  ("
              << std::setprecision(0)
              << ticks_to_ns(s.mean) / (double)TOTAL_FILLS << " ns)\n\n";
}

// ============================================================
// 8. FOK reject
// ============================================================
BENCH_NOINLINE
void bench_fok_reject() {
    Book book;

    // 1000 levels x 100 orders = 100k orders
    for (int p = 100; p < 1100; p++)
        for (int j = 0; j < 100; j++) {
            Order o(PriceType::Ask, OrderType::Limit, p, 10);
            book.insertOrder(o);
        }

    // Vary FOK price each iteration so branch predictor can't learn pattern
    warmup([&](int i) {
        int price = 200 + (i % 900);
        Order fok(PriceType::Bid, OrderType::FOK, price, 9999999);
        book.insertOrder(fok);
    }, WARMUP);

    auto samples = bench([&](int i) {
        int price = 200 + (i % 900);
        Order fok(PriceType::Bid, OrderType::FOK, price, 9999999);
        book.insertOrder(fok);
        do_not_optimize(book.allOrders.size());
    }, ITERS);

    auto s = compute_stats(samples);
    print_stats("8. FOK reject (volume scan, 1000 levels, 100k orders)", s);
    std::cout << "\n";
}

// ============================================================
// 9. COLD BOOK — allocation-heavy, no warm cache
// ============================================================
BENCH_NOINLINE
void bench_cold_book() {
    constexpr int COLD_ITERS = 100;
    constexpr int ORDERS_PER = 500'000;

    // Warmup
    for (int w = 0; w < 5; w++) {
        Book book;
        for (int i = 0; i < ORDERS_PER; i++) {
            PriceType side = (i % 2 == 0) ? PriceType::Bid : PriceType::Ask;
            int price = (side == PriceType::Bid) ? 100 + (i % 500) : 700 + (i % 500);
            Order o(side, OrderType::Limit, price, 10);
            book.insertOrder(o);
        }
    }

    std::vector<uint64_t> samples;
    samples.reserve(COLD_ITERS);

    for (int i = 0; i < COLD_ITERS; i++) {
        Book book;

        uint64_t t0 = tsc_start();
        for (int j = 0; j < ORDERS_PER; j++) {
            PriceType side = (j % 2 == 0) ? PriceType::Bid : PriceType::Ask;
            int price = (side == PriceType::Bid) ? 100 + (j % 500) : 700 + (j % 500);
            Order o(side, OrderType::Limit, price, 10);
            book.insertOrder(o);
        }
        uint64_t t1 = tsc_end();

        do_not_optimize(book.allOrders.size());
        samples.push_back(t1 - t0);
    }

    auto s = compute_stats(samples);
    print_stats("9. Cold book (500k inserts from empty, 1000 levels)", s);
    std::cout << "   per insert: " << std::fixed << std::setprecision(1)
              << s.mean / (double)ORDERS_PER << " ticks  ("
              << std::setprecision(0)
              << ticks_to_ns(s.mean) / (double)ORDERS_PER << " ns)\n\n";
}

// -------------------------------------------------
// MAIN
// -------------------------------------------------
int main() {
    calibrate_tsc();

    std::mt19937 rng(42);
    #if defined(__aarch64__) || defined(_M_ARM64)
    std::cout << "NOTE: ARM timer resolution is ~"
              << std::setprecision(0) << g_ns_per_tick
              << " ns. Sub-" << std::setprecision(0) << g_ns_per_tick
              << " ns measurements are quantised. Any operation that takes less than 1ns can't be measured and will show as 0 ticks.\n";
#endif

    std::cout << "============================================\n";
    std::cout << "     ORDER BOOK BENCHMARK\n";
    std::cout << "============================================\n";
    std::cout << "Timer:      " << ARCH_NAME << "\n";
    std::cout << "Counter freq: " << std::fixed << std::setprecision(3)
              << g_ticks_per_ns * 1e3 << " MHz  ("
              << std::setprecision(4) << g_ticks_per_ns << " ticks/ns)\n";
    std::cout << "Resolution: " << std::setprecision(2)
              << g_ns_per_tick << " ns/tick\n";
    std::cout << "Iterations: " << ITERS
              << "  warmup: " << WARMUP << "\n";
    std::cout << "\n";

    size_t rss_start = get_rss_bytes();

    bench_insert_passive();
    bench_insert_aggressive_1level();
    bench_insert_aggressive_5level();
    bench_cancel(rng);
    bench_modify(rng);
    bench_mixed(rng);
    bench_market_order();
    bench_fok_reject();
    bench_cold_book();

    size_t rss_end = get_rss_bytes();
    std::cout << "============================================\n";
    std::cout << "Memory:\n";
    if (rss_start > 0) {
        std::cout << "  RSS start: " << rss_start / 1024 << " KB\n";
        std::cout << "  RSS end:   " << rss_end / 1024 << " KB\n";
        std::cout << "  RSS delta: " << (rss_end - rss_start) / 1024 << " KB\n";
    } else {
        std::cout << "  (RSS tracking not available on this platform)\n";
    }
    std::cout << "============================================\n";

    return 0;
}
