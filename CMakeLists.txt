cmake_minimum_required(VERSION 3.2)

include(../simplicity/simplicity-common.cmake)

# Project
#########################
project(simplicity-raknet)

# Project Dependencies
#########################

# RakNet
add_subdirectory(lib/RakNet)

# Source
#########################
file(GLOB_RECURSE SRC_FILES src/main/c++/*.cpp src/main/c++/*.h)

# Target
#########################
add_library(simplicity-raknet STATIC ${SRC_FILES})
target_include_directories(simplicity-raknet PUBLIC src/main/c++)

# Target Dependencies
#########################

# RakNet
target_include_directories(simplicity-raknet PUBLIC lib/RakNet/Source)
target_link_libraries(simplicity-raknet RakNetLibStatic)

# Simplicity
target_link_libraries(simplicity-raknet simplicity)
