//
// Created by 张艺文 on 19.6.3.
//

#include "persistent_allocator.h"

namespace rocksdb {

PersistentAllocator::PersistentAllocator(const std::string path, uint64_t size) {
    //pmemaddr_ = static_cast<char *>(pmem_map_file(path.c_str(), size, PMEM_FILE_CREATE | PMEM_FILE_EXCL, 0666, &mapped_len_, &is_pmem_));
    //printf("map file at %s\n", path.c_str());
    pmemaddr_ = static_cast<char *>(pmem_map_file(path.c_str(), size, PMEM_FILE_CREATE, 0666, &mapped_len_, &is_pmem_));

    if (pmemaddr_ == NULL) {
        printf("PersistentAllocator: map error\n");
        exit(-1);
    }
    /*if(is_pmem_){
        printf("is pmem\n");
    }else{
        printf("is not pmem\n");
    }*/
    capacity_ = size;
    cur_index_ = pmemaddr_;
}

PersistentAllocator::~PersistentAllocator() {
    pmem_unmap(pmemaddr_, mapped_len_);
}

char* PersistentAllocator::Allocate(size_t bytes) {
    char *result = cur_index_;
    cur_index_ += bytes;
    return result;
}

char* PersistentAllocator::AllocateAligned(size_t bytes, size_t huge_page_size) {
    char* result = cur_index_;
    cur_index_ += bytes;
    return result;
}

size_t PersistentAllocator::BlockSize() {
    return 0;
}
}