

#include "ext/lmdb++.h"

#include <iostream>

using namespace std;


const char* const LMDB_SPENT_KEYS = "spent_keys";


void test_monero()
{
    auto env = lmdb::env::create();
    env.set_max_dbs(20);

    env.open("/home/mwo/.bitmonero/lmdb", MDB_RDONLY, 0664);

    auto rtxn = lmdb::txn::begin(env, nullptr, MDB_RDONLY);

    auto dbi = lmdb::dbi::open(rtxn, LMDB_SPENT_KEYS);

    /* Fetch key/value pairs in a read-only transaction: */
   // auto rtxn = lmdb::txn::begin(env, nullptr, MDB_RDONLY);
    auto cursor = lmdb::cursor::open(rtxn, dbi);
    std::string key, value;
    while (cursor.get(key, value, MDB_NEXT)) {
        std::printf("key: '%s', value: '%s'\n", key.c_str(), value.c_str());
    }

    cursor.close();
    rtxn.abort();
}

int main() {

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


    test_monero();


    cout << "Hello, World!" << endl;
    return 0;
}