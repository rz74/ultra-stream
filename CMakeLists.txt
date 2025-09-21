cmake_minimum_required(VERSION 3.10)
project(data_processing_service)

# Set common flags and output directory
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pthread")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Option for static linking (uncomment for portable executables)
# set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++")
# set(BUILD_SHARED_LIBS OFF)

# Include header directory
include_directories(include)

# Core application sources (packet_parser.cpp removed, parser_utils.cpp added)
set(SOURCES
    src/main.cpp
    src/data_book.cpp
    src/parser_utils.cpp
    src/composite_score_calculator.cpp
    src/udp_receiver.cpp
    src/tcp_sender.cpp
    src/logger.cpp
)

# Library sources (exclude main.cpp for shared library)
set(LIB_SOURCES
    src/data_book.cpp
    src/parser_utils.cpp
    src/composite_score_calculator.cpp
    src/udp_receiver.cpp
    src/tcp_sender.cpp
    src/logger.cpp
)

# Optional: Create shared library for the core functionality
# Uncomment the next line to build a shared library (.so/.dll)
# add_library(data_processing_engine SHARED ${LIB_SOURCES})

# Main application executable
add_executable(data_processing_service ${SOURCES})

# Test application (packet_parser.cpp removed, parser_utils.cpp added)
add_executable(test_all test/test_all.cpp
    src/data_book.cpp
    src/parser_utils.cpp
    src/composite_score_calculator.cpp
    src/logger.cpp
)

# UDP packet generator
add_executable(udp_packet_generator test/udp_packet_generator.cpp)

# TCP receiver
add_executable(tcp_receiver test/tcp_receiver.cpp)

# Ensure all test binaries go to build/bin
foreach(target data_processing_service test_all udp_packet_generator tcp_receiver)
    set_target_properties(${target} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
    )
endforeach()
