#pragma once

#include <cstring>
#include <string>
#include "libpmem.h"
#include "random.h"
#include "nvm_node.h"
#include "persistent_skiplist_no_transaction.h"
class Statistic;

namespace rocksdb {

class Persistent_SkipList {
    public:
        explicit Persistent_SkipList(PersistentAllocator* allocator, int32_t max_height = 12, int32_t branching_factor = 4, size_t key_size = 16 ,uint64_t opt_num = 0, size_t per_1g_num = 0);

        ~Persistent_SkipList() {
#ifdef CAL_ACCESS_COUNT
            // PrintAccessTime();
#endif
        }

        void Insert(const std::string& key, Statistic& stats);

        std::string Get(const std::string& key);

        void PrintKey(const char *str,uint64_t &last_num, uint64_t &last_seq_num) const;

        void Print() const;

        void PrintLevelNum() const;
#ifdef CAL_ACCESS_COUNT
        void PrintAccessTime();
#endif

    private:
        PersistentAllocator* allocator_;
        Node* head_;
        Node** prev_;
        uint32_t prev_height_;
        uint16_t kMaxHeight_;
        uint16_t kBranching_;
        uint32_t kScaledInverseBranching_;

#ifdef CAL_ACCESS_COUNT
        uint64_t cnt_time_;
        uint64_t all_cnt_;
        uint64_t head_cnt_;
        uint64_t suit_cnt_;
        uint64_t opt_num_;
        uint64_t opt_1g_num_;
        size_t per_1g_num_;
#endif

        uint16_t max_height_;

        size_t key_size_;

        inline int GetMaxHeight() const {
            return max_height_;
        }

        Node* NewNode(const std::string &key, int height);

        int RandomHeight();

        bool Equal(const char *a, const char *b) {
            return strcmp(a, b) == 0;
        }

        bool LessThan(const char *a, const char *b) {
            return strcmp(a, b) < 0;
        }

        bool KeyIsAfterNode(const std::string& key, Node* n, Statistic &stat) const;
        
        int CompareKeyAndNode(const std::string& key, Node* n);

        Node* FindGreaterOrEqual(const std::string& key) const;

        void FindLessThan(const std::string& key, Node** prev = nullptr);

        void FindNextNode(const std::string &key, Node** prev, Statistic &stat);
    };
}