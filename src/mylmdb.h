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

    using epee::string_tools::pod_to_hex;

    using namespace std;


    class MyLMDB
    {


        static const uint64_t DEFAULT_MAPSIZE = 10UL * 1024UL * 1024UL * 1024UL; /* 10 GiB */
        static const uint64_t DEFAULT_NO_DBs  = 5;


        string m_db_path;

        uint64_t m_mapsize;
        uint64_t m_no_dbs;

        lmdb::env m_env;


    public:
        MyLMDB(string _path,
               uint64_t _mapsize = DEFAULT_MAPSIZE,
               uint64_t _no_dbs = DEFAULT_NO_DBs)
                : m_db_path {_path},
                  m_mapsize {_mapsize},
                  m_no_dbs {_no_dbs},
                  m_env {nullptr}
        {
            create_and_open_env();
        }

        bool
        create_and_open_env()
        {
            try
            {   m_env = lmdb::env::create();
                m_env.set_mapsize(m_mapsize);
                m_env.set_max_dbs(m_no_dbs);
                m_env.open(m_db_path.c_str(), MDB_CREATE, 0664);
            }
            catch (lmdb::error& e )
            {
                cerr << e.what() << endl;
                return false;
            }

            return true;
        }


        bool
        write_key_images(const transaction& tx)
        {
            crypto::hash tx_hash = get_transaction_hash(tx);

            string tx_hash_str = pod_to_hex(tx_hash);

            vector<cryptonote::txin_to_key> key_images
                    = xmreg::get_key_images(tx);

            lmdb::txn wtxn {nullptr};
            lmdb::dbi wdbi {0};

            unsigned int flags = MDB_CREATE | MDB_DUPSORT | MDB_DUPFIXED;

            try
            {
                wtxn = lmdb::txn::begin(m_env);
                wdbi  = lmdb::dbi::open(wtxn, "key_images", flags);
            }
            catch (lmdb::error& e )
            {
                cerr << e.what() << endl;
                return false;
            }

            for (const cryptonote::txin_to_key& key_image: key_images)
            {
                string key_img_str = pod_to_hex(key_image.k_image);

                lmdb::val key_img_val {key_img_str};
                lmdb::val tx_hash_val {tx_hash_str};

                wdbi.put(wtxn, key_img_val, tx_hash_val);
            }

            try
            {
                wtxn.commit();
            }
            catch (lmdb::error& e )
            {
                cerr << e.what() << endl;
                return false;
            }

            return true;
        }

        bool
        write_public_keys(const transaction& tx)
        {
            crypto::hash tx_hash = get_transaction_hash(tx);

            string tx_hash_str = pod_to_hex(tx_hash);

            vector<pair<cryptonote::txout_to_key, uint64_t>> outputs
                    = xmreg::get_ouputs(tx);

            lmdb::txn wtxn {nullptr};
            lmdb::dbi wdbi {0};

            unsigned int flags = MDB_CREATE | MDB_DUPSORT | MDB_DUPFIXED;

            try
            {
                wtxn = lmdb::txn::begin(m_env);
                wdbi = lmdb::dbi::open(wtxn, "public_keys", flags);
            }
            catch (lmdb::error& e )
            {
                cerr << e.what() << endl;
                return false;
            }

            for (const pair<cryptonote::txout_to_key, uint64_t>& output: outputs)
            {
                string public_key_str = pod_to_hex(output.first.key);

                lmdb::val public_key_val {public_key_str};
                lmdb::val tx_hash_val    {tx_hash_str};

                wdbi.put(wtxn, public_key_val, tx_hash_val);
            }

            try
            {
                wtxn.commit();
            }
            catch (lmdb::error& e )
            {
                cerr << e.what() << endl;
                return false;
            }

            return true;
        }

        template<typename T>
        vector<T> search(const T& key)
        {
            vector<T> out;

            lmdb::txn    rtxn {nullptr};
            lmdb::dbi    rdbi {0};
            lmdb::cursor cr   {nullptr};

            unsigned int flags = MDB_RDONLY | MDB_DUPSORT | MDB_DUPFIXED;

            try
            {
                rtxn = lmdb::txn::begin(m_env, nullptr, MDB_RDONLY);
                rdbi = lmdb::dbi::open(rtxn, "key_images", flags);
                cr   = lmdb::cursor::open(rtxn, rdbi);
            }
            catch (lmdb::error& e )
            {
                cerr << e.what() << endl;
                return out;
            }

            return out;
        }



    };

}

#endif //XMRLMDBCPP_MYLMDB_H
