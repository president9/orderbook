cmake_minimum_required(VERSION 3.23)
project(orderbook)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
file(GLOB_RECURSE SOURCES src/*.cpp)
file(GLOB_RECURSE HEADERS include/*.h)
list(FILTER SOURCES EXCLUDE REGEX "main\\.cpp$")

add_library(orderbook_core)
target_sources(orderbook_core
    PRIVATE
        ${SOURCES}
    PUBLIC
        FILE_SET myHeaders
        TYPE HEADERS
        BASE_DIRS include 
        FILES 
            ${HEADERS}
)

target_include_directories(orderbook_core PRIVATE include)

add_executable(orderbook src/main.cpp)
target_link_libraries(orderbook PRIVATE orderbook_core)
target_compile_options(orderbook PRIVATE -Wall -Wextra -Werror)
target_compile_features(orderbook PRIVATE cxx_std_23)

add_executable(tests tests/tests.cpp)
target_link_libraries(tests PRIVATE orderbook_core)
target_compile_options(orderbook PRIVATE -Wall -Wextra -Werror)
target_compile_features(orderbook PRIVATE cxx_std_23)


