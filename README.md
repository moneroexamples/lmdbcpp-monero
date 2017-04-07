# Extract information from Monero's blockchain into custom lmdb databases

Monero is using [lmdb databases](http://symas.com/mdb/) to store its blockchain.

Although one can interact directly with the blockchain to get information
from it using Monero C++ libraries,
searching and retrieving some information maybe be not very efficient.

This in this example, it is shown how to create your own custom lmdb databases
based on information from the blockchain using [lmdb++](https://github.com/bendiken/lmdbxx)
interface to the lmdb database.

## Custom databases

The example creates the following lmdb databases based on the information
available in the blockchain:

- `key_images` - key: input key image as string; value: tx_hash as string
- `tx_public_keys` - key: tx public key as string; value: tx_hash as string
- `payments_id` - key: tx payment id as string; value: tx_hash as string
- `encrypted_payments_id` - key: encrypted tx payment id as string; value: tx_hash as string
- `output_public_keys` - key: output public key as string; value: tx_hash as string
- `output_amounts` - key: output public key as string; value: amount as uint64_t
- `output_info` - key: output timestamp as uint64; value: struct {out_pub_key as public_key,
tx_hash as hash, tx_pub_key as public_key, amount as uint64_t, index_in_tx as uint64_t}

## Prerequisite

Everything here was done and tested using Monero 10.3 on Xubuntu 16.04 x86_64.

Instruction for Monero 10.3 compilation:
 - [Compile Monero 0.9 on Ubuntu 16.04 x64](https://github.com/moneroexamples/compile-monero-09-on-ubuntu-16-04)


## Compile this example
The dependencies are same as those for Monero, so I assume Monero compiles
correctly. If so then to download and compile this example, the following
steps can be executed:

```bash
# download the source code
git clone https://github.com/moneroexamples/lmdbcpp-monero.git

# enter the downloaded sourced code folder
cd lmdbcpp-monero

# create build folder
make build && cd build

# create the makefile
cmake .. 

# altearnatively can use cmake -DMONERO_DIR=/path/to/monero_folder .. 
# if monero is not in ~/monero

# compile
make
```

After successful compilation, executable `xmrlmdbcpp` should be created.
When executed, it will look for the Monero blockchain lmdb database in its
default location, i.e., `~/.bitmonero/lmdb`. Once found, it will start extracting
information, block by block, and constructing the custom database. This
can be time consuming, and the new database can be large. At the moment it is
about 12GB!

Once the database is constructed, the `xmrlmdbcpp`
will run in a loop, with 60s breaks, to keep updating itself as new blocks
are added to the Monero blockchain. There is default 10 blocks of delay.
The reason is to minimize adding orphaned blocks in the custom lmdb database.
The number 10 was chosen as default because this is a default number of blocks
before funds get spendable in Monero.

By default, the custom lmdb database will be located in `~/.bitmonero/lmdb2`
folder.

To run it in the background on a headless server,
execute `xmrlmdbcpp` inside `screen` or `tmux` sessions.

## Program options

```bash
./xmrlmdbcpp -h
xmrlmdbcpp, save key_images and public_keys into a custom lmdb database:
  -h [ --help ] [=arg(=1)] (=0)       produce help message
  -b [ --bc-path ] arg                path to lmdb blockchain
  -n [ --no-confirmations ] arg (=10) no of blocks before they are added to the
                                      custom lmdb
  -t [ --testnet ] [=arg(=1)] (=0)    is the address from testnet network
  -s [ --search ] [=arg(=1)] (=0)     search for tx from user input
```


## Example output

```bash
./xmrlmdbcpp
```

```bash
Blockchain path: "/home/mwo/.bitmonero/lmdb"
2016-May-28 12:59:25.670173 Blockchain initialized. last block: 1056841, d0.h0.m5.s7 time ago, current difficulty: 1471793108
1056842
Current blockchain height:1056842
analyzing blk 1056841/1056842
Wait for 60 seconds ....................
1056842
Current blockchain height:1056842
Wait for 60 seconds ........
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
