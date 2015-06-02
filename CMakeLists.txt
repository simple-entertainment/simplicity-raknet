cmake_minimum_required(VERSION 3.2)

include(../simplicity/simplicity-common.cmake)

# Project
project(simplicity-raknet)

# Paths
file(GLOB_RECURSE SRC_FILES src/main/c++/*.cpp src/main/c++/*.h src/raknet/c++/*.cpp src/raknet/c++/*.h)
include_directories(../simplicity/src/main/c++)
include_directories(src/main/c++)
include_directories(src/raknet/c++)

# Output
add_library(simplicity-raknet STATIC ${SRC_FILES})
