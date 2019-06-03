#pragma once
#include "log_batch.h"
#include <condition_variable>
#include <thread>
#include <mutex>
#include <unistd.h>
#include <atomic>
#include "thread_pool.h"
#include "flush_nvm.h"
#include "persistent_allocator.h"

namespace rocksdb {
    static std::queue<FlushNVM> queue_;
	static std::mutex q_mutex_;

    static std::mutex mut;
    static std::condition_variable queue_cond;
    static atomic<bool> is_running_;

    class SkiplistWriteNVM {
	public:
        SkiplistWriteNVM();

        ~SkiplistWriteNVM();

        void Flush(ThreadPool* tpool);

        void Insert(const std::string &key);

        void Put_log(const char *key, size_t size);

        //void Get_log();

		void Realloc();

        void Init(const std::string &path, PersistentSkiplistWrapper* skiplist_nvm, int32_t max_height = 12, int32_t branching_factor_ = 4, uint64_t one_log_size = 1638400, size_t skiplist_max_num = 1, size_t key_size = 16);

#ifndef DRAM_CANCEL_LOG
        void AllocateFile();
#endif
        static void CallThread();
        
    private:
        p<bool> been_inited_;
        // AEP connection or set by background thread
#ifndef DRAM_CANCEL_LOG
        PersistentAllocator *allocator_;
#endif
        Memory_SkipList *skiplist_;
        // only initilized once.
        PersistentSkiplistWrapper* skiplist_nvm_;

        p<int32_t> max_height_;
        p<int32_t> branching_factor_;
        std::atomic<bool> is_full_;

		static size_t skiplist_max_num_;
        static size_t block_skiplist_num_; // later use atomic type

		uint64_t max_mem_skiplist_size_;
		uint64_t cur_mem_skiplist_size_;

        size_t key_size_;
        std::string path_;
        uint32_t skiplist_seq_number_;      // initialize to zero, every skiplist flush to nvm, skiplist_seq_number_ += 1;
	};
}
