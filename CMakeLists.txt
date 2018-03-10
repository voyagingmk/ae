cmake_minimum_required(VERSION 3.0.0)


project(wynet VERSION 0.0.0)

set(CMAKE_BUILD_TYPE Debug)
SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g2 -ggdb")  
SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")  

if(UNIX)
    MESSAGE(STATUS "It is UNIX-like OS")  
    SET (CMAKE_C_COMPILER             "/usr/bin/clang")
    SET (CMAKE_CXX_COMPILER             "/usr/bin/clang++")
endif()

set (CMAKE_CXX_STANDARD 11)
set (BUILD_SHARED_LIBS false)

include_directories("${PROJECT_SOURCE_DIR}")
include_directories("${PROJECT_SOURCE_DIR}/include")
include_directories("${PROJECT_SOURCE_DIR}/external")
include_directories("${PROJECT_SOURCE_DIR}/src")

FILE(GLOB_RECURSE MY_SOURCERS
    wykcp.cpp
    wynet.cpp
    wyserver.cpp
    wyclient.cpp
    wytcpclient.cpp
    wytcpserver.cpp
    wyudpclient.cpp
    wyudpserver.cpp
    wyutils.cpp
    uniqid.cpp
    ae.c
    wrapsock.c
    zmalloc.c
    sock_ntop.c
    writen.c
    ikcp.c
    log.c
    error.c)

SET(HEADERS
    protocol_define.h
    protocol.h
    ikcp.h
    ae.h
    common.h
    zmalloc.h
    unp.h
    wynet.h
    wrapsock.h
    wyconnection.h
    wykcp.h
    wysockbase.h
    wybuffer.h
    wyclient.h
    wyserver.h
    wyudpclient.h
    wytcpclient.h
    wyudpserver.h
    wytcpserver.h
    uniqid.h
    wyutils.h
    error.h
    log.h
  )

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_executable(example_server example_server.cpp  ${MY_SOURCERS} ${HEADERS})

add_executable(example_client example_client.cpp  ${MY_SOURCERS} ${HEADERS})

target_link_libraries(example_server Threads::Threads)

target_link_libraries(example_client Threads::Threads)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
