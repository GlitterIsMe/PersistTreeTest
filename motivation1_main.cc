#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <optional>
#include <chrono>
#include <iomanip>

#include "test_common.h"
#include "random.h"
#include "persistent_skiplist_wrapper.h"
#include "log_batch_wrapper.h"
#include "statistic.h"
#include "thread_pool.h"

using namespace std;

const string NVM_PATH = "/home/czl/pmem0/skiplist_test_dir/persistent_skiplist2";
const string NVM_LOG_PATH = "/home/czl/pmem0/skiplist_test_dir/dram_skiplist";
const size_t NVM_SIZE = 200 * (1ULL << 30);             // 200GB

enum EngineType {
    SKIPLIST,
    BTREE,
    BPLUSTREE,
    DEFAULT,
};

struct Config {
    /* parameter:
 *    @using_existing_data: whether using old data or not(read operations must rely on existing data on NVM)
 *    @test_type(0: write NVM directly, 1: write DRAM and thread flush data to NVM)
 *    @value_size
 *    @ops_type: Read=0, Write=1
 *    @ops_num: data_size = @value_size * @ops_num
 *    @mem_skiplist_size: limit skiplist size(MB) in DRAM. It also used to initilize DRAM skiplist
 *    @skiplist_max_num: allow maximum number skiplist not dealt by background thread.
 *    @storage_type: 0:skiplist, 1:B-Tree, 2:B+ Tree        // To Be Done
 */
    const string path_;
    const string log_path_;
    const uint64_t nvm_size_;

    const size_t key_size_;
    const size_t value_size_;
    const uint64_t ops_num_;
    const uint64_t get_after_insert_;

    // by MB
    const size_t mem_skiplist_size_;
    const size_t skiplist_max_num_;

    const bool using_existing_data_;
    const bool nvm_direct_;
    const EngineType enging_;

    Config(string path_dir) :
            path_(path_dir),
            log_path_(path_dir),
            nvm_size_(1ul * 1024 * 1024 * 1024),
            key_size_(16),
            value_size_(1024),
            ops_num_(1000),
            get_after_insert_(0),
            mem_skiplist_size_(64ul * 1024 * 1024),
            skiplist_max_num_(1),
            using_existing_data_(false),
            nvm_direct_(false),
            enging_(DEFAULT) {}

    Config(string path_dir, string log_path_dir, uint64_t nvm_size,
           size_t key_size, size_t value_size, uint64_t ops_num, uint64_t get_after_insert,
           size_t mem_skiplist_size, size_t skiplist_max_num,
           bool using_existing_data, bool nvm_direct, EngineType type) :
            path_(std::move(path_dir)),
            log_path_(std::move(log_path_dir)),
            nvm_size_(nvm_size),
            value_size_(value_size),
            key_size_(key_size),
            ops_num_(ops_num),
            get_after_insert_(get_after_insert),
            mem_skiplist_size_(mem_skiplist_size * 1024 * 1024),
            skiplist_max_num_(skiplist_max_num),
            using_existing_data_(using_existing_data),
            nvm_direct_(nvm_direct),
            enging_(type) {}

    Config(Config &config)
            : path_(config.path_),
              log_path_(config.log_path_),
              nvm_size_(config.nvm_size_),
              value_size_(config.value_size_),
              key_size_(config.key_size_),
              ops_num_(config.ops_num_),
              get_after_insert_(config.get_after_insert_),
              mem_skiplist_size_(config.mem_skiplist_size_),
              skiplist_max_num_(config.skiplist_max_num_),
              using_existing_data_(config.using_existing_data_),
              nvm_direct_(config.nvm_direct_),
              enging_(config.enging_) {}
};

class RunBenchmark {
public:
    RunBenchmark(Config &&para_config) : config(para_config) {
        output_interval = (1024 * 1024) / config.value_size_ * 1024 - 1;
        skiplist_nvm = new rocksdb::PersistentSkiplistWrapper();
        skiplist_nvm->Init(config.path_, config.nvm_size_, 20, 2, config.key_size_, config.ops_num_, output_interval);

        CZL_PRINT("prepare to create PATH_LOG:%s", NVM_LOG_PATH.c_str());
        // KV write DRAM skiplist, and then background thread write it to NVM
        if (!config.nvm_direct_) {
            tpool = new ThreadPool(2);          // create threadpool(thread number = 2)
            skiplist_dram = new rocksdb::SkiplistWriteNVM(tpool);
            skiplist_dram->Init(
                    config.log_path_,
                    skiplist_nvm,
                    12,
                    4,
                    config.mem_skiplist_size_,
                    config.skiplist_max_num_,
                    config.key_size_);
        }
    }

    ~RunBenchmark() {
        delete tpool;
        delete skiplist_dram;
        delete skiplist_nvm;
    }

    void Run() {
        start_ = chrono::high_resolution_clock::now();
        if (!config.nvm_direct_) {
            // create new thread, beginning with write_to_dram()
            write_to_dram();
            end_ = chrono::high_resolution_clock::now();

            dram_not_flush_print();
            skiplist_dram->Flush();     // write DRAM data to NVM.
            end_ = chrono::high_resolution_clock::now();
        } else {
            write_to_nvm();
            end_ = chrono::high_resolution_clock::now();
            //skiplist_nvm->PrintLevelNum();
            skiplist_nvm->Print();
        }
        if (!config.nvm_direct_) {
            dram_print();
        } else {
            nvm_print();
        }
    }

private:
    void write_to_nvm(bool single = false) {
        CZL_PRINT("start!");
        auto rnd = rocksdb::Random::GetTLSInstance();
        char buf[config.key_size_ + config.value_size_];
        memset(buf, 0, sizeof(buf));
        string value(config.value_size_, 'v');
        auto start = chrono::high_resolution_clock::now();
        auto last_time = start;
        Statistic stats;
        /*vector<vector<string>*> ops_key;
        for(size_t i = 0; i < 1024; i++){
            ops_key.push_back(new vector<string>);
        }*/
        for (uint64_t i = 1; i <= config.ops_num_; i++) {
            uint64_t number = rnd->Next() % config.ops_num_;
            snprintf(buf, sizeof(buf), "%08llu%010llu%s", number, i, value.c_str());
            string data(buf);
            string key(buf, 18);
            size_t pos;
            if (single) {
                pos = skiplist_nvm->Insert(data, 0, stats);
            } else {
                pos = skiplist_nvm->Insert(data, stats);
            }
            //ops_key[pos]->push_back(std::move(key));

#ifdef EVERY_1G_PRINT
            if ((i % output_interval) == 0) {
            auto middle_time = chrono::high_resolution_clock::now();
            chrono::duration<double, std::micro> diff = middle_time - last_time;
            cout<<fixed<<setprecision(4)
                <<"every_1GB("<<i / output_interval<<"th):"
                <<" time: "<<diff.count() * 1e-6<<" s,"
                <<" throughput: "<<(config.key_size_ + config.value_size_) * output_interval * 1e6 / diff.count() / 1048576<<" MB/s,"
                <<" IOPS: "<<output_interval * 1e6 / diff.count()<<" IOPS\n";
            last_time = middle_time;
            stats.print_latency();
            stats.clear_period();
        }
#endif
        }

        /*if(single){
            // get in a table which is 0
            stats.clear_period();
            stats.start();
            do_get(*ops_key[0], 0);
            stats.end();
            stats.print_cur();
        }else{
            // get in a table which is one of 1024
            stats.clear_period();
            stats.start();
            do_get(*ops_key[0], 0);
            stats.end();
            stats.print_cur();

            // get in overall 1024 table
            stats.clear_period();
            stats.start();
            do_get(ops_key);
            stats.end();
            stats.print_cur();

            for(auto v: ops_key){
                v->clear();
                delete v;
            }
        }*/
        CZL_PRINT("end!");
    }

// 需要另外开一个线程负责模拟KV写入DRAM的skiplist结构
// 模拟数据写入到DRAM的跳表结构中
    void write_to_dram() {
        CZL_PRINT("start!");
        auto rnd = rocksdb::Random::GetTLSInstance();
        char buf[config.key_size_ + config.value_size_];
        memset(buf, 0, sizeof(buf));
        string value(config.value_size_, 'v');
        /*start_time = get_now_micros();
        last_tmp_time = start_time;*/
        auto start = chrono::high_resolution_clock::now();
        auto last_time = start;
        //string batch;
        for (uint64_t i = 1; i <= config.ops_num_; i++) {
            auto number = rnd->Next() % config.ops_num_;
            snprintf(buf, sizeof(buf), "%08d%08d%s", number, i, value.c_str());
            string insert_data(buf);
            skiplist_dram->Insert(insert_data);

#ifdef EVERY_1G_PRINT
            if ((i % output_interval) == 0) {
                /*tmp_time = get_now_micros();
                tmp_use_time = tmp_time - last_tmp_time;*/
                auto middle_time = chrono::high_resolution_clock::now();
                chrono::duration<double, std::micro> diff = middle_time - last_time;
                cout<<"every 1GB ("<<i / output_interval<<"GB):"
                    <<" time: "<< 1.0 * diff.count() * 1e-6<<" s,"
                    <<" speed: "<<1.0 * (config.key_size_ + config.value_size_) * output_interval * 1e6 / diff.count() / 1048576<<" MB/s,"
                    <<" IOPS: "<<1.0 * output_interval * 1e6 / diff.count()<<" IOPS\n";
                last_time = middle_time;
            }
#endif
        }
    }

    void do_get(vector<vector<string> *> &keys) {
        cout << keys.size() << endl;
        for (int i = 0; i < config.get_after_insert_; i++) {
            auto rnd = rocksdb::Random::GetTLSInstance();
            size_t table_pos = rnd->Next() % keys.size();
            vector<string> &v = *keys[table_pos];
            size_t key_pos = rnd->Next() % v.size();
            skiplist_nvm->Get(v[key_pos]);
        }
    }

    void do_get(vector<string> &keys, size_t which) {
        for (int i = 0; i < config.get_after_insert_; i++) {
            auto rnd = rocksdb::Random::GetTLSInstance();
            size_t pos = rnd->Next() % keys.size();
            skiplist_nvm->Get(keys[pos], which);
        }
    }

    void nvm_print() {
        chrono::duration<double, std::micro> diff = end_ - start_;
        printf("-------------   write to nvm  start: ----------------------\n");
        printf("key: %zuB, value: %zuB, number: %llu\n", config.key_size_, config.value_size_, config.ops_num_);
        printf("time: %.4f s,  speed: %.3f MB/s, IOPS: %.1f IOPS\n",
               1.0 * diff.count() * 1e-6,
               1.0 * (config.key_size_ + config.value_size_) * config.ops_num_ * 1e6 / diff.count() / 1048576,
               1.0 * config.ops_num_ * 1e6 / diff.count());
        printf("-------------   write to nvm  end: ----------------------\n");
    }

    void dram_print() {
        chrono::duration<double, std::micro> diff = end_ - start_;
        printf("-------------   write to dram  start: ----------------------\n");
        printf("key: %zuB, value: %zuB, number: %llu, mem_skiplist_size: %zu\n",
               config.key_size_,
               config.value_size_,
               config.ops_num_,
               config.mem_skiplist_size_);
        printf("time: %.4f s,  speed: %.3f MB/s, IOPS: %.1f IOPS\n",
               1.0 * diff.count() * 1e-6,
               1.0 * (config.key_size_ + config.value_size_) * config.ops_num_ * 1e6 / diff.count() / 1048576,
               1.0 * config.ops_num_ * 1e6 / diff.count());
        printf("-------------   write to dram   end: ------------------------\n");
    }

    void dram_not_flush_print() {
        chrono::duration<double, std::micro> diff = end_ - start_;
        printf("-------------   write to dram  not flush  --- start: ----------------------\n");
        printf("key: %zuB, value: %zuB, number: %llu, mem_skiplist_size: %zu\n",
               config.key_size_,
               config.value_size_,
               config.ops_num_,
               config.skiplist_max_num_);
        printf("time: %.4f s,  speed: %.3f MB/s, IOPS: %.1f IOPS\n",
               1.0 * diff.count() * 1e-6,
               1.0 * (config.key_size_ + config.value_size_) * config.ops_num_ * 1e6 / diff.count() / 1048576,
               1.0 * config.ops_num_ * 1e6 / diff.count());
        printf("-------------   write to dram  not flush ---  end: ------------------------\n");
    }

    const Config config;
    size_t output_interval;
    rocksdb::SkiplistWriteNVM *skiplist_dram;
    rocksdb::PersistentSkiplistWrapper *skiplist_nvm;
    ThreadPool *tpool;
    chrono::high_resolution_clock::time_point start_;
    chrono::high_resolution_clock::time_point end_;
};

/* parameter: 
 *    @using_existing_data: whether using old data or not(read operations must rely on existing data on NVM)
 *    @test_type(0: write NVM directly, 1: write DRAM and thread flush data to NVM)
 *    @value_size
 *    @ops_type: Read=0, Write=1
 *    @ops_num: data_size = @value_size * @ops_num
 *    @mem_skiplist_size: limit skiplist size(MB) in DRAM. It also used to initilize DRAM skiplist
 *    @skiplist_max_num: allow maximum number skiplist not dealt by background thread.
 *    @storage_type: 0:skiplist, 1:B-Tree, 2:B+ Tree        // To Be Done
 */
int main(int argc, char **argv) {
    // parse parameter
    CZL_PRINT("num=%d", argc);
    if (argc != 8) {
        cout << "input parameter nums incorrect! " << argc << endl;
        return -1;
    }

    bool using_existing_data = atoi(argv[1]) == 1;
    bool nvm_direct = atoi(argv[2]) == 0;
    int value_size = atoi(argv[3]);
    int ops_type = atoi(argv[4]);
    int ops_num = atoi(argv[5]);
    int mem_skiplist_size = atoi(argv[6]);
    int skiplist_max_num = atoi(argv[7]);

    time_t now = time(0);
    char* dt = ctime(&now);
    cout<<"Time: "<<dt<<"\n";
    cout<<"Using Existing Data: "<<using_existing_data<<" (0:no, 1:yes)\n";
    cout<<"Write To NVM Directly: "<<nvm_direct<<"\n";
    cout<<"Key Size: "<<18<<" Bytes\nValue Size: "<<value_size<<" Bytes\n";
    cout<<"OpsNum: "<<ops_num<<"("<<ops_num * value_size / 1048576.0<<")MB estimated\n";
    cout<<"Size of Memtable in Memory: "<<mem_skiplist_size<<" MB\n";
    cout<<"Max Skiplist Num: "<<skiplist_max_num<<"\n";
    cout<<"Data Structure: Skiplist\n";

    //assert((test_type >> 1) == 0);
    assert((value_size & (value_size - 1)) == 0);
    assert(skiplist_max_num >= 1 && skiplist_max_num <= 20);

    Config config(
            NVM_PATH,
            NVM_LOG_PATH,
            NVM_SIZE,
            18/*key_size*/,
            value_size,
            ops_num,
            10000/*get_after_insert*/,
            mem_skiplist_size,
            skiplist_max_num,
            using_existing_data,
            nvm_direct,
            SKIPLIST
            );

    auto benckmark = new RunBenchmark(std::move(config));
    benckmark->Run();
    return 0;
}
