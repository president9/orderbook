# Orderbook
```bash 
git clone https://github.com/president9/orderbook.git
cd orderbook
cmake -S . -B build
cd build 
make
```
Executables will be in the build folder. Run benchmark using:
```bash
./benchmark
```
in the build folder.

# TODO
- ~~Benchmarker~~
- Refactor to use cache efficient data structure
- Trade/execution log
- Add networked orders
- More

# REQUIREMENTS
- cmake
- Conan (optional, required if you want the boost features to work)
  
