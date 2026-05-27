# Orderbook
Basic orderbook project to learn networking, dsa and low latency practices.

```bash 
git clone https://github.com/president9/orderbook.git
cd orderbook
conan install . --build=missing
cmake -S . -B build
cd build 
make
```

If using conan:
```bash
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=build/Release/generators/conan_toolchain.cmake -DCMAKE_PREFIX_PATH=build/Release/generators -DCMAKE_BUILD_TYPE=Release
```


Executables will be in the build folder. Run benchmark using:
```bash
./benchmark
```
in the build folder.

# TODO
- Serialisation tests
- ~~Benchmarker~~
- Refactor to use cache efficient data structure
- Trade/execution log
- Add networked orders
- More

# REQUIREMENTS
- cmake
- Conan 
- Boost
  
