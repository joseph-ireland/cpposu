cmake_minimum_required(VERSION 3.4...3.18)

project(cpposu)
set(CMAKE_CXX_STANDARD 20)

add_library(cpposu INTERFACE)
target_sources(cpposu INTERFACE
    cpposu/beatmap_parser.hpp
    cpposu/line_parser.hpp
    cpposu/path.hpp
    cpposu/slider.hpp
    cpposu/types.hpp
    )

target_include_directories(cpposu INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

add_library(cpposu_lib SHARED cpposu/cpposu_dll.cpp)
target_link_libraries(cpposu_lib PRIVATE cpposu)

add_subdirectory(tests)
