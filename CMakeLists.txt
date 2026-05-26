cmake_minimum_required(VERSION 3.23)
cmake_policy(SET CMP0167 NEW)
project(orderbook)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Boost QUIET)

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
target_include_directories(orderbook_core PUBLIC include)
target_link_libraries(orderbook_core PUBLIC Boost::boost)
target_compile_options(orderbook_core PRIVATE -Wall -Wextra -Werror)
target_compile_features(orderbook_core PRIVATE cxx_std_23)

if(Boost_FOUND)
    target_link_libraries(orderbook_core PUBLIC Boost::boost)
    target_compile_definitions(orderbook_core PUBLIC HAS_BOOST)
endif()

add_executable(orderbook src/main.cpp)
target_link_libraries(orderbook PRIVATE orderbook_core)

add_executable(tests tests/tests.cpp)
target_link_libraries(tests PRIVATE orderbook_core)

add_executable(boostTest tests/boostTests.cpp)
target_link_libraries(boostTest PRIVATE orderbook_core)
