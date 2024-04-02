project(translate++ C CXX)
cmake_minimum_required(VERSION 3.3.2)

include(cmake/App.cmake)

set(SOURCES main.cpp)

add_app("${SOURCES}")
