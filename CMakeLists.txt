cmake_minimum_required(VERSION 3.10)
project(CShell)

set(CMAKE_CXX_STANDARD 14)

add_executable(coffee_shell main.cpp)

# Link against the stdc++fs library
target_link_libraries(coffee_shell stdc++fs readline)