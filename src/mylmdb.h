//
// Created by mwo on 27/04/16.
//
#ifndef XMRLMDBCPP_MYLMDB_H
#define XMRLMDBCPP_MYLMDB_H

#include "../ext/lmdb++.h"

#include <iostream>
#include <memory>

namespace xmreg
{

    using namespace std;


    const uint64_t DEFAULT_MAPSIZE = 10UL * 1024UL * 1024UL * 1024UL; /* 10 GiB */
    const uint64_t DEFAULT_NO_DBs  = 5;


    class MyLMDB
    {

        string m_db_path;

        uint64_t m_mapsize;
        uint64_t m_no_dbs;

        MDB_env* handle {nullptr};

        unique_ptr<lmdb::env> m_env;


    public:
        MyLMDB(string _path,
               uint64_t _mapsize = DEFAULT_MAPSIZE,
               uint64_t _no_dbs=DEFAULT_NO_DBs)
                : m_db_path {_path},
                  m_mapsize {_mapsize},
                  m_no_dbs {_no_dbs}
        {
            create_and_open_env();
        }

        bool create_and_open_env()
        {
            try
            {   m_env = unique_ptr<lmdb::env>(new lmdb::env(handle));
                m_env->open(m_db_path.c_str(), MDB_CREATE, 0664);
            }
            catch (lmdb::error& e )
            {
                cerr << e.what() << end;
                return false;
            }

            return true;
        }

        lmdb::dbi open_db_handle()
        {
            return lmdb::dbi(*handle);
        }

    };

}

#endif //XMRLMDBCPP_MYLMDB_H
