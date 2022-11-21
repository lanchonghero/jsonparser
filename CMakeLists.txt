cmake_minimum_required(VERSION 3.0.0)

project(json_parser)

aux_source_directory(. DIR_SRCS)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
include_directories(./)

add_executable(example ${DIR_SRCS})
