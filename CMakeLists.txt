cmake_minimum_required(VERSION 3.0.0)


project(wynet VERSION 0.0.0)

if(UNIX)
    MESSAGE(STATUS "It is UNIX-like OS")  
    SET (CMAKE_C_COMPILER             "/usr/bin/clang")
    SET (CMAKE_CXX_COMPILER             "/usr/bin/clang++")
endif()

set (CMAKE_CXX_STANDARD 11)
set (BUILD_SHARED_LIBS false)

include_directories("${PROJECT_SOURCE_DIR}")
FILE(GLOB_RECURSE MY_SOURCERS
    wykcp.cpp
    wynet.cpp
    wyserver.cpp
    wyclient.cpp
    wytcpclient.cpp
    wytcpserver.cpp
    wyudpclient.cpp
    wyudpserver.cpp
    uniqid.cpp
    ae.c
    wrapsock.c
    zmalloc.c
    sock_ntop.c
    readline.c
    writen.c
    ikcp.c
    error.c)

add_executable(example_server example_server.cpp  ${MY_SOURCERS})

add_executable(example_client example_client.cpp  ${MY_SOURCERS})

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
