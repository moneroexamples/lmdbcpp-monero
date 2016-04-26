#include "src/MicroCore.h"
#include "src/CmdLineOptions.h"

#include "ext/format.h"
#include "ext/lmdb++.h"

using boost::filesystem::path;

using namespace std;

// needed for log system of momero
namespace epee {
    unsigned int g_test_dbg_lock_sleep = 0;
}


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




    /* Create and open the LMDB environment: */
    auto env = lmdb::env::create();
    env.set_mapsize(1UL * 1024UL * 1024UL * 1024UL); /* 1 GiB */
    env.set_max_dbs(2);
    env.open("/tmp", MDB_CREATE, 0664);

    /* Insert some key/value pairs in a write transaction: */
    auto wtxn = lmdb::txn::begin(env);
    auto dbi = lmdb::dbi::open(wtxn, "personal", MDB_DUPSORT | MDB_CREATE);

    dbi.put(wtxn, "username", "jhacker");
    dbi.put(wtxn, "email", "jhacker@example.org");
    dbi.put(wtxn, "email", "mwo@example.org");
    dbi.put(wtxn, "fullname", "Middle name");
    dbi.put(wtxn, "job", "accountant");
    dbi.put(wtxn, "email", "dddd@example.org");
    dbi.put(wtxn, "fullname", "J. Random Hacker");
    dbi.put(wtxn, "job", "programmer");
    dbi.put(wtxn, "fullname", "Test Name");
    wtxn.commit();

    /* Fetch key/value pairs in a read-only transaction: */
    auto rtxn = lmdb::txn::begin(env, nullptr, MDB_RDONLY);
    auto cursor = lmdb::cursor::open(rtxn, dbi);
    std::string key, value;
    while (cursor.get(key, value, MDB_NEXT)) {
        std::printf("key: '%s', value: '%s'\n", key.c_str(), value.c_str());
    }
    cursor.close();
    rtxn.abort();

    lmdb::val key1 {"email"};
    lmdb::val  data2;
    string  data3;


    //
    // get a single value
    //

    rtxn = lmdb::txn::begin(env, nullptr, MDB_RDONLY);
    dbi.get(rtxn, key1, data2);
    rtxn.abort();


    cout << "Found key: " << string(data2.data(), data2.size()) << endl;

    cout << endl;

    //
    // retrieve multiple items for same key
    //

    rtxn = lmdb::txn::begin(env, nullptr, MDB_RDONLY);
    cursor = lmdb::cursor::open(rtxn, dbi);

    lmdb::val key2 {"fullname"}, value2;

    // set cursor the the first item
    cursor.get(key2, value2, MDB_SET);

    // process the first item
    cout << "key2: " << string(key2.data(), key2.size())
         << ", value2: " << string(value2.data(), value2.size()) << endl;

    // process other values for the same key
    while (cursor.get(key2, value2, MDB_NEXT_DUP)) {
        cout << "key2: " << string(key2.data(), key2.size())
             << ", value2: " << string(value2.data(), value2.size()) << endl;
    }

    cursor.close();
    rtxn.abort();


    cout << endl;

    env.close();

    // test access to lmdb of monero


   // test_monero();


    cout << "Hello, World!" << endl;
    return 0;
}