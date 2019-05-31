#include "persistent_skip_list.h"
#include "statistic.h"
namespace rocksdb {

    // class Persistent_SkipList
    Persistent_SkipList::Persistent_SkipList(PersistentAllocator* allocator, int32_t max_height, int32_t branching_factor, size_t key_size ,uint64_t opt_num,size_t per_1g_num)
            :
            allocator_(allocator),
            kMaxHeight_(static_cast<uint16_t>(max_height)),
            key_size_(key_size),
            kBranching_(static_cast<uint16_t>(branching_factor)),
            kScaledInverseBranching_((Random::kMaxNext + 1) / kBranching_),
            max_height_(1) {
        head_ = NewNode(" ", max_height);
#ifdef CAL_ACCESS_COUNT
        cnt_time_ = 0;
        all_cnt_ = 0;
        suit_cnt_ = 0;
        head_cnt_ = 0;
        opt_num_ = opt_num;
        opt_1g_num_ = 0;
        per_1g_num_ = per_1g_num;
#endif
        prev_ = reinterpret_cast<Node**>(allocator_->Allocate(sizeof(Node*) * kMaxHeight_));
        for (int i = 0; i < kMaxHeight_; i++) {
            head_->SetNext(i, nullptr);
            prev_[i] = head_;
            pmem_flush(prev_ + i, sizeof(Node*));
        }
        pmem_drain();
        prev_height_ = 1;
    }

    Node* Persistent_SkipList::NewNode(const std::string &key, int height) {
        bool is_pmem = allocator_->is_pmem();
        char* mem = allocator_->Allocate(sizeof(Node) + sizeof(Node*) * (height - 1));
        char* pkey = allocator_->Allocate(key.size());
        if(is_pmem){
            pmem_memcpy_persist(pkey, key.c_str(), key.size());
        }else{
            memcpy(pkey, key.c_str(), key.size());
            pmem_msync(pkey, key.size());
        }
        return new (mem) Node(pkey);
    }

    int Persistent_SkipList::RandomHeight() {
        auto rnd = Random::GetTLSInstance();
        int height = 1;
        while (height < kMaxHeight_ && rnd->Next() < kScaledInverseBranching_) {
            height++;
        }
        return height;
    }

    bool Persistent_SkipList::KeyIsAfterNode(const std::string& key, Node* n, Statistic &stat) const {
        if(n == nullptr) return false;
        stat.start();
        int res = strncmp(n->key_, key.c_str(), key_size_);
        stat.end();
        stat.add_comp_lat();
        stat.add_comp_num();
        return res < 0;
        //return (n != nullptr) && (strncmp(n->key_, key.c_str(), key_size_) < 0);
    }

    int Persistent_SkipList::CompareKeyAndNode(const std::string& key, Node* n) {
        int res = strncmp(key.c_str(), n->key_, key_size_);        // 16 equal KEY_SIZE
        return res;
    }

    // ignore function name
    void Persistent_SkipList::FindLessThan(const std::string &key, Node** prev) {
        Node* x = head_;
        int level = GetMaxHeight() - 1;
#ifdef CAL_ACCESS_COUNT
        uint64_t cnt = 0;
        all_cnt_ += 1;
        head_cnt_ += 1;
#endif

        for (int i = level; i >= 0; i--) {
            Node *next = x->Next(i);
            Statistic stat;
            while (next != nullptr && KeyIsAfterNode(key, next, stat)) {
                x = next;
                next = x->Next(i);
#ifdef CAL_ACCESS_COUNT
                cnt++;
#endif
            }
            prev[i] = x;
        }

#ifdef CAL_ACCESS_COUNT
        cnt_time_ += cnt;
        if(opt_1g_num_%per_1g_num_==0){
            printf("every 1GB(%dGB): times: %.1f\n", (opt_1g_num_ / per_1g_num_), 1.0*cnt_time_/per_1g_num_);
		//	printf("every 64MB(%d/64MB): times: %.1f\n", (opt_1g_num_ / per_1g_num_), 1.0*cnt_time_/per_1g_num_);
			cnt_time_ = 0;
        } 
#endif
    }

    void Persistent_SkipList::FindNextNode(const std::string &key, Node** prev, Statistic &stats)
    {
        // 从prev[level]节点往后查找合适的node
        int level = GetMaxHeight() - 1;
        //Node* x = prev[level];
        Node *x = head_;

        for (int i = level; i >= 0; i--) {
            // next初始化为x的next，第一层为head
            // 后面就是查找的地方的next
            Node* next = x->Next(i);
            stats.add_node_search();
            while (next != nullptr && KeyIsAfterNode(key, next, stats)) {
                x = next;
                next = x->Next(i);
                stats.add_node_search();
            }
            prev[i] = x;
        }
    }

    void Persistent_SkipList::Insert(const std::string &key, Statistic& stats) {
        // 比较待插入节点与prev[0]的关系
        stats.start();
        /*if(!KeyIsAfterNode(key, prev_[0]->Next(0)) &&
            (prev_[0] == head_ || KeyIsAfterNode(key, prev_[0]))){
            for(size_t i = 1; i < prev_height_; i++){
                prev_[i] = prev_[0];
            }
            stats.add_node_search();
        }else{
            FindNextNode(key, prev_, stats);
        }*/
        FindNextNode(key, prev_, stats);
        stats.end();
        stats.add_search();

        int height = RandomHeight();
        if(height > GetMaxHeight()){
            for(int i = GetMaxHeight(); i < height; i++){
                prev_[i] = head_;
            }
            max_height_ = static_cast<uint16_t>(height);
        }

        stats.start();
        Node* x = NewNode(key, height);
        bool is_pmem = allocator_->is_pmem();
        for(int i = 0; i < height; i++){
            x->SetNext(i, prev_[i]->Next(i));
            prev_[i]->SetNext(i ,x);
            if(is_pmem){
                pmem_persist(prev_[i], sizeof(Node*));
            }else{
                pmem_msync(prev_[i], sizeof(Node*));
            }
            //prev_[i] = x;
        }
        prev_[0] = x;
        if(is_pmem){
            pmem_persist(prev_[0], sizeof(Node*));
        }else{
            pmem_msync(prev_[0], sizeof(Node*));
        }
        stats.end();
        stats.add_write();
        stats.add_entries_num();
        prev_height_ = static_cast<uint16_t >(height);
    }

    std::string Persistent_SkipList::Get(const std::string &key) {
        Node* grt_or_equal = FindGreaterOrEqual(key);
        if(grt_or_equal != nullptr){
            int cmp = strncmp(grt_or_equal->key_, key.c_str(), key_size_);
            if(cmp == 0){
                return std::string(grt_or_equal->key_, strlen(grt_or_equal->key_));
            }else{
                return string("");
            }
        }else{
            return string("");
        }
    }

    Node* Persistent_SkipList::FindGreaterOrEqual(const std::string &key) const {
        Node* x = head_;
        int level = GetMaxHeight() - 1;
        Node* last_bigger;
        while(true){
            Node* next = x->Next(level);
            int cmp = (next == nullptr || next == last_bigger) ? 1 : strncmp(next->key_, key.c_str(), key_size_);
            if(cmp == 0 || (cmp > 0 && level ==0)){
                return next;
            } else if(cmp < 0) {
                x = next;
            } else {
                last_bigger = next;
                level--;
            }
        }
    }

    void Persistent_SkipList::PrintKey(const char *str ,uint64_t &last_num, uint64_t &last_seq_num) const {
        // uint64_t key_num, seq_num;
        // key_num = 0;    seq_num = 0;

        // for (int i = 0; i < 8; i++) {
        //     key_num = key_num * 10 + (str[i] - '0');
        // }
        // for (int i = 8; i < 16; i++) {
        //     seq_num = seq_num * 10 + (str[i] - '0');
        // }
        // printf("%llu - %llu    ", key_num, seq_num);
        uint64_t num = 0;
        uint64_t seq_num = 0;
        for (int i = 0; i < 8; i++) {
            num = (str[i] - '0') + 10 * num;
        }
        
        for (int i = 8; i < 18; i++) {
            seq_num = (str[i] - '0') + 10 * seq_num;
        }

        bool res = ((last_num < num) || ((last_num == num) && (last_seq_num < seq_num)));
        //printf("%llu - %llu\n", num, seq_num);
        if (!res) {
            printf("----------------error: DRAM skiplist is not in order!!! -----------------------------\n");
        }

        last_num = num;
        last_seq_num = seq_num;
    }

    void Persistent_SkipList::Print() const {
        Node* start = head_->Next(0);
        uint64_t last_num = 0;
        uint64_t last_seq_num = 0;
        while(start != nullptr) {
            PrintKey(start->key_,last_num, last_seq_num);
            start = start->Next(0);
        }
    }

    void Persistent_SkipList::PrintLevelNum() const {
        for(int i=0;i<kMaxHeight_;i++){
            Node* start = head_->Next(i);
            int num=0;
            while(start != nullptr){
                num++;
                start=start->Next(i);
            }
            printf("Level %d = %d\n",i,num);
        }
        printf("\n");
    }

#ifdef CAL_ACCESS_COUNT
    void Persistent_SkipList::PrintAccessTime() {
        printf("opt num = %llu\n",opt_num_);
        printf("nvm skiplist: time = %.1f, all_cnt = %llu, head_cnt = %llu, suit_cnt = %llu\n", 1.0*cnt_time_/opt_num_, all_cnt_, head_cnt_, suit_cnt_);
    }
#endif
}
