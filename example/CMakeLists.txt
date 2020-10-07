cmake_minimum_required(VERSION 3.5)
project(mango-examples)

set(CMAKE_CXX_FLAGS "-Wall")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

link_libraries(mango)

if (APPLE)
else ()
    link_libraries(dl)
endif ()

add_executable(compress misc/compress.cpp)
