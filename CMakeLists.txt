cmake_minimum_required(VERSION 3.10)
link_libraries( -lcrypto)
add_executable(searchHash searchHash.cpp)
add_executable(searchHashBinarySearch searchHashBinarySearch.cpp)