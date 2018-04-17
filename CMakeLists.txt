cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)

project(wynet_project LANGUAGES C CXX VERSION 0.0.0)


if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set(MACOSX TRUE)
endif()

set (CMAKE_CXX_STANDARD 11)
set (CMAKE_CXX_STANDARD_REQUIRED ON)
set (BUILD_SHARED_LIBS false)
# set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_BUILD_TYPE Debug)

#set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++ -lc++abi")

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=gnu++0x")

SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS}  -O0  -g2 -ggdb")  
SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 ")  

#if(MACOSX)
#    MESSAGE(STATUS "It is MACOSX")  
if(UNIX)
    MESSAGE(STATUS "It is UNIX")  
    
    SET (CMAKE_C_COMPILER             "/usr/bin/clang")
    SET (CMAKE_CXX_COMPILER             "/usr/bin/clang++")
endif()


FILE(GLOB MY_SOURCERS
    src/wykcp.cpp
    src/wynet.cpp
    src/wyserver.cpp
    src/wyclient.cpp
    src/wytcpclient.cpp
    src/wytcpserver.cpp
    src/wyudpclient.cpp
    src/wyudpserver.cpp
    src/wyutils.cpp
    src/eventloop.cpp
    src/eventloop_thread.cpp
    src/eventloop_threadpool.cpp
    src/uniqid.cpp
    src/wythreadbase.cpp
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

FILE(GLOB MY_HEADERS
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
    include/eventloop.h
    include/eventloop_thread.h
    include/eventloop_threadpool.h
    include/common.h
    include/mutex.h
    include/condition.h
    include/wythreadbase.h
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


add_library(wynet STATIC ${MY_SOURCERS} ${MY_HEADERS})
target_include_directories(
        wynet
        PUBLIC
        include
        src
        external
        )
install(TARGETS wynet DESTINATION lib)


set( project_tests
    test_client
    test_server
    test_eventloop
    test_eventloopthread
   )

set( EXE_test_client_SOURCES
    test/test_client.cpp
)
set( EXE_test_server_SOURCES
    test/test_server.cpp
)
set( EXE_test_eventloop_SOURCES
    test/test_eventloop.cpp
)
set( EXE_test_eventloopthread_SOURCES
    test/test_eventloopthread.cpp
)

# For each executable in the list...
foreach( exe ${project_tests} )
    # declare the target...
    add_executable( ${exe} ${EXE_${exe}_SOURCES})
    target_include_directories(
        ${exe}
        PUBLIC
        include
        src
        external
        )
    target_compile_features(${exe} PRIVATE cxx_range_for)
    target_link_libraries(${exe} PRIVATE wynet ${CMAKE_THREAD_LIBS_INIT})    
endforeach()

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
