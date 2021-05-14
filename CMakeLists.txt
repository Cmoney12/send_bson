cmake_minimum_required(VERSION 3.19)
project(client_bson)

set(CMAKE_CXX_STANDARD 14)
find_package(ZLIB REQUIRED)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
find_library(ZSTD_LIBRARY NAMES zstdd zstd_staticd zstd zstd_static NAMES_PER_DIR)
find_package (bson-1.0 1.7 REQUIRED)
FIND_PACKAGE(Boost REQUIRED COMPONENTS system thread regex)
FIND_PACKAGE(Boost)
IF(Boost_FOUND)
    INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIR})
    LINK_DIRECTORIES(${Boost_LIBRARY_DIRS})
ENDIF(Boost_FOUND)

SET(USED_LIBS ${Boost_SYSTEM_LIBRARY} ${Boost_THREAD_LIBRARY} ${Boost_REGEX_LIBRARY})

add_executable(client_bson main.cpp chat_message.h)


target_link_libraries(client_bson ${ZSTD_LIBRARY} mongo::bson_shared Boost::boost)