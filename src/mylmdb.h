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
    using epee::string_tools::hex_to_pod;

    using namespace std;

    struct txindex {
        crypto::hash key;
        tx_data_t data;
    };

    /**
     * Stores info about outputs useful
     * for checking which ouputs belong to a
     * given address and viewkey
     */
    struct output_info
    {
        crypto::public_key out_pub_key;
        uint64_t           tx_id; // tx id blockchain's lmdb tx table
        crypto::public_key tx_pub_key;
        uint64_t           amount;
        uint64_t           index_in_tx;
    };

    std::ostream& operator<<(std::ostream& os, const output_info&  out_info)
    {
        os  << ", out_pub_key: " << out_info.out_pub_key
            << ", tx_id: "       << out_info.tx_id
            << ", tx_pub_key: "  << out_info.tx_pub_key
            << ", amount: "      << XMR_AMOUNT(out_info.amount)
            << ", index_in_tx: " << out_info.index_in_tx;

        return os;
    }

    static const char *DBI_NAMES[] = {
        "key_images",
        "output_public_keys",
        "output_amounts",
        "output_info",
        "tx_public_keys",
        "payments_id",
        "encrypted_payments_id"
    };

    class MyLMDB
    {

        static const uint64_t DEFAULT_MAPSIZE = 30UL * 1024UL * 1024UL * 1024UL; /* 30 GiB */
        static const uint64_t DEFAULT_NO_DBs  = 10;

    public:
        enum D_dbi
        {
            D_key_images,
            D_output_public_keys,
            D_output_amounts,
            D_output_info,
            D_tx_public_keys,
            D_payments_id,
            D_encrypted_payments_id,
            D_NUM_DBIS
        };


    private:
        string m_db_path;

        uint64_t m_mapsize;
        uint64_t m_no_dbs;

        lmdb::env m_env;
        lmdb::txn m_wtxn;
        lmdb::dbi *m_dbis;


    public:
        MyLMDB(string _path,
               uint64_t _mapsize = DEFAULT_MAPSIZE,
               uint64_t _no_dbs = DEFAULT_NO_DBs)
                : m_db_path {_path},
                  m_mapsize {_mapsize},
                  m_no_dbs {_no_dbs},
                  m_env {nullptr}, m_wtxn {nullptr}
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

                m_env.open(m_db_path.c_str(), MDB_CREATE|MDB_NOSYNC, 0664);

                m_wtxn = lmdb::txn::begin(m_env);

                m_dbis = static_cast<lmdb::dbi *>(::operator new[](D_NUM_DBIS * sizeof(lmdb::dbi)));

                unsigned int i, flags = MDB_CREATE | MDB_DUPSORT | MDB_DUPFIXED;

                for (i=D_key_images; i<D_NUM_DBIS; i++)
                    m_dbis[i] = lmdb::dbi::open(m_wtxn, DBI_NAMES[i], flags);

                m_wtxn.commit();
            }
            catch (lmdb::error& e )
            {
                cerr << e.what() << endl;
                return false;
            }

            return true;
        }

        bool
        sync()
        {
            try
            {
                m_env.sync();
            }
            catch (lmdb::error& e )
            {
                cerr << e.what() << endl;
                return false;
            }

            return true;
        }

        bool
        begin_txn()
        {
            try
            {
                m_wtxn = lmdb::txn::begin(m_env);
            }
            catch (lmdb::error& e )
            {
                cerr << e.what() << endl;
                return false;
            }

            return true;
        }

        bool
        end_txn()
        {
            try
            {
                m_wtxn.commit();
            }
            catch (lmdb::error& e )
            {
                cerr << e.what() << endl;
                return false;
            }

            return true;
        }

        bool
        write_key_images(const uint64_t& tx_id, const transaction& tx)
        {

            lmdb::val tx_id_val = pod_to_val(tx_id);

            vector<cryptonote::txin_to_key> key_images
                    = xmreg::get_key_images(tx);

            for (const cryptonote::txin_to_key& key_image: key_images)
            {
                string key_img_str = pod_to_hex(key_image.k_image);

                lmdb::val key_img_val {key_img_str};
                m_dbis[D_key_images].put(m_wtxn, key_img_val, tx_id_val);
            }
            return true;
        }

        bool
        write_output_public_keys(const uint64_t& tx_id, const transaction& tx, const block& blk)
        {
            crypto::public_key tx_pub_key = get_tx_pub_key_from_extra(tx);

            lmdb::val tx_id_val = pod_to_val(tx_id);;

            vector<tuple<txout_to_key, uint64_t, uint64_t>> outputs =
                    xmreg::get_ouputs_tuple(tx);

            for (auto& output: outputs)
            {

                public_key out_pub_key = std::get<0>(output).key;

                string public_key_str = pod_to_hex(out_pub_key);

                lmdb::val public_key_val {public_key_str};

                uint64_t amount = std::get<1>(output);

                lmdb::val amount_val = pod_to_val(amount);

                uint64_t index_in_tx = std::get<2>(output);

                output_info out_info {out_pub_key, tx_id,
                                      tx_pub_key, amount,
                                      index_in_tx};

                uint64_t out_timestamp = blk.timestamp;

                lmdb::val out_timestamp_val     {static_cast<void*>(&out_timestamp),
                                                 sizeof(out_timestamp)};
                lmdb::val out_info_val          {static_cast<void*>(&out_info),
                                                 sizeof(out_info)};

                m_dbis[D_output_public_keys].put(m_wtxn, public_key_val, tx_id_val);
                m_dbis[D_output_amounts].put(m_wtxn, public_key_val, amount_val);
                m_dbis[D_output_info].put(m_wtxn, out_timestamp_val, out_info_val);
            }

            return true;
        }

        bool
        write_tx_public_key(const uint64_t& tx_id, const transaction& tx)
        {

            lmdb::val tx_id_val = pod_to_val(tx_id);

            public_key pk = get_tx_pub_key_from_extra(tx);

            string pk_str = pod_to_hex(pk);

            try
            {
                lmdb::val public_key_val {pk_str};

                m_dbis[D_tx_public_keys].put(m_wtxn, public_key_val, tx_id_val);
            }
            catch (lmdb::error& e)
            {
                cerr << e.what() << endl;
                return false;
            }

            return true;
        }

        bool
        write_payment_id(const uint64_t& tx_id, const transaction& tx)
        {
            lmdb::val tx_id_val = pod_to_val(tx_id);

            crypto::hash  payment_id;
            crypto::hash8 payment_id8;

            get_payment_id(tx, payment_id, payment_id8);

            if (payment_id == null_hash)
            {
                return true;
            }

            string payment_id_str = pod_to_hex(payment_id);

            try
            {
                //cout << "Saving payment_id: " << payment_id_str << endl;

                lmdb::val payment_id_val {payment_id_str};

                m_dbis[D_payments_id].put(m_wtxn, payment_id_val, tx_id_val);
            }
            catch (lmdb::error& e)
            {
                cerr << e.what() << endl;
                return false;
            }

            return true;
        }

        bool
        write_encrypted_payment_id(const uint64_t& tx_id, const transaction& tx)
        {
            lmdb::val tx_id_val = pod_to_val(tx_id);

            crypto::hash  payment_id;
            crypto::hash8 payment_id8;

            get_payment_id(tx, payment_id, payment_id8);

            if (payment_id8 == null_hash8)
            {
                return true;
            }

            string payment_id_str = pod_to_hex(payment_id8);

            try
            {
                //cout << "Saving encrypted payiment_id: " << payment_id_str << endl;
                //string wait_for_enter;
                //cin >> wait_for_enter;

                lmdb::val payment_id_val {payment_id_str};

                m_dbis[D_encrypted_payments_id].put(m_wtxn, payment_id_val, tx_id_val);
            }
            catch (lmdb::error& e)
            {
                cerr << e.what() << endl;
                return false;
            }

            return true;
        }

        bool
        search(const string& key,
               vector<uint64_t >& found_tx_ids,
               const enum D_dbi rdbi = D_key_images)
        {
            unsigned int flags = MDB_DUPSORT | MDB_DUPFIXED;

            try
            {

                lmdb::txn rtxn  = lmdb::txn::begin(m_env, nullptr, MDB_RDONLY);
                lmdb::cursor cr = lmdb::cursor::open(rtxn, m_dbis[rdbi]);

                lmdb::val key_to_find{key};
                lmdb::val tx_id_val;

                // set cursor the the first item
                if (cr.get(key_to_find, tx_id_val, MDB_SET))
                {
                    uint64_t tx_id = *(tx_id_val.data<uint64_t>());

                    //cout << key_val_to_str(key_to_find, tx_hash_val) << endl;
                    found_tx_ids.push_back(tx_id);

                    // process other values for the same key
                    while (cr.get(key_to_find, tx_id_val, MDB_NEXT_DUP))
                    {
                        //cout << key_val_to_str(key_to_find, tx_hash_val) << endl;
                        found_tx_ids.push_back(tx_id);
                    }
                }
                else
                {
                    return false;
                }

                cr.close();
                rtxn.abort();

            }
            catch (lmdb::error& e)
            {
                cerr << e.what() << endl;
                return false;
            }

            return true;
        }

        bool
        get_output_amount(const string& key,
                          uint64_t& amount,
                          const enum D_dbi rdbi = D_output_amounts)
        {

            unsigned int flags = 0;

            try
            {

                lmdb::txn rtxn  = lmdb::txn::begin(m_env, nullptr, MDB_RDONLY);

                lmdb::val key_to_find{key};
                lmdb::val amount_val;

                if(!m_dbis[rdbi].get(rtxn, key_to_find, amount_val))
                {
                    return false;
                }

                amount = *(amount_val.data<uint64_t>());

                rtxn.abort();

            }
            catch (lmdb::error& e)
            {
                cerr << e.what() << endl;
                return false;
            }

            return true;
        }


        bool
        get_output_info(uint64_t key_timestamp,
                        vector<output_info>& out_infos,
                        const enum D_dbi rdbi = D_output_info)
        {

            unsigned int flags = 0;

            try
            {

                lmdb::txn rtxn  = lmdb::txn::begin(m_env, nullptr, MDB_RDONLY);

                lmdb::val key_to_find{static_cast<void*>(&key_timestamp),
                                      sizeof(key_timestamp)};
                lmdb::val info_val;

                lmdb::cursor cr = lmdb::cursor::open(rtxn, m_dbis[rdbi]);

                // set cursor the the first item
                if (cr.get(key_to_find, info_val, MDB_SET_RANGE))
                {
                    out_infos.push_back(*(info_val.data<output_info>()));

                    // process other values for the same key
                    while (cr.get(key_to_find, info_val, MDB_NEXT_DUP))
                    {
                        //cout << key_val_to_str(key_to_find, tx_hash_val) << endl;
                        out_infos.push_back(*(info_val.data<output_info>()));
                    }
                }
                else
                {
                    return false;
                }

                rtxn.abort();

            }
            catch (lmdb::error& e)
            {
                cerr << e.what() << endl;
                return false;
            }

            return true;
        }

        bool
        get_output_info_range(uint64_t key_timestamp_start,
                              uint64_t key_timestamp_end,
                              vector<pair<uint64_t, output_info>>& out_infos,
                              const enum D_dbi rdbi = D_output_info)
        {

            unsigned int flags = 0;

            try
            {
                lmdb::txn rtxn  = lmdb::txn::begin(m_env, nullptr, MDB_RDONLY);

                lmdb::val key_to_find{static_cast<void*>(&key_timestamp_start),
                                      sizeof(key_timestamp_start)};
                lmdb::val info_val;

                lmdb::cursor cr = lmdb::cursor::open(rtxn, m_dbis[rdbi]);

                uint64_t current_timestamp = key_timestamp_start;

                // set cursor the the first item
                if (cr.get(key_to_find, info_val, MDB_SET_RANGE))
                {

                    current_timestamp = *key_to_find.data<uint64_t>();

                    if (current_timestamp > key_timestamp_end)
                    {
                        return false;
                    }

                    out_infos.push_back(make_pair(
                            current_timestamp,
                            *(info_val.data<output_info>())));

                    // process other values for the same key
                    while (cr.get(key_to_find, info_val, MDB_NEXT))
                    {
                        current_timestamp = *key_to_find.data<uint64_t>();

                        if (current_timestamp > key_timestamp_end)
                        {
                            break;
                        }

                        out_infos.push_back(make_pair(
                                current_timestamp,
                                *(info_val.data<output_info>())));
                    }
                }
                else
                {
                    return false;
                }

                rtxn.abort();

            }
            catch (lmdb::error& e)
            {
                cerr << e.what() << endl;
                return false;
            }

            return true;
        }

        /**
         * Returns sorted and unique tx hashes withing a
         * given timestamp range
         *
         * @param key_timestamp_start
         * @param key_timestamp_end
         * @param out_txs
         * @return bool
         */
        bool
        get_txs_from_timestamp_range(uint64_t key_timestamp_start,
                                     uint64_t key_timestamp_end,
                                     vector<uint64_t >& out_txs)
        {
            using output_pair = pair<uint64_t, output_info>;

            auto sort_by_timestamp = [](const output_pair& l,
                                        const output_pair& r)
            {
                return l.first < r.first;
            };

            vector<output_pair> out_infos;

            if (get_output_info_range(key_timestamp_start,
                                      key_timestamp_end,
                                      out_infos))
            {

                set<output_pair, decltype(sort_by_timestamp)> unique_txs(sort_by_timestamp);

                for (auto oi: out_infos)
                    unique_txs.insert(oi);

                for (auto ut: unique_txs)
                    out_txs.push_back(ut.second.tx_id);

                return true;
            }

            return false;
        }



        void
        for_all_outputs(
                std::function<bool(public_key& out_pubkey,
                                   output_info& out_info)> f)
        {
            unsigned int flags = MDB_DUPSORT | MDB_DUPFIXED;

            try
            {

                lmdb::txn rtxn  = lmdb::txn::begin(m_env, nullptr, MDB_RDONLY);
                lmdb::cursor cr = lmdb::cursor::open(rtxn, m_dbis[D_output_info]);

                lmdb::val key_to_find;
                lmdb::val amount_val;


                // process all values for the same key
                while (cr.get(key_to_find, amount_val, MDB_NEXT))
                {
                    public_key pub_key;

                    hex_to_pod(string(key_to_find.data(), key_to_find.size()),
                               pub_key);

                    output_info out_info =  *(amount_val.data<output_info>());

                    if (f(pub_key, out_info) == false)
                    {
                        break;
                    }
                }

                cr.close();
                rtxn.abort();

            }
            catch (lmdb::error& e)
            {
                cerr << e.what() << endl;
            }
        }


        void
        print_all(const enum D_dbi rdbi)
        {
            unsigned int flags = MDB_DUPSORT | MDB_DUPFIXED;

            try
            {

                lmdb::txn rtxn  = lmdb::txn::begin(m_env, nullptr, MDB_RDONLY);
                lmdb::cursor cr = lmdb::cursor::open(rtxn, m_dbis[rdbi]);

                lmdb::val key_to_find;
                lmdb::val tx_id_val;


                // process other values for the same key
                while (cr.get(key_to_find, tx_id_val, MDB_NEXT))
                {
                    cout << key_val_to_str(key_to_find, tx_id_val) << endl;
                }

                cr.close();
                rtxn.abort();

            }
            catch (lmdb::error& e)
            {
                cerr << e.what() << endl;
            }
        }

        static uint64_t
        get_blockchain_height(string blk_path = "/home/mwo/.blockchain/lmdb")
        {
            uint64_t height {0};

            try
            {
                auto env = lmdb::env::create();
                env.set_mapsize(DEFAULT_MAPSIZE * 3);
                env.set_max_dbs(20);
                env.open(blk_path.c_str(), MDB_CREATE, 0664);

                auto rtxn = lmdb::txn::begin(env, nullptr, MDB_RDONLY);
                auto rdbi = lmdb::dbi::open(rtxn, "blocks");

                MDB_stat stats = rdbi.stat(rtxn);

                height = static_cast<uint64_t>(stats.ms_entries);

                rtxn.abort();
            }
            catch (lmdb::error& e)
            {
                cerr << e.what() << endl;
                return height;
            }
            catch (exception& e)
            {
                cerr << e.what() << endl;
                return height;
            }

            //cout << height << endl;

            return height;

        }

        static bool
        get_tx_with_tx_id(uint64_t tx_id,
                          transaction& tx,
                          crypto::hash& tx_hash,
                          string blk_path = "/home/mwo/.bitmonero/lmdb")
        {

            try
            {
                auto env = lmdb::env::create();
                env.set_mapsize(DEFAULT_MAPSIZE * 3);
                env.set_max_dbs(20);
                env.open(blk_path.c_str(), MDB_CREATE, 0664);

                auto rtxn = lmdb::txn::begin(env, nullptr, MDB_RDONLY);
                auto rdbi = lmdb::dbi::open(rtxn, "txs");

                lmdb::val tx_id_val {static_cast<void*>(&tx_id), sizeof(tx_id)};

                lmdb::val result_val;

                rdbi.get(rtxn, tx_id_val, result_val);

                string tx_blob {result_val.data(), result_val.size()};

                crypto::hash tx_prefix_hash;

                if (!parse_and_validate_tx_from_blob(tx_blob, tx, tx_hash, tx_prefix_hash))
                {
                    cerr << "Failed to parse transaction from blob" << endl;
                    rtxn.abort();
                    return false;
                }

                (void) tx_prefix_hash; // not using this.

                rtxn.abort();
            }
            catch (lmdb::error& e)
            {
                cerr << e.what() << endl;
                return false;
            }
            catch (exception& e)
            {
                cerr << e.what() << endl;
                return false;
            }

            return true;
        }

        string
        key_val_to_str(const lmdb::val& key, const lmdb::val& val)
        {
            return "key: "     + string(key.data(), key.size())
                   + ", val: " + string(val.data(), val.size());
        }


        template<typename T>
        inline lmdb::val
        pod_to_val(T _pod)
        {
            return lmdb::val {static_cast<void*>(&_pod), sizeof(_pod)};
        }




    };

}

#endif //XMRLMDBCPP_MYLMDB_H
