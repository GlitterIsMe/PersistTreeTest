#include "log_batch_wrapper.h"

namespace rocksdb {

    size_t SkiplistWriteNVM::skiplist_max_num_ = 1;
    size_t SkiplistWriteNVM::block_skiplist_num_ = 0;

	SkiplistWriteNVM::SkiplistWriteNVM(): been_inited_(false),
		max_mem_skiplist_size_(k64MB), cur_mem_skiplist_size_(0){
        skiplist_ = nullptr;
		is_full_.store(false, std::memory_order_relaxed);

#ifndef DRAM_CANCEL_LOG
        allocator_ = nullptr;
#endif
	}

    /*
    SkiplistWriteNVM::SkiplistWriteNVM(uint64_t max_mem_skiplist_size, uint32_t skiplist_max_num) : been_inited_(false), 
			skiplist_max_num_(skiplist_max_num), max_mem_skiplist_size_(max_mem_skiplist_size), cur_mem_skiplist_size_(0) {
        is_full_.store(false, std::memory_order_relaxed);
    }
    */
    SkiplistWriteNVM::~SkiplistWriteNVM() {
#ifndef DRAM_CANCEL_LOG
        if (allocator_ != nullptr) {
            delete allocator_;
        }
#endif

        if (skiplist_ != nullptr) {
            delete skiplist_;
        }
    }

    void SkiplistWriteNVM::Put_log(const char *key, size_t size)
    {
        //log->Put(key,size);
    }

#ifndef DRAM_CANCEL_LOG
    void SkiplistWriteNVM::AllocateFile() {
        char *p = new char[path_.size() + 9];
        
        skiplist_seq_number_ += 1;
        snprintf(p, path_.size() + 8, "%s%08d", path_.c_str(), skiplist_seq_number_);
        const std::string new_path = p;

        allocator_ = new PersistentAllocator(new_path, max_mem_skiplist_size_ + 1048576);
        delete p;
    }
#endif

    // one_log_size equal to max_mem_skiplist_size_
    void SkiplistWriteNVM::Init(const std::string &path,
            PersistentSkiplistWrapper *skiplist_nvm,
            int32_t max_height,
            int32_t branching_factor,
            uint64_t one_log_size,
            size_t skiplist_max_num,
            size_t key_size)
    {
        skiplist_ = nullptr;
        max_height_ = max_height;
        branching_factor_ = branching_factor;
        skiplist_ = new Memory_SkipList(max_height_, branching_factor_, key_size);
        path_ = path;
        key_size_ = key_size;

#ifndef DRAM_CANCEL_LOG
        // @path: add skiplist_seq_number_ to path.
        AllocateFile();
#endif

        skiplist_nvm_ = skiplist_nvm;    // not sure whether it's ok 

        max_mem_skiplist_size_ = one_log_size;
        skiplist_max_num_ = skiplist_max_num;
        block_skiplist_num_ = 0;
    }

    // call it only when we need realloc a new skiplist in DRAM
    void SkiplistWriteNVM::Realloc() {
        assert(skiplist_ == nullptr);
        //assert(allocator_ == nullptr);
        cur_mem_skiplist_size_ = 0;
        
        skiplist_ = new Memory_SkipList(max_height_, branching_factor_, key_size_);

#ifndef DRAM_CANCEL_LOG
        // new_path: 
        AllocateFile();
#endif
    }

    void SkiplistWriteNVM::CallThread()
    {
        //CZL_PRINT("call thread start!");
        FlushNVM flush;
        is_running_.store(true, std::memory_order_relaxed);

        if (queue_.empty()) {
            std::cout << "error: queue_ is empty!" << std::endl;
            return ;
        }

        while (!queue_.empty()) {
            q_mutex_.lock();
            if (!queue_.empty()) {
                flush = queue_.front();
                queue_.pop();
                block_skiplist_num_ -= 1;
            }
            q_mutex_.unlock();

            if (queue_.size() <= skiplist_max_num_) {
                std::lock_guard<std::mutex> lk(mut);
                queue_cond.notify_one();  // unlock anyone thread(actually only one) waiting for queue_cond
            }
            flush.Run();
    //        CZL_PRINT("deal with next full skiplist in DRAM");
        }
        is_running_.store(false, std::memory_order_relaxed);
        //CZL_PRINT("call thread end!");
    }

    void SkiplistWriteNVM::Flush() {
       // CZL_PRINT("begin to flush! block_skiplist_num_=%u, max_skiplist_num=%u", block_skiplist_num_, skiplist_max_num_);
        q_mutex_.lock();		// lock it, keep it thread safety.

		//	is_full_.store(true, std::memory_order_relaxed);
		// wakeup thread to flush DRAM skiplist to NVM, or queue it into 

#ifndef DRAM_CANCEL_LOG
        FlushNVM tmp_flush_nvm(&skiplist_, skiplist_nvm_, &allocator_);
#else
        FlushNVM tmp_flush_nvm(&skiplist_, skiplist_nvm_);
#endif
		queue_.push(tmp_flush_nvm);             // skiplist_ released by another thread
		skiplist_ = nullptr;

#ifndef DRAM_CANCEL_LOG
        allocator_ = nullptr;
#endif

        block_skiplist_num_ += 1;
		q_mutex_.unlock();

        if (block_skiplist_num_ > skiplist_max_num_) {        // block until ....
            std::unique_lock<std::mutex> lk(mut);
            queue_cond.wait(lk, [this]{ return (queue_.size() <= skiplist_max_num_);});   

            lk.unlock();
        } else {
            if (!is_running_.load(std::memory_order_relaxed)) {
                //wakeup another thread.
            //    CZL_PRINT("ready to wake up thread!");
                tpool->enqueue(CallThread);  // thread number no more than 2
            }
        }
        Realloc();          // new a MemSkiplist
        //CZL_PRINT("end flush!");
    }

    void SkiplistWriteNVM::Insert(const std::string &key) {
        // if you want to use @parameter(BATCH_NUM), please comment next line code
        //log_->Put(key.c_str(), key.size()); // KV log write to AEP directly, avoiding data loss.
#ifndef DRAM_CANCEL_LOG
// 如果没有定义这个就是不写LOG的方案
        char *log = allocator_->Allocate(key.size());
        if (allocator_->is_pmem()){
            pmem_memcpy_persist(log, key.c_str(), key.size());
        }else{
            memcpy(log, key.c_str(), key.size());
            pmem_msync(log, key.size());
        };
#endif
        skiplist_->Insert(key);
		cur_mem_skiplist_size_ += key.size();
        if (cur_mem_skiplist_size_ >= max_mem_skiplist_size_) {
			Flush();
        }
    }
}
