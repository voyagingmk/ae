cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)


project(wynet LANGUAGES CXX VERSION 0.0.0)

set(CMAKE_BUILD_TYPE Debug)
SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g2 -ggdb")  
SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")  

if(UNIX)
    MESSAGE(STATUS "It is UNIX-like OS")  
    SET (CMAKE_C_COMPILER             "/usr/bin/clang")
    SET (CMAKE_CXX_COMPILER             "/usr/bin/clang++")
endif()

set (CMAKE_CXX_STANDARD 11)
set (CMAKE_CXX_STANDARD_REQUIRED ON)
set (BUILD_SHARED_LIBS false)

FILE(GLOB_RECURSE MY_SOURCERS
    src/wykcp.cpp
    src/wynet.cpp
    src/wyserver.cpp
    src/wyclient.cpp
    src/wytcpclient.cpp
    src/wytcpserver.cpp
    src/wyudpclient.cpp
    src/wyudpserver.cpp
    src/wyutils.cpp
    src/event_loop.cpp
    src/uniqid.cpp
    src/wythread.cpp
    src/exception.cpp
    src/count_down_latch.cpp
    src/logger/logger.cpp
    src/logger/log_file.cpp
    external/ae.c
    external/wrapsock.c
    external/zmalloc.c
    external/sock_ntop.c
    external/writen.c
    external/ikcp.c
    external/log.c
    external/error.c)

SET(HEADERS
    include/protocol_define.h
    include/protocol.h
    include/wynet.h
    include/wyconnection.h
    include/wykcp.h
    include/wysockbase.h
    include/wybuffer.h
    include/wyclient.h
    include/wyserver.h
    include/wyudpclient.h
    include/wytcpclient.h
    include/wyudpserver.h
    include/wytcpserver.h
    include/uniqid.h
    include/wyutils.h
    include/event_loop.h
    include/common.h
    include/mutex.h
    include/condition.h
    include/wythread.h
    include/exception.h
    include/count_down_latch.h
    include/logger/logging_buffer.h
    include/logger/logger.h
    include/logger/log_file.h
    external/wrapsock.h
    external/error.h
    external/log.h
    external/ikcp.h
    external/ae.h
    external/zmalloc.h
    external/unp.h
  )

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package( Threads )

add_executable(example_server example_server.cpp  ${MY_SOURCERS} ${HEADERS})

add_executable(example_client example_client.cpp  ${MY_SOURCERS} ${HEADERS})

target_include_directories(
    example_server
    PUBLIC
    include
    src
    external
    )
    
target_include_directories(
    example_client
    PUBLIC
    include
    src
    external
    )

target_compile_features(example_server PRIVATE cxx_range_for)

target_compile_features(example_client PRIVATE cxx_range_for)

target_link_libraries(example_server ${CMAKE_THREAD_LIBS_INIT})

target_link_libraries(example_client ${CMAKE_THREAD_LIBS_INIT})

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
