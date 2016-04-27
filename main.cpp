#include "src/MicroCore.h"
#include "src/CmdLineOptions.h"
#include "src/mylmdb.h"

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


    xmreg::MyLMDB mylmdb {"/home/mwo/Desktop"};



    //uint64_t  start_height = 1033838UL;
    uint64_t  start_height = 0UL;

    string last_height_str = xmreg::read(LAST_HEIGHT_FILE);

    if (!last_height_str.empty())
    {
        boost::trim(last_height_str);
        start_height = boost::lexical_cast<uint64_t>(last_height_str);
    }


    // get the current blockchain height. Just to check
    //uint64_t height =  xmreg::MyLMDB::get_blockchain_height(blockchain_path.string());
    uint64_t height =  xmreg::MyLMDB::get_blockchain_height(blockchain_path.string());
    //uint64_t height = core_storage.get_current_blockchain_height();
    cout << "Current blockchain height:" << height << endl;


    for (uint64_t blk_height = start_height; blk_height < height; ++blk_height)
    {
        cryptonote::block blk;

        try
        {
            blk = core_storage.get_db().get_block_from_height(blk_height);
        }
        catch (std::exception &e)
        {
            cerr << e.what() << endl;
            return 1;
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

        if (blk_height % 1 == 0)
        {
            fmt::print("analyzing blk {:d}/{:d}\n", blk_height, height);
        }


        for (const cryptonote::transaction& tx : txs)
        {
            if (!mylmdb.write_key_images(tx))
            {
                cerr << "write_key_images failed in blk " << blk_height << endl;
                return 1;
            }

            if (!mylmdb.write_public_keys(tx))
            {
                cerr << "write_public_keys failed in blk " << blk_height << endl;
                return 1;
            }
        }

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

//    auto rtxn = lmdb::txn::begin(env, nullptr, MDB_RDONLY);
//    auto dbi = lmdb::dbi::open(rtxn, "key_images");
//
//    if (dbi.get(rtxn, key, data2))
//    {
//
//        string key_img(key.data(), key.size());
//        string tx_hash(data2.data(), data2.size());
//
//        fmt::print("Key image {:s} found in tx {:s}\n", key_img, tx_hash);
//    }
//    else
//    {
//        fmt::print("Key image {:s} not found\n", key);
//    }
//
//    rtxn.abort();



    cout << "Hello, World!" << endl;
    return 0;
}