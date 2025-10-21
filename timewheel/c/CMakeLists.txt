cmake_minimum_required(VERSION 3.10)
project(timer_wheel C)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Add compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g -O0 -DDEBUG_TIMER_WHEEL")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O2")

# Find urcu library
find_library(URCU_LIBRARY NAMES urcu)
find_path(URCU_INCLUDE_DIR urcu/list.h)

if(NOT URCU_LIBRARY)
    message(FATAL_ERROR "liburcu not found. Please install it using: sudo apt-get install liburcu-dev")
endif()

if(URCU_INCLUDE_DIR)
    include_directories(${URCU_INCLUDE_DIR})
    message(STATUS "Found urcu include directory: ${URCU_INCLUDE_DIR}")
else()
    message(STATUS "Using system urcu headers")
endif()

message(STATUS "Found urcu library: ${URCU_LIBRARY}")

# Include current directory for headers
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Source files
set(SOURCES
    timer_wheel.c
    helper.c
    main.c
)

# Create executable
add_executable(timer_wheel_demo ${SOURCES})

# Link libraries
target_link_libraries(timer_wheel_demo ${URCU_LIBRARY})

# Optional: Install target
install(TARGETS timer_wheel_demo DESTINATION bin)

# Print build information
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C Compiler: ${CMAKE_C_COMPILER}")
message(STATUS "C Flags: ${CMAKE_C_FLAGS}")
