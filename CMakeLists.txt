cmake_minimum_required(VERSION 3.10)
link_libraries( -lcrypto)
add_executable(searchHash searchHash.cpp searchHelpers.cpp searchHelpers.h printHelpers.cpp printHelpers.h)
add_executable(searchHashBinarySearch searchHashBinarySearch.cpp searchHelpers.cpp searchHelpers.h printHelpers.cpp printHelpers.h)