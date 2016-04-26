
set(lmdb_sources
       mdb.c
       midl.c)

add_library(lmdb
        ${lmdb_sources})

target_link_libraries(lmdb)

