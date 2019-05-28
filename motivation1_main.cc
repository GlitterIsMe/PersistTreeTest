#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>

#include "test_common.h"
#include "random.h"
#include "persistent_skiplist_wrapper.h"
#include "log_batch_wrapper.h"

using namespace std;

//#define PATH "/home/czl/pmem0/skiplist_test_dir/persistent_skiplist"

#define PATH "/dev/dax1.0"

#define PATH_LOG "/home/czl/pmem0/skiplist_test_dir/dram_skiplist"

//const size_t NVM_SIZE = 50 * (1ULL << 30);             // 50GB
const size_t NVM_SIZE = 133175443456;             // 50GB
//const size_t NVM_LOG_SIZE = 42 * (1ULL << 30);         // 42GB
const size_t KEY_SIZE = 16;         // 16B

// default parameter
bool using_existing_data = false;
size_t test_type = 0;
size_t VALUE_SIZE = 4096;
bool ops_type = true;
uint64_t ops_num = 1000000;
size_t mem_skiplist_size = 64;       // default: 64MB
size_t skiplist_max_num = 1;

size_t buf_size;        // initilized in parse_input()

rocksdb::SkiplistWriteNVM *skiplist_dram;

uint64_t start_time, end_time, use_time, last_tmp_time, tmp_time, tmp_use_time;

void dram_print()
{
    printf("-------------   write to dram  start: ----------------------\n");
    printf("key: %uB, value: %uB, number: %llu, mem_skiplist_size: %u\n", KEY_SIZE, VALUE_SIZE, ops_num, mem_skiplist_size);
    printf("time: %.4f s,  speed: %.3f MB/s, IOPS: %.1f IOPS\n", 1.0*use_time*1e-6, 1.0 * (KEY_SIZE + VALUE_SIZE) * ops_num * 1e6 / use_time / 1048576, 1.0 * ops_num * 1e6 / use_time);
    printf("-------------   write to dram   end: ------------------------\n");
}

void dram_not_flush_print()
{
    uint64_t tmp1 = get_now_micros();
    uint64_t tmp2 = tmp1 - start_time;
    printf("-------------   write to dram  not flush  --- start: ----------------------\n");
    printf("key: %uB, value: %uB, number: %llu, mem_skiplist_size: %u\n", KEY_SIZE, VALUE_SIZE, ops_num, mem_skiplist_size);
    printf("time: %.4f s,  speed: %.3f MB/s, IOPS: %.1f IOPS\n", 1.0*tmp2*1e-6, 1.0 * (KEY_SIZE + VALUE_SIZE) * ops_num * 1e6 / tmp2 / 1048576, 1.0 * ops_num * 1e6 / tmp2);
    printf("-------------   write to dram  not flush ---  end: ------------------------\n");
}

void nvm_print()
{
    printf("-------------   write to nvm  start: ----------------------\n");
    printf("key: %uB, value: %uB, number: %llu\n", KEY_SIZE, VALUE_SIZE, ops_num);
    printf("time: %.4f s,  speed: %.3f MB/s, IOPS: %.1f IOPS\n", 1.0*use_time*1e-6, 1.0 * (KEY_SIZE + VALUE_SIZE) * ops_num * 1e6 / use_time / 1048576, 1.0 * ops_num * 1e6 / use_time);
    printf("-------------   write to nvm  end: ----------------------\n");
}

// 需要另外开一个线程负责模拟KV写入DRAM的skiplist结构
// 模拟数据写入到DRAM的跳表结构中
void write_to_dram()
{
    CZL_PRINT("start!");
    auto rnd = rocksdb::Random::GetTLSInstance();
    char buf[buf_size];
    memset(buf, 0, sizeof(buf));
    string value(VALUE_SIZE, 'v');
    
    start_time = get_now_micros();
    last_tmp_time = start_time;
    size_t per_1g_num = (1024 * 1000) / VALUE_SIZE * 1000;
    //string batch;
    for (uint64_t i = 1; i <= ops_num; i++) {
        auto number = rnd->Next() % ops_num;
        snprintf(buf, sizeof(buf), "%08d%08d%s", number, i, value.c_str());
        string insert_data(buf);
        skiplist_dram->Insert(insert_data);

#ifdef EVERY_1G_PRINT
        if ((i % per_1g_num) == 0) {
            tmp_time = get_now_micros();
            tmp_use_time = tmp_time - last_tmp_time;
            printf("every 1GB (%dGB): time: %.4f s,  speed: %.3f MB/s, IOPS: %.1f IOPS\n", (i / per_1g_num), 1.0 * tmp_use_time * 1e-6, 1.0 * (KEY_SIZE + VALUE_SIZE) * per_1g_num * 1e6 / tmp_use_time / 1048576, 1.0 * per_1g_num * 1e6 / tmp_use_time);
            last_tmp_time = tmp_time;
        }
#endif
    }
    dram_not_flush_print();
    skiplist_dram->Flush();     // write DRAM data to NVM.
}

void write_to_nvm()
{
    CZL_PRINT("start!");
    auto rnd = rocksdb::Random::GetTLSInstance();
    char buf[buf_size];
    memset(buf, 0, sizeof(buf));
    string value(VALUE_SIZE, 'v');
    start_time = get_now_micros();
    last_tmp_time = start_time;
    // *号前面是计算1MB有多少条记录，1GB有1024MB，所以再乘一个1024
    // 所以为什么不直接1024 * 1024 * 1024 / VALUE_SIZE，看了半天
    // per_1g_num = (1024 * 1024) / VALUE_SIZE * 1024 - 1;

    // 改成64MB统计一次
    size_t per_1g_num = (1024 * 1024) / VALUE_SIZE * 64 - 1;
    for (uint64_t i = 1; i <= ops_num; i++) {
        auto number = rnd->Next() % ops_num;
        snprintf(buf, sizeof(buf), "%08d%08d%s", number, i, value.c_str());
        string data(buf);
        skiplist_nvm->Insert(data);
#ifdef EVERY_1G_PRINT
        if ((i % per_1g_num) == 0) {
            tmp_time = get_now_micros();
            tmp_use_time = tmp_time - last_tmp_time;
            printf("every 1GB(%dGB): time: %.4f s,  speed: %.3f MB/s, IOPS: %.1f IOPS\n", (i / per_1g_num), 1.0 * tmp_use_time * 1e-6, 1.0 * (KEY_SIZE + VALUE_SIZE) * per_1g_num * 1e6 / tmp_use_time / 1048576, 1.0 * per_1g_num * 1e6 / tmp_use_time);
            last_tmp_time = tmp_time;
        }
#endif
    }
    skiplist_nvm->PrintLevelNum();
    skiplist_nvm->Print();
    CZL_PRINT("end!");
}

// parse input parameter: ok return 0, else return -1;
int parse_input(int num, char **para)
{
    CZL_PRINT("num=%d", num);
    if (num != 8) {
        cout << "input parameter nums incorrect! " << num << endl;
        return -1; 
    }

    using_existing_data = atoi(para[1]);
    test_type = atoi(para[2]);
    VALUE_SIZE = atoi(para[3]);
    ops_type = atoi(para[4]);
    ops_num = atoi(para[5]);
    mem_skiplist_size = atoi(para[6]);
    skiplist_max_num = atoi(para[7]);

    buf_size = KEY_SIZE + VALUE_SIZE + 1;

    CZL_PRINT("buf_size:%u", buf_size);
    CZL_PRINT("using_existing_data: %d(0:no, 1:yes)", using_existing_data);
    CZL_PRINT("test_type:%u(0:NVM, 1:DRAM)  value_size:%u", test_type, VALUE_SIZE);
    CZL_PRINT("ops_type:%d      ops_num:%llu", ops_type, ops_num);
    CZL_PRINT("mem_skiplist_size:%uMB   skiplist_max_num:%u", mem_skiplist_size, skiplist_max_num);

    assert((test_type>>1) == 0);
    assert((VALUE_SIZE & (VALUE_SIZE-1)) == 0);
//    assert(mem_skiplist_size <= 4096);
    assert(skiplist_max_num >= 1 && skiplist_max_num <= 20);
    return 0;
}

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
int main(int argc, char **argv)
{
    // parse parameter
    int res = parse_input(argc, argv);
    if (res < 0) {
        cout << "parse parameter failed!" << endl;
        return 0;
    }
    size_t per_1g_num = (1024 * 1024) / VALUE_SIZE * 1024 - 1;
    skiplist_nvm = new rocksdb::PersistentSkiplistWrapper();
    skiplist_nvm->Init(PATH, NVM_SIZE, 20, 2, KEY_SIZE,ops_num,per_1g_num);
    
    CZL_PRINT("prepare to create PATH_LOG:%s", PATH_LOG);
    // KV write DRAM skiplist, and then background thread write it to NVM
    if (test_type == 1) {
        tpool = new ThreadPool(2);          // create threadpool(thread number = 2)

        skiplist_dram = new rocksdb::SkiplistWriteNVM();
        skiplist_dram->Init(PATH_LOG, skiplist_nvm, 12, 4, mem_skiplist_size * 1024 * 1024, skiplist_max_num, KEY_SIZE);
        
        thread t(write_to_dram); // create new thread, beginning with write_to_dram()

        t.join();
        delete tpool;

        delete skiplist_dram;
    } else {
        thread t(write_to_nvm);
        t.join();
    }

    end_time = get_now_micros();
    use_time = end_time - start_time;
    if (test_type == 1) {
        dram_print();
    } else {
        nvm_print();
    }

    delete skiplist_nvm;
    return 0;
}
