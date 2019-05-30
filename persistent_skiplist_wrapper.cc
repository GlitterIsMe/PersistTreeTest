#include "persistent_skiplist_wrapper.h"
#include <city.h>
#include <string>
#include "statistic.h"

namespace rocksdb {

    PersistentSkiplistWrapper::PersistentSkiplistWrapper() {
        allocator_ = nullptr;
        for(int i = 0; i < slots_num; i++){
            skiplists_[i] = nullptr;
        }
    }

    PersistentSkiplistWrapper::~PersistentSkiplistWrapper() {
        for(auto list : skiplists_){
            delete list;
        }

        if (allocator_ != nullptr) {
            delete allocator_;
        }
    }

    void PersistentSkiplistWrapper::Init(const std::string& path, uint64_t size, int32_t max_height, int32_t branching_factor, size_t key_size, uint64_t opt_num,size_t per_1g_num) {
        allocator_ = new PersistentAllocator(path, size);
        for(auto &list : skiplists_){
            list = new Persistent_SkipList(allocator_, max_height, branching_factor, key_size, opt_num ,per_1g_num);
        }
        key_size_ = key_size;
    }

    void PersistentSkiplistWrapper::Insert(const std::string &key, Statistic& stats) {
        uint64_t hash = CityHash64(key.c_str(), key_size_);
        size_t slot = hash % slots_num;
        skiplists_[slot]->Insert(key);
    }

    void PersistentSkiplistWrapper::Print() {
        for(auto list : skiplists_){
            list->Print();
        }

    }

    void PersistentSkiplistWrapper::PrintLevelNum() {
        for(auto list : skiplists_){
            list->PrintLevelNum();
        }

    }

#ifdef CAL_ACCESS_COUNT
    void PersistentSkiplistWrapper::PrintAccessTime() {
        for(auto list : skiplists_){
            list->PrintAccessTime();
        }
    }
#endif
}

rocksdb::PersistentSkiplistWrapper *skiplist_nvm;
