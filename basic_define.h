#pragma once

#include <iostream>
#include <cstdio>
#include <string>
#include <cstring>
#include <ctime>
#include "test_common.h"
#include "../libpmemobj++/p.hpp"
#include "../libpmemobj++/persistent_ptr.hpp"
#include "../libpmemobj++/transaction.hpp"
#include "../libpmemobj++/pool.hpp"
#include "../libpmemobj++/make_persistent.hpp"
#include "../libpmemobj++/make_persistent_array.hpp"

using namespace pmem::obj;

namespace rocksdb {
    
    static int file_exists(char const *file) {
        return access(file, F_OK);
    }

    const int kMaxHeight = 12;
    
    struct p_string{
        persistent_ptr<char[]> data_;
        p <size_t> size_;

        p_string(pool_base & pop, const std::string &src){
            transaction::run(pop,[&]
            {
                data_ = make_persistent<char[]>(src.size() + 1);
                memcpy(&data_[0], src.c_str(), src.size());
                data_[src.size()] = 0;
                size_ = src.size();
                    //printf("p_string = %s\n", &data_[0]);
            });
        }

        int compare(const std::string &right){
            return strcmp(&data_[0], right.c_str());
        }

        const char *data() {
            return &data_[0];
        }

        size_t size() {
            return size_;
        }
    };
}