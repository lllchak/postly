cmake_minimum_required(VERSION 3.9)

project(rss)

set(CMAKE_MACOSX_RPATH 1)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
set(POSTLY_CXX_FLAGS "-pthread;-Wall;-Wextra;-Wno-sign-compare;-fno-omit-frame-pointer;")
set(POSTLY_CXX_DEBUG_FLAGS "-fsanitize=address;")
set(POSTLY_CXX_RELEASE_FLAGS "-funroll-loops;-mtune=generic;")

add_executable(${PROJECT_NAME} main.cpp)
