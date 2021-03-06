set (CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMake")
include(cotire)
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
    src/kcp.cpp
    src/net.cpp
    src/server.cpp
    src/client.cpp
    src/tcpclient.cpp
    src/tcpserver.cpp
    src/udpclient.cpp
    src/udpserver.cpp
    src/utils.cpp
    src/buffer.cpp
    src/sockbuffer.cpp
    src/eventloop.cpp
    src/eventloop_thread.cpp
    src/eventloop_threadpool.cpp
    src/connection.cpp
    src/connection_manager.cpp
    src/uniqid.cpp
    src/threadbase.cpp
    src/thread.cpp
    src/exception.cpp
    src/count_down_latch.cpp
    src/logger/logger.cpp
    src/logger/append_file.cpp
    src/logger/log_file.cpp
    src/logger/log.cpp
    external/ae.c
    external/wrapsock.c
    external/zmalloc.c
    external/sock_ntop.c
    external/writen.c
    external/ikcp.c
    external/error.c)

FILE(GLOB MY_HEADERS
    include/protocol_define.h
    include/protocol.h
    include/net.h
    include/connection.h
    include/connection_manager.h
    include/kcp.h
    include/sockbase.h
    include/buffer.h
    include/sockbuffer.h
    include/client.h
    include/server.h
    include/udpclient.h
    include/tcpclient.h
    include/udpserver.h
    include/tcpserver.h
    include/uniqid.h
    include/utils.h
    include/eventloop.h
    include/eventloop_thread.h
    include/eventloop_threadpool.h
    include/common.h
    include/mutex.h
    include/condition.h
    include/threadbase.h
    include/thread.h
    include/exception.h
    include/count_down_latch.h
    include/logger/logging_buffer.h
    include/logger/logger.h
    include/logger/append_file.h
    include/logger/log_file.h
    include/logger/log.h
    external/wrapsock.h
    external/error.h
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

set_target_properties(wynet PROPERTIES COTIRE_CXX_PREFIX_HEADER_INIT "include/common.h")
cotire(wynet)

install(TARGETS wynet DESTINATION lib)


set( project_tests
    test_client
    test_server
    test_eventloop1
    test_eventloop2
    test_eventloopthread
    test_buffer
    test_concurrentqueue
    test_logging
    bench_client
    bench_server
   )

set( EXE_test_client_SOURCES
    test/test_client.cpp
)
set( EXE_test_server_SOURCES
    test/test_server.cpp
)
set( EXE_test_eventloop1_SOURCES
    test/test_eventloop1.cpp
)

set( EXE_test_eventloop2_SOURCES
    test/test_eventloop2.cpp
)

set( EXE_test_eventloopthread_SOURCES
    test/test_eventloopthread.cpp
)

set( EXE_test_buffer_SOURCES
    test/test_buffer.cpp
)

set( EXE_test_concurrentqueue_SOURCES
    test/test_concurrentqueue.cpp
)

set( EXE_test_logging_SOURCES
    test/test_logging.cpp
)

set( EXE_bench_client_SOURCES
    test/bench_client.cpp
)

set( EXE_bench_server_SOURCES
    test/bench_server.cpp
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
