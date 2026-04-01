

cmake_minimum_required(VERSION 3.15)
project(redis_clone)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Compile commands for editors/LSPs
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(redis_clone
    src/main.cpp
    src/server.cpp
    src/resp_parser.cpp
)

target_include_directories(redis_clone PRIVATE include)

# Link pthreads for std::thread
find_package(Threads REQUIRED)
target_link_libraries(redis_clone PRIVATE Threads::Threads)



