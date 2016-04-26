#include "src/MicroCore.h"
#include "src/CmdLineOptions.h"

#include "ext/format.h"
#include "ext/lmdb++.h"


using epee::string_tools::pod_to_hex;
using boost::filesystem::path;

using namespace std;

// needed for log system of monero
namespace epee {
    unsigned int g_test_dbg_lock_sleep = 0;
}

static const char* LAST_HEIGHT_FILE = "/home/mwo/Desktop/last_height.txt";

int main(int ac, const char* av[])  {


    // get command line options
    xmreg::CmdLineOptions opts {ac, av};

    auto help_opt = opts.get_option<bool>("help");

    // if help was chosen, display help text and finish
    if (*help_opt)
    {
        return 0;
    }

    // get other options
    auto bc_path_opt      = opts.get_option<string>("bc-path");
    auto testnet_opt      = opts.get_option<bool>("testnet");


    bool testnet        = *testnet_opt ;


    path blockchain_path;

    if (!xmreg::get_blockchain_path(bc_path_opt, blockchain_path))
    {
        // if problem obtaining blockchain path, finish.
        return 1;
    }


    fmt::print("Blockchain path: {:s}\n", blockchain_path);

    // enable basic monero log output
    xmreg::enable_monero_log();

    // create instance of our MicroCore
    xmreg::MicroCore mcore;

    // initialize the core using the blockchain path
    if (!mcore.init(blockchain_path.string()))
    {
        cerr << "Error accessing blockchain." << endl;
        return 1;
    }

    // get the high level cryptonote::Blockchain object to interact
    // with the blockchain lmdb database
    cryptonote::Blockchain& core_storage = mcore.get_core();


    // get the current blockchain height. Just to check
    // if it reads ok.
    uint64_t height = core_storage.get_current_blockchain_height();

    cout << "Current blockchain height:" << height << endl;

    //uint64_t  start_height = 1033838UL;
    uint64_t  start_height = 0UL;

    string last_height_str = xmreg::read(LAST_HEIGHT_FILE);

    if (!last_height_str.empty())
    {
        boost::trim(last_height_str);
        start_height = boost::lexical_cast<uint64_t>(last_height_str) - 1;
    }

    cout << start_height << endl;

    /* Create and open the LMDB environment: */
    auto env = lmdb::env::create();
    env.set_mapsize(5UL * 1024UL * 1024UL * 1024UL); /* 5 GiB */
    env.set_max_dbs(2);
    env.open("/home/mwo/Desktop", MDB_CREATE, 0664);

    uint64_t tx_idx {0};


    for (uint64_t blk_height = start_height; blk_height < height; ++blk_height)
    {
        cryptonote::block blk;

        try
        {
            blk = core_storage.get_db().get_block_from_height(blk_height);
        }
        catch (std::exception &e) {
            cerr << e.what() << endl;
            continue;
        }

        // get all transactions in the block found
        // initialize the first list with transaction for solving
        // the block i.e. coinbase.
        list<cryptonote::transaction> txs {blk.miner_tx};
        list<crypto::hash> missed_txs;

        if (!mcore.get_core().get_transactions(blk.tx_hashes, txs, missed_txs))
        {
            cerr << "Cant find transactions in block: " << height << endl;
            return 1;
        }


        /* Insert some key/value pairs in a write transaction: */
        auto wtxn = lmdb::txn::begin(env);
        auto dbi = lmdb::dbi::open(wtxn, "key_images", MDB_CREATE);

        //auto wtxn2 = lmdb::txn::begin(env);
        auto dbi2 = lmdb::dbi::open(wtxn, "public_keys", MDB_CREATE | MDB_DUPSORT | MDB_DUPFIXED);


        for (const cryptonote::transaction& tx : txs)
        {
            crypto::hash tx_hash = cryptonote::get_transaction_hash(tx);

            string tx_hash_str = pod_to_hex(tx_hash);

             vector<cryptonote::txin_to_key> key_images = xmreg::get_key_images(tx);


            vector<pair<cryptonote::txout_to_key, uint64_t>> outputs = xmreg::get_ouputs(tx);

            if (!key_images.empty() && tx_idx % 10 == 0)
            {
                cout << "block_height: " << blk_height
                     << " key images size: " << key_images.size()
                     << endl;
            }

            for (const cryptonote::txin_to_key& key_image: key_images)
            {
                string key_img_str = pod_to_hex(key_image.k_image);

                lmdb::val key   {key_img_str};
                lmdb::val data2 {tx_hash_str};

                dbi.put(wtxn, key, data2);
            }

            for (const pair<cryptonote::txout_to_key, uint64_t>& output: outputs)
            {
                lmdb::val key   {pod_to_hex(output.first.key)};
                lmdb::val data2 {tx_hash_str};

                dbi2.put(wtxn, key, data2);
            }

            ++tx_idx;

        }

        wtxn.commit();
       // wtxn2.commit();

        {
            ofstream out_file(LAST_HEIGHT_FILE);
            out_file << blk_height;
        }



    } // for (uint64_t i = start_height; i < height; ++i)

    /* Insert some key/value pairs in a write transaction: */
    //auto wtxn = lmdb::txn::begin(env);



//    /* Fetch key/value pairs in a read-only transaction: */
//    auto rtxn = lmdb::txn::begin(env, nullptr, MDB_RDONLY);
//    auto dbi = lmdb::dbi::open(rtxn, "key_images");
//    auto cursor = lmdb::cursor::open(rtxn, dbi);
//
//    lmdb::val key ;
//    lmdb::val data2;
//    while (cursor.get(key, data2, MDB_NEXT)) {
//        //std::printf("key: '%s', value: '%s'\n", key.c_str(), value.c_str());
//
//        cout << "key_image: " << string(key.data(), key.size())
//             << " tx_hash: "   <<   string(data2.data(), data2.size())
//             << endl;
//
//
//    }
//    cursor.close();
//    rtxn.abort();

    lmdb::val key  {"f83301b08f23800a928aa50bafaca5e20135b6cd9cb7603585199dcd0d30e9b7"};
    lmdb::val data2 ;

    auto rtxn = lmdb::txn::begin(env, nullptr, MDB_RDONLY);
    auto dbi = lmdb::dbi::open(rtxn, "key_images");

    if (dbi.get(rtxn, key, data2))
    {

        string key_img(key.data(), key.size());
        string tx_hash(data2.data(), data2.size());

        fmt::print("Key image {:s} found in tx {:s}\n", key_img, tx_hash);
    }
    else
    {
        fmt::print("Key image {:s} not found\n", key);
    }

    rtxn.abort();



    cout << "Hello, World!" << endl;
    return 0;
}