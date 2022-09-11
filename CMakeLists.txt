cmake_minimum_required(VERSION 3.4...3.18)

project(cpposu)
set(CMAKE_CXX_STANDARD 20)

add_library(cpposu INTERFACE)
target_sources(cpposu INTERFACE
    cpposu/line_parser.hpp
    )
target_include_directories(cpposu INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(tests)
