
cmake_minimum_required(VERSION 3.8)


if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project("DataProcessingService")

add_definitions(-D_WIN32_WINNT=0x0601)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Try to find Boost
find_package(Boost QUIET)
if(NOT Boost_FOUND)
    # Try common installation paths
    set(BOOST_ROOT "C:/boost_1_88_0")
    if(NOT EXISTS ${BOOST_ROOT})
        set(BOOST_ROOT "C:/local/boost_1_88_0")
    endif()
    
    set(Boost_NO_SYSTEM_PATHS ON)
    set(Boost_USE_STATIC_LIBS ON)
    set(Boost_USE_MULTITHREADED ON)
    
    find_package(Boost)
endif()

if(Boost_FOUND)
    message(STATUS "Boost found at: ${Boost_INCLUDE_DIRS}")
else()
    message(WARNING "Boost not found. You may need to install Boost or set BOOST_ROOT manually.")
endif()

add_executable(DataProcessingService "DataProcessingService.cpp")

if(Boost_FOUND)
    target_include_directories(DataProcessingService PRIVATE ${Boost_INCLUDE_DIRS})
endif()

if(WIN32)
    target_link_libraries(DataProcessingService PRIVATE ws2_32)
endif()

# Debug arguments
if(MSVC)
    set_target_properties(DataProcessingService PROPERTIES 
        VS_DEBUGGER_COMMAND_ARGUMENTS "239.255.0.1 12345"
        VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
    
    target_compile_definitions(DataProcessingService PRIVATE 
        DEFAULT_MULTICAST_ADDRESS="239.255.0.1"
        DEFAULT_PORT=12345)
endif()

