#pragma once

#include "persistent_skiplist_no_transaction.h"
#include "basic_define.h"
#include "persistent_skip_list.h"

namespace rocksdb {

class PersistentSkiplistWrapper {
    public:
        PersistentSkiplistWrapper();

        ~PersistentSkiplistWrapper();

        void Insert(const std::string &key);

        void Print();

        void PrintLevelNum();

        void Init(const std::string& path, uint64_t size, int32_t max_height = 12, int32_t branching_factor = 4, size_t key_size = 16, uint64_t opt_num = 0, size_t per_1g_num = 0);

#ifdef CAL_ACCESS_COUNT
        void PrintAccessTime();
#endif

    private:
        PersistentAllocator* allocator_;
        Persistent_SkipList* skiplist_;
        size_t key_size_;
    };
}

extern rocksdb::PersistentSkiplistWrapper* skiplist_nvm;
