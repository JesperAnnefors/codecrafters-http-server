cmake_minimum_required(VERSION 3.13)
project(http-server-starter-cpp)
find_package(ZLIB REQUIRED)
set(CMAKE_CXX_STANDARD 20) # Enable the C++20 standard

file(GLOB_RECURSE SOURCE_FILES src/*.cpp)

add_executable(server ${SOURCE_FILES})
target_link_libraries(server pthread ZLIB::ZLIB)