# Orderbook
A limit order book supporting limit, market, IOC, and FOK orders with price-time priority matching. 

# REQUIREMENTS
- CMake 3.23+
- Conan2 
- Boost
  
# HOW TO USE
To get started:
```bash 
git clone https://github.com/president9/orderbook.git
cd orderbook
conan install . --build=missing -s build_type=Debug
cmake --preset conan-debug
cmake --build build/Debug
cd build 
```

Then run what program you want:
```bash
- /messagesTest
- /benchmark
- /orderbook
- /tests
```

Executables will be in the build folder. Run benchmark using:
```bash
./benchmark
```
in the build folder.

# TODO
- Add networked upload of order
- ~~Benchmarker~~ Benchmark with network Latency
- ~~Serialisation tests~~ Orderbook tests using serialisation + network
- Refactor to use cache efficient data structure
- Trade/execution log
- More

