cmake_minimum_required(VERSION 3.2)

include(../simplicity/simplicity-common.cmake)

# Project
#########################
project(simplicity-raknet)

# Source
#########################
file(GLOB_RECURSE SRC_FILES src/main/c++/*.cpp src/main/c++/*.h src/raknet/c++/*.cpp src/raknet/c++/*.h)

# Target
#########################
add_library(simplicity-raknet STATIC ${SRC_FILES})
target_include_directories(simplicity-raknet PUBLIC src/main/c++)

# Target Dependencies
#########################

# RakNet
target_include_directories(simplicity-raknet PUBLIC src/raknet/c++)

# Ws2
IF(WIN32)
	target_link_libraries(simplicity-raknet Ws2_32)
ENDIF(WIN32)

# pthread
IF(UNIX)
	target_link_libraries(simplicity-raknet pthread)
ENDIF(UNIX)

# Simplicity
target_link_libraries(simplicity-raknet simplicity)
