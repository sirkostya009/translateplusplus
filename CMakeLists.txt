project(translate++ C CXX)
cmake_minimum_required(VERSION 3.3.2)

include(cmake/App.cmake)

set(SOURCES src/main.cpp
            src/App.hpp
            src/Info.hpp
            src/Editor.hpp
            src/Translator.hpp
            src/Translator.cpp
            src/json.hpp)

add_app("${SOURCES}")
