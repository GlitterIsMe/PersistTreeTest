#pragma once

#include <iostream>
#include <cstdio>
#include "log_batch.h"
#include "persistent_allocator.h"
#include "persistent_skiplist_wrapper.h"

using namespace std;
namespace rocksdb {

// Background thread flush DRAM skiplist to NVM.
class FlushNVM {
public:
    FlushNVM() {
        skiplist_ = nullptr;
    }

    FlushNVM(const FlushNVM &f) {
        this->skiplist_ = f.skiplist_;
        this->skiplist_nvm_ = f.skiplist_nvm_;
#ifndef DRAM_CANCEL_LOG
        this->allocator_ = f.allocator_;
#endif
    }

#ifndef DRAM_CANCEL_LOG
    FlushNVM(Memory_SkipList **skiplist, PersistentSkiplistWrapper *skiplist_wrapper, PersistentAllocator **allocator) {
        skiplist_ = *skiplist;
        skiplist_nvm_ = skiplist_wrapper;
        allocator_ = *allocator;
    }
#else 
    FlushNVM(Memory_SkipList **skiplist, PersistentSkiplistWrapper *skiplist_wrapper) {
        skiplist_ = *skiplist;
        skiplist_nvm_ = skiplist_wrapper;
    }
#endif
    
    ~FlushNVM() {}

    void Run();

    void PrintKey(std::string &key, uint64_t &last_num, uint64_t &last_seq_num);

private:
    Memory_SkipList *skiplist_;
#ifndef DRAM_CANCEL_LOG
    PersistentAllocator *allocator_;
#endif
    // nvm skiplist address.
    PersistentSkiplistWrapper* skiplist_nvm_;
};

}