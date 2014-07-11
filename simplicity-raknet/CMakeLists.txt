cmake_minimum_required(VERSION 2.8)

include(../../simplicity/simplicity/simplicity-common.cmake)

# Project
project(simplicity-raknet)

# Paths
file(GLOB_RECURSE SRC_FILES src/main/c++/*.cpp src/main/c++/*.h src/raknet/c++/*.cpp src/raknet/c++/*.h)
include_directories(../../simplicity/simplicity/src/main/c++)
include_directories(src/main/c++)
include_directories(src/raknet/c++)

# Output
add_library(simplicity-raknet STATIC ${SRC_FILES})

IF(WIN32)
	target_link_libraries(simplicity-raknet Ws2_32)
ENDIF(WIN32)
IF(UNIX)
	target_link_libraries(simplicity-raknet pthread)
ENDIF(UNIX)

