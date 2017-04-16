cmake_minimum_required(VERSION 2.8)

project(myxmr)

set(SOURCE_HEADERS
        MicroCore.h
		tools.h
		mylmdb.h
		monero_headers.h
		tx_details.h)

set(SOURCE_FILES
		MicroCore.cpp
		tools.cpp
		CmdLineOptions.cpp
		tx_details.cpp mylmdb.h)

# make static library called libmyxrm
# that we are going to link to
# in the root CMakeLists.txt file
add_library(myxmr
		STATIC
		${SOURCE_FILES})

target_include_directories(myxmr
		PUBLIC "${LMDB_INCLUDE}")
