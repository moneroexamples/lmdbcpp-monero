#include "src/MicroCore.h"
#include "src/CmdLineOptions.h"
#include "src/mylmdb.h"

#include "ext/format.h"
#include "ext/lmdb++.h"

#include <thread>

using epee::string_tools::pod_to_hex;
using boost::filesystem::path;

using namespace std;

// needed for log system of monero
namespace epee {
    unsigned int g_test_dbg_lock_sleep = 0;
}

static const char* LAST_HEIGHT_FILE = "/home/mwo/.bitmonero/lmdb2/last_height.txt";

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
    auto bc_path_opt  = opts.get_option<string>("bc-path");
    auto testnet_opt  = opts.get_option<bool>("testnet");
    auto search_opt   = opts.get_option<bool>("search");


    bool testnet         = *testnet_opt;
    bool search_enabled  = *search_opt;


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

    uint64_t  start_height = 0UL;

    xmreg::MyLMDB mylmdb {"/home/mwo/.bitmonero/lmdb2"};

    while (true)
    {

        string last_height_str = xmreg::read(LAST_HEIGHT_FILE);

        if (!last_height_str.empty())
        {
            boost::trim(last_height_str);
            start_height = boost::lexical_cast<uint64_t>(last_height_str) + 1;
        }


        // get the current blockchain height. Just to check
        uint64_t height =  xmreg::MyLMDB::get_blockchain_height(blockchain_path.string());

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
                crypto::hash tx_hash = get_transaction_hash(tx);

                if (!mylmdb.write_key_images(tx))
                {
                    cerr << "write_key_images failed in tx " << tx_hash << endl;
                    return 1;
                }

                if (!mylmdb.write_output_public_keys(tx))
                {
                    cerr << "write_output_public_keys failed in tx " << tx_hash << endl;
                    return 1;
                }

                if (!mylmdb.write_tx_public_key(tx))
                {
                    cerr << "write_tx_public_key failed in tx " << tx_hash << endl;
                    return 1;
                }

                if (!mylmdb.write_payment_id(tx))
                {
                    cerr << "write_payment_id failed in tx " << tx_hash << endl;
                    return 1;
                }

                if (!mylmdb.write_encrypted_payment_id(tx))
                {
                    cerr << "write_payment_id failed in tx " << tx_hash << endl;
                    return 1;
                }

            }

            {
                ofstream out_file(LAST_HEIGHT_FILE);
                out_file << blk_height;
            }

        } // for (uint64_t i = start_height; i < height; ++i)


        uint64_t what_to_search {0};

        if (search_enabled)
        {
            cout << "What to search "
                 << "[0 - nothing, 1 - key_image, 2- out_public_key, "
                 << "3 - tx_public_key, 4 - payment id, 5 - encrypted payment id"
                 << endl;
            cout << "Your choise [0-6]: ";
            cin >> what_to_search;
        }


        vector<string> found_txs;

        string to_search;

        switch (what_to_search)
        {
            case 1:
                cout << "Enter key_image to find: "; cin >> to_search;
                cout << "Searching for: <" << to_search << ">" << endl;

                if(!mylmdb.search(to_search, found_txs, "key_images"))
                {
                    cout << " - not found" << endl;
                }

                break;

            case 2:
                cout << "Enter output public_key to find: "; cin >> to_search;
                cout << "Searching for: <" << to_search << ">" << endl;

                if(!mylmdb.search(to_search, found_txs,  "output_public_keys"))
                {
                    cout << " - not found" << endl;
                }

                // now find the amount for this output

                if (!found_txs.empty())
                {
                    uint64_t amount;

                    if (mylmdb.get_output_amount(to_search, amount))
                    {
                        cout << " - amount found for this output: "
                             << XMR_AMOUNT(amount)
                             << endl;
                    }
                }

                break;
            case 3:
                cout << "Enter tx public key to find: "; cin >> to_search;
                cout << "Searching for: <" << to_search << ">" << endl;

                if(!mylmdb.search(to_search, found_txs,  "tx_public_keys"))
                {
                    cout << " - not found" << endl;
                }

                break;
            case 4:
                cout << "Enter tx payment_id to find: "; cin >> to_search;
                cout << "Searching for: <" << to_search << ">" << endl;

                if(!mylmdb.search(to_search, found_txs,  "payments_id"))
                {
                    cout << " - not found" << endl;
                }

                break;
            case 5:
                cout << "Enter encrypted tx payment_id to find: "; cin >> to_search;
                cout << "Searching for: <" << to_search << ">" << endl;

                if(!mylmdb.search(to_search, found_txs,  "encrypted_payments_id"))
                {
                    cout << " - not found" << endl;
                }

                break;
        }

        if (search_enabled)
        {
            cout << "Found " << found_txs.size() << " tx:" << endl;

            for (const string& found_tx: found_txs)
            {
                fmt::print(" - tx hash: {:s}\n", found_tx);
            }
        }
        else
        {
            cout << "Wait for 60 seconds " << flush;

            for (size_t i = 0; i < 20; ++i)
            {
                cout << "." << flush;
                std::this_thread::sleep_for(std::chrono::seconds(3));
            }

        }

        cout << endl;

    } //while (true)

    cout << "Hello, World!" << endl;

    return 0;
}
