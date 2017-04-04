#include "src/MicroCore.h"
#include "src/CmdLineOptions.h"
#include "src/mylmdb.h"

#include "ext/format.h"
#include "ext/lmdb++.h"

#include <thread>

using epee::string_tools::pod_to_hex;
using boost::filesystem::path;

using namespace std;

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
    auto bc_path_opt          = opts.get_option<string>("bc-path");
    auto testnet_opt          = opts.get_option<bool>("testnet");
    auto search_opt           = opts.get_option<bool>("search");
    auto no_confirmations_opt = opts.get_option<uint64_t>("no-confirmations");


    bool testnet               = *testnet_opt;
    bool search_enabled        = *search_opt;
    uint64_t  no_confirmations = *no_confirmations_opt;

    path blockchain_path;

    if (!xmreg::get_blockchain_path(bc_path_opt, blockchain_path, testnet))
    {
        // if problem obtaining blockchain path, finish.
        return 1;
    }


    fmt::print("Blockchain path: {:s}\n", blockchain_path);

    // set  monero log output level
    uint32_t log_level = 0;
    mlog_configure("", true);

    // create instance of our MicroCore
    // and make pointer to the Blockchain
    xmreg::MicroCore mcore;
    cryptonote::Blockchain* core_storage;

    // initialize mcore and core_storage
    if (!xmreg::init_blockchain(blockchain_path.string(),
                                mcore, core_storage))
    {
        cerr << "Error accessing blockchain." << endl;
        return EXIT_FAILURE;
    }

    uint64_t  start_height = 0UL;

    // the directory MUST already exist. Make it manually
    path mylmdb_location  = blockchain_path.parent_path() / path("lmdb2");

    // if the lmdb2 folder does not exist, create it
    if (!boost::filesystem::exists(mylmdb_location))
    {
        if (boost::filesystem::create_directory(mylmdb_location))
        {
            cout << "Folder " << mylmdb_location << "created" << endl;
        }
        else
        {
            cerr << "Cant created folder: " << mylmdb_location << endl;
            return EXIT_FAILURE;
        }
    }

    // file in which last number of block analyzed is will be stored
    path last_height_file =  mylmdb_location / path("last_height.txt");

    // instance of MyLMDB class that interacts with the custom database
    xmreg::MyLMDB mylmdb {mylmdb_location.string()};


    // the infinte loop that first reads all tx in the blockchain
    // and then makes an interation every 60s to process new
    // blockchain in the real time as they come
    while (true)
    {

        string last_height_str = xmreg::read(last_height_file.string());

        if (!last_height_str.empty())
        {
            boost::trim(last_height_str);
            start_height = boost::lexical_cast<uint64_t>(last_height_str) + 1;
        }


        // get the current blockchain height. Just to check
        uint64_t height =  xmreg::MyLMDB::get_blockchain_height(blockchain_path.string());

        cout << "Current blockchain height: " << height << endl;


        for (uint64_t blk_height = start_height; blk_height < height - no_confirmations; ++blk_height)
        {
            cryptonote::block blk;

            try
            {
                blk = core_storage->get_db().get_block_from_height(blk_height);
            }
            catch (std::exception &e)
            {
                cout << e.what()
                     << "Wait 60 seconds and try again ";

                for (size_t i = 0; i < 20; ++i)
                {
                    cout << "." << flush;
                    std::this_thread::sleep_for(std::chrono::seconds(3));
                }

                cout << endl;

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

            if (blk_height % 1 == 0)
            {
                fmt::print("analyzing blk {:d}/{:d}\n",
                           blk_height, height);
            }

            if (!mylmdb.begin_txn())
            {
                cerr << "begin_txn failed" << endl;
                return 1;
            }
            for (const cryptonote::transaction& tx : txs)
            {
                crypto::hash tx_hash = get_transaction_hash(tx);

                if (!mylmdb.write_key_images(tx))
                {
                    cerr << "write_key_images failed in tx " << tx_hash << endl;
                    return 1;
                }

                if (!mylmdb.write_output_public_keys(tx, blk))
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
            if (!mylmdb.end_txn())
            {
                cerr << "end_txn failed" << endl;
                return 1;
            }

            // save the height of just analyzed block into the last_height_file
            {
                ofstream out_file(last_height_file.string());
                out_file << blk_height;
            }

        } // for (uint64_t i = start_height; i < height; ++i)

        if (!mylmdb.sync())
        {
            cerr << "env_sync failed" << endl;
            return 1;
        }

        uint64_t what_to_search {0};

        if (search_enabled)
        {
            cout << "What to search "
                 << "[0 - nothing, 1 - key_image, 2- out_public_key, "
                 << "3 - tx_public_key, 4 - payment id, 5 - encrypted payment id, "
                 << "6 - output info, 7 - block height based on timestamp, "
                 << "8 - block heights withing timestamp range\""
                 << endl;
            cout << "Your choise [0-8]: ";
            cin >> what_to_search;
        }


        vector<string> found_txs;

        string to_search;

        switch (what_to_search)
        {
            case 1:
                cout << "Enter key_image to find: "; cin >> to_search;
                cout << "Searching for: <" << to_search << ">" << endl;

                if(!mylmdb.search(to_search, found_txs, mylmdb.D_key_images))
                {
                    cout << " - not found" << endl;
                }

                break;

            case 2:
                cout << "Enter output public_key to find: "; cin >> to_search;
                cout << "Searching for: <" << to_search << ">" << endl;

                if(!mylmdb.search(to_search, found_txs,  mylmdb.D_output_public_keys))
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

                if(!mylmdb.search(to_search, found_txs,  mylmdb.D_tx_public_keys))
                {
                    cout << " - not found" << endl;
                }

                break;

            case 4:
                cout << "Enter tx payment_id to find: "; cin >> to_search;
                cout << "Searching for: <" << to_search << ">" << endl;

                if(!mylmdb.search(to_search, found_txs,  mylmdb.D_payments_id))
                {
                    cout << " - not found" << endl;
                }

                break;
            case 5:
                cout << "Enter encrypted tx payment_id to find: "; cin >> to_search;
                cout << "Searching for: <" << to_search << ">" << endl;

                if(!mylmdb.search(to_search, found_txs,  mylmdb.D_encrypted_payments_id))
                {
                    cout << " - not found" << endl;
                }

                break;

            case 6:
            {
                cout << "Enter output (i.e., block) timestamp to find: ";
                cin >> to_search;
                cout << "Searching for: <" << to_search << ">" << endl;

                uint64_t out_timestamp = boost::lexical_cast<uint64_t>(to_search);

                vector<xmreg::output_info> out_infos;

                if (mylmdb.get_output_info(out_timestamp, out_infos)) {
                    cout << " - following outputs were found:" << endl;

                    for (const auto &out_info: out_infos)
                    {
                        cout << "   - " << out_info << endl;
                    }
                }
                else
                {
                    cout << "No outputs with timestamp of " << out_timestamp
                         << "were found." << endl;
                }

                break;
            }
            case 7:
            {
                cout << "Enter block timestamp to find: ";
                cin >> to_search;
                cout << "Searching for: <" << to_search << ">" << endl;

                // block timestamps are same as outputs obviously, so we
                // can use this info for this purpose
                uint64_t blk_timestamp = boost::lexical_cast<uint64_t>(to_search);

                vector<xmreg::output_info> out_infos2;

                if (mylmdb.get_output_info(blk_timestamp, out_infos2))
                {
                    // since many outputs can be in a single block
                    // just get the first one to obtained its block

                    uint64_t found_blk_height = core_storage->get_db()
                            .get_tx_block_height(out_infos2.at(0).tx_hash);

                    cout << " - following timestamp was found for block no:"
                         << found_blk_height
                         << endl;
                }
                else
                {
                    cout << "No block with timestamp of " << blk_timestamp
                         << "was found." << endl;
                }
                break;
            }
            case 8:
            {
                cout << "Enter start block timestamp to find: ";
                cin >> to_search;

                string to_search_end;
                cout << "Enter end block timestamp to find: ";
                cin >> to_search_end;

                // block timestamps are same as outputs obviously, so we
                // can use this info for this purpose
                uint64_t blk_timestamp_start = boost::lexical_cast<uint64_t>(to_search);
                uint64_t blk_timestamp_end   = boost::lexical_cast<uint64_t>(to_search_end);


                vector<pair<uint64_t, xmreg::output_info>> out_infos2;

                if (mylmdb.get_output_info_range(blk_timestamp_start, blk_timestamp_end, out_infos2))
                {
                    // since many outputs can be in a single block
                    // just get the first one to obtained its block

                    for (const auto &out_info: out_infos2)
                    {
                        cout << "   - " << out_info.second << endl;
                    }
                }
            }
        } // switch (what_to_search)

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


    return EXIT_SUCCESS;
}
