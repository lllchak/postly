cmake_minimum_required(VERSION 3.9)

project(postly)

add_subdirectory(driver)
add_subdirectory(rss)

include_directories(${CMAKE_SOURCE_DIR}/driver)
include_directories(${CMAKE_SOURCE_DIR}/rss)
