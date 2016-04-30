# Extract information from Monero's blockchain into custom lmdb databases

Monero is using [lmdb databases](http://symas.com/mdb/) to store its blockchain.

Although one can interact directly with the blockchain to get information
from it, searching and retreving some information maybe be not very efficient.

This in this example, it is shown how to create your own custom lmdb databases
based on information from the blockchain using [lmdb++](https://github.com/bendiken/lmdbxx)
interface to the lmdb database.

## Custom databases

The example creates the following lmdb databases based on the information
available in the blockchain:

- `key_images` - key: input key image as string; value: tx_hash as string
- `output_public_keys` - key: output public key as string; value: tx_hash as string
- `output_amounts` - key: output public key as string; value: amount as uint64_t
- `tx_public_keys` - key: tx public key as string; value: tx_hash as string
- `payments_id` - key: tx payment id as string; value: tx_hash as string
- `encrypted_payments_id` - key: encrypted tx payment id as string; value: tx_hash as string

## Prerequisite

Everything here was done and tested using Monero 0.9.4 on
Xubuntu 16.04 x86_64.

Instruction for Monero 0.9 compilation and Monero headers and libraries setup are
as shown here:
 - [Compile Monero 0.9 on Ubuntu 16.04 x64](https://github.com/moneroexamples/compile-monero-09-on-ubuntu-16-04)


## C++ code
The main part of the example is main.cpp.

```c++
int main(int ac, const char* av[])  {


    // get command line options
    xmreg::CmdLineOptions opts {ac, av};

    //
    // *** paring and basing setup skipped to save some space ***
    //


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

    // the directory MUST already exist. Make it manually
    path mylmdb_location  = blockchain_path.parent_path() / path("lmdb2");

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

           // save the height of just analyzed block into the last_height_file
           {
               ofstream out_file(last_height_file.string());
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

    return 0;
}
```



## Compile this example
The dependencies are same as those for Monero, so I assume Monero compiles
correctly. If so then to download and compile this example, the following
steps can be executed:

```bash
# download the source code
git clone https://github.com/moneroexamples/lmdbcpp-monero.git

# enter the downloaded sourced code folder
cd lmdbcpp-monero

# create the makefile
cmake .

# compile
make
```

## Other examples
Other examples can be found on  [github](https://github.com/moneroexamples?tab=repositories).
Please know that some of the examples/repositories are not
finished and may not work as intended.

## How can you help?

Constructive criticism, code and website edits are always good. They can be made through github.

Some Monero are also welcome:
```
48daf1rG3hE1Txapcsxh6WXNe9MLNKtu7W7tKTivtSoVLHErYzvdcpea2nSTgGkz66RFP4GKVAsTV14v6G3oddBTHfxP6tU
```
