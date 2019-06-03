//
// Created by 张艺文 on 19.6.3.
//
#pragma once

#include <cstring>
#include <string>
#include <exception>
#include "libpmem.h"
#include "random.h"
#include "nvm_node.h"
#include "error.h"
using std::exception;
namespace rocksdb {

class PersistentAllocator {
public:
    PersistentAllocator(const std::string path, size_t size);

    ~PersistentAllocator();

    char* Allocate(size_t bytes);

    char* AllocateAligned(size_t bytes, size_t huge_page_size = 0);

    size_t BlockSize();

    bool is_pmem(){
        return is_pmem_;
    }

private:
    char* pmemaddr_;
    size_t mapped_len_;
    uint64_t capacity_;
    int is_pmem_;
    char* cur_index_;
};
}
