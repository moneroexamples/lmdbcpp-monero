cmake_minimum_required(VERSION 3.5)
project(lmdbcpp_test)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")


add_subdirectory(ext/)

set(SOURCE_FILES
        main.cpp)

add_executable(lmdbcpp_test
        ${SOURCE_FILES})


target_include_directories(lmdbcpp_test
        PUBLIC "${LMDB_INCLUDE}")

target_link_libraries(lmdbcpp_test
        lmdb
        pthread)

