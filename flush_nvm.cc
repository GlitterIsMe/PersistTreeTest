#include "flush_nvm.h"
#include "statistic.h"
namespace rocksdb {
    void FlushNVM::PrintKey(std::string &key, uint64_t &last_num, uint64_t &last_seq_num)
    {
        uint64_t num = 0;
        uint64_t seq_num = 0;
        const char *str = key.c_str();
        for (int i = 0; i < 8; i++) {
            num = (str[i] - '0') + 10 * num;
        }
        
        for (int i = 8; i < 16; i++) {
            seq_num = (str[i] - '0') + 10 * seq_num;
        }

        bool res = ((last_num < num) || ((last_num == num) && (last_seq_num < seq_num)));
        if (res == false) {
            printf("----------------error: DRAM skiplist is not in order!!! -----------------------------\n\n\n\n");
        }

        last_num = num;
        last_seq_num = seq_num;

        // printf("%llu - %llu  ", num, seq_num);
    }

    void FlushNVM::Run()
    {
        assert(skiplist_ != nullptr);
#ifndef DRAM_CANCEL_LOG
        assert(allocator_ != nullptr);
#endif

        DramNode* head = skiplist_->HeadNode();
        DramNode* start = head->Next(0);
        uint64_t last_num = 0;
        uint64_t last_seq_num = 0;
        assert(start != nullptr);
        Statistic stats;

        while (start != nullptr) {
            std::string key = start->key_;     // here copy data process, maybe waste more time
            // insert to NVM skiplist
            PrintKey(key, last_num, last_seq_num);
            skiplist_nvm_->Insert(key, stats);
            start = start->Next(0);
        }
        // int level[20] = 0;
        delete skiplist_;

#ifndef DRAM_CANCEL_LOG
        // unmap NVM log file
        delete allocator_;
#endif
        
    }
}