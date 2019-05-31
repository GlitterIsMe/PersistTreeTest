#pragma once

#include "persistent_skiplist_no_transaction.h"
#include "basic_define.h"
#include "persistent_skip_list.h"
#include <array>
#include <string>
using std::array;
using std::string;
const size_t slots_num = 1025;
class Statistic;
namespace rocksdb {

class PersistentSkiplistWrapper {
    public:
        PersistentSkiplistWrapper();

        ~PersistentSkiplistWrapper();

        size_t Insert(const std::string &key, Statistic& stats);

        size_t Insert(const std::string &key, size_t which, Statistic& stats);

        string Get(const std::string &key);

        string Get(const std::string &key, size_t which);

        void Print();

        void PrintLevelNum();

        void Init(const std::string& path, uint64_t size, int32_t max_height = 12, int32_t branching_factor = 4, size_t key_size = 18, uint64_t opt_num = 0, size_t per_1g_num = 0);

#ifdef CAL_ACCESS_COUNT
        void PrintAccessTime();
#endif

    private:
        PersistentAllocator* allocator_;
        array<Persistent_SkipList*, slots_num> skiplists_;
        size_t key_size_;
    };
}

extern rocksdb::PersistentSkiplistWrapper* skiplist_nvm;
