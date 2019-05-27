#pragma once
#include <cstring>
#include <string>
#include <stdio.h>
#include <ctime>
#include <atomic>
#include <queue>
#include <algorithm>
#include <mutex>
#include "thread_pool.h"
#include "random.h"
#include "test_common.h"
//#include "nvm_node.h"
#include "libpmemobj++/p.hpp"
#include "libpmemobj++/persistent_ptr.hpp"
#include "libpmemobj++/transaction.hpp"
#include "libpmemobj++/pool.hpp"
#include "libpmemobj++/make_persistent.hpp"
#include "libpmemobj++/make_persistent_array.hpp"

using namespace pmem::obj;

namespace rocksdb {

extern const uint64_t k64MB;

char* EncodeVarint32(char* dst, uint32_t v);

void PutVarint32(std::string* dst, uint32_t v);

void PutLengthPrefixedSlice(std::string* dst, const std::string& value);

struct DramNode {
    explicit DramNode() {};

    ~DramNode() { delete key_; }

    DramNode *Next(int n) {
        assert(n >= 0);
        return next_[n];
    }

    void SetNext(int n, DramNode *next) {
        assert(n >= 0);
        next_[n] = next;
    };
    char*  key_;
    size_t  key_size;
    uint16_t height;
    DramNode *next_[0];
};

struct ENTRY {
    persistent_ptr<char[]> data;
    p<uint64_t> size;
    persistent_ptr<ENTRY> next;
};

struct LOG_ENTRY {
    persistent_ptr<char[]> data;
    p<uint64_t> w_oft;
    p<uint64_t> size;
    persistent_ptr<LOG_ENTRY> next;
};

class AEP_LOG {
    public:
        AEP_LOG(pool_base &pop,uint64_t entry_size);

        ~AEP_LOG(){};

        persistent_ptr<LOG_ENTRY> NewEntry();

        uint64_t Put(const char *key, size_t size) ;

        void Get();

        void Delete();  // delete all log from NVM.(connected with single skiplist)

    private:
        pool_base& pop_;
        persistent_ptr<LOG_ENTRY> head;
        persistent_ptr<LOG_ENTRY> tail;
        p<uint64_t> entry_size_;
};

class Memory_SkipList {
    public:
        explicit Memory_SkipList( int32_t max_height = 12, int32_t branching_factor = 4, size_t key_size = 16);

        ~Memory_SkipList() ;

        void Insert(const std::string &key);

        DramNode *HeadNode() { return head_; }
        //bool Contains(const char *key);

    private:
        DramNode *head_;
        DramNode **prev_;
        uint32_t prev_height_;
        uint16_t kMaxHeight_;
        uint16_t kBranching_;
        uint32_t kScaledInverseBranching_;

        size_t key_size_;

        uint16_t max_height_;

        inline int GetMaxHeight() const {
            return max_height_;
        }

        DramNode *NewNode(const std::string &key, int height);

        int RandomHeight();

        bool Equal(const char *a, const char *b) {
            return strcmp(a, b) == 0;
        }

        bool LessThan(const char *a, const char *b) {
            return strcmp(a, b) < 0;
        }

        bool KeyIsAfterNode(const std::string &key, DramNode *n) const;

        int CompareKeyAndNode(const std::string& key, DramNode* n);

        // DramNode *FindGreaterOrEqual(const std::string &key) const;

        DramNode *FindLessThan(const std::string &key, DramNode **prev = nullptr) const;

        void FindNextNode(const std::string &key, DramNode** prev);
    };
}

// thread pool
extern ThreadPool *tpool;       // constructor parameter list initialize it.
