cmake_minimum_required(VERSION 2.8)

set(PROJECT_NAME
        xmrlmdbcpp)

project(${PROJECT_NAME})

set(CMAKE_CXX_FLAGS
        "${CMAKE_CXX_FLAGS} -std=c++11 -fpermissive")

# set location of monero source tree
set(MONERO_SOURCE_DIR
        ../monero)

# set location of monero build tree
set(MONERO_BUILD_DIR
        ${MONERO_SOURCE_DIR}/build/release)

# include monero headers
include_directories(
        ${MONERO_SOURCE_DIR}/src
        ${MONERO_SOURCE_DIR}/external
        ${MONERO_SOURCE_DIR}/external/db_drivers/liblmdb
        ${MONERO_SOURCE_DIR}/external/easylogging++
        ${MONERO_SOURCE_DIR}/contrib/epee/include)

add_library(common STATIC IMPORTED)
set_property(TARGET common PROPERTY IMPORTED_LOCATION ${MONERO_BUILD_DIR}/src/common/libcommon.a)

add_library(blocks STATIC IMPORTED)
set_property(TARGET blocks PROPERTY IMPORTED_LOCATION ${MONERO_BUILD_DIR}/src/blocks/libblocks.a)

add_library(cryptoxmr STATIC IMPORTED)
set_property(TARGET cryptoxmr
        PROPERTY IMPORTED_LOCATION ${MONERO_BUILD_DIR}/src/crypto/libcrypto.a)

add_library(cryptonote_basic STATIC IMPORTED)
set_property(TARGET cryptonote_basic
        PROPERTY IMPORTED_LOCATION ${MONERO_BUILD_DIR}/src/cryptonote_basic/libcryptonote_basic.a)

add_library(cryptonote_core STATIC IMPORTED)
set_property(TARGET cryptonote_core
        PROPERTY IMPORTED_LOCATION ${MONERO_BUILD_DIR}/src/cryptonote_core/libcryptonote_core.a)

add_library(cryptonote_protocol STATIC IMPORTED)
set_property(TARGET cryptonote_protocol
        PROPERTY IMPORTED_LOCATION ${MONERO_BUILD_DIR}/src/cryptonote_protocol/libcryptonote_protocol.a)

add_library(daemonizer STATIC IMPORTED)
set_property(TARGET daemonizer
        PROPERTY IMPORTED_LOCATION ${MONERO_BUILD_DIR}/src/daemonizer/libdaemonizer.a)

add_library(mnemonics STATIC IMPORTED)
set_property(TARGET mnemonics
        PROPERTY IMPORTED_LOCATION ${MONERO_BUILD_DIR}/src/mnemonics/libmnemonics.a)

add_library(epee STATIC IMPORTED)
set_property(TARGET epee
        PROPERTY IMPORTED_LOCATION ${MONERO_BUILD_DIR}/contrib/epee/src/libepee.a)

add_library(lmdb STATIC IMPORTED)
set_property(TARGET lmdb
        PROPERTY IMPORTED_LOCATION ${MONERO_BUILD_DIR}/external/db_drivers/liblmdb/liblmdb.a)

add_library(blockchain_db STATIC IMPORTED)
set_property(TARGET blockchain_db
        PROPERTY IMPORTED_LOCATION ${MONERO_BUILD_DIR}/src/blockchain_db/libblockchain_db.a)

add_library(ringct STATIC IMPORTED)
set_property(TARGET ringct
        PROPERTY IMPORTED_LOCATION ${MONERO_BUILD_DIR}/src/ringct/libringct.a)

add_library(wallet STATIC IMPORTED)
set_property(TARGET wallet
        PROPERTY IMPORTED_LOCATION ${MONERO_BUILD_DIR}/lib/libwallet.a)

if (EXISTS ${MONERO_BUILD_DIR}/external/unbound/libunbound.a)
    add_library(unbound STATIC IMPORTED)
    set_property(TARGET unbound PROPERTY IMPORTED_LOCATION ${MONERO_BUILD_DIR}/external/unbound/libunbound.a)
endif()

# find boost
find_package(Boost COMPONENTS
        system
        filesystem
        thread
        date_time
        chrono
        regex
        serialization
        program_options
        date_time
        REQUIRED)

include_directories(${Boost_INCLUDE_DIRS})


add_subdirectory(ext/)
add_subdirectory(src/)

set(SOURCE_FILES
        main.cpp)

add_executable(${PROJECT_NAME}
        ${SOURCE_FILES})

set(LIBRARIES
        myxrm
        myext
        blockchain_db
        cryptonote_core
        cryptonote_protocol
        cryptonote_basic
        cryptoxmr
        blocks
        lmdb
        ringct
        common
        epee
        ${Boost_LIBRARIES}
        pthread
        unbound
        crypto
        ssl)

if (NOT ${CMAKE_SYSTEM_NAME} MATCHES "Darwin" AND NOT WIN32)
    set(LIBRARIES ${LIBRARIES} unwind)
endif()

if (WIN32)
    set(LIBRARIES ${LIBRARIES}
        wsock32
        ntdll
        ws2_32
        Iphlpapi)
else()
    set(LIBRARIES ${LIBRARIES} dl)
endif()

target_link_libraries(${PROJECT_NAME} ${LIBRARIES})
