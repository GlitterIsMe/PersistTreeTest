#include "persistent_skiplist_wrapper.h"

namespace rocksdb {

    PersistentSkiplistWrapper::PersistentSkiplistWrapper() {
        allocator_ = nullptr;
        skiplist_ = nullptr;
    }

    PersistentSkiplistWrapper::~PersistentSkiplistWrapper() {
        if (skiplist_ != nullptr) {
            delete skiplist_;
        }

        if (allocator_ != nullptr) {
            delete allocator_;
        }
    }

    void PersistentSkiplistWrapper::Init(const std::string& path, uint64_t size, int32_t max_height, int32_t branching_factor, size_t key_size, uint64_t opt_num,size_t per_1g_num) {
        allocator_ = new PersistentAllocator(path, size);
        skiplist_ = new Persistent_SkipList(allocator_, max_height, branching_factor, key_size, opt_num ,per_1g_num);
        key_size_ = key_size;
    }

    void PersistentSkiplistWrapper::Insert(const std::string &key) {
        skiplist_->Insert(key);
    }

    void PersistentSkiplistWrapper::Print() {
        skiplist_->Print();
    }

    void PersistentSkiplistWrapper::PrintLevelNum() {
        skiplist_->PrintLevelNum();
    }

#ifdef CAL_ACCESS_COUNT
    void PersistentSkiplistWrapper::PrintAccessTime() {
        skiplist_->PrintAccessTime();
    }
#endif
}

rocksdb::PersistentSkiplistWrapper *skiplist_nvm;
