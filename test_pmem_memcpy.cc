#include <iostream>
#include <string>
#include "libpmem.h"
#include "persistent_skiplist_no_transaction.h"
#define PATH "/home/czl/pmem0/skiplist_test_dir/persistent_skiplist"

using namespace::std;

struct TestNode{
    TestNode(const char* key){
        key_ = key;
    }
    string key_;
};

int main(){
    rocksdb::PersistentAllocator* allocator_=new rocksdb::PersistentAllocator(PATH, 50);
    string key = "77777";
    char *mem = allocator_->Allocate(sizeof(TestNode) );
    char *pkey = allocator_->Allocate(key.size());
    pmem_memcpy_persist(pkey, key.c_str(), key.size());
    TestNode *a = new (mem) TestNode(pkey);
    cout<<a->key_<<endl;
}