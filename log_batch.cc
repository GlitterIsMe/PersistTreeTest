#include "log_batch.h"

using namespace pmem::obj;

namespace rocksdb {

const uint64_t k64MB = 67108864;

char* EncodeVarint32(char* dst, uint32_t v) {
  // Operate on characters as unsigneds
  unsigned char* ptr = reinterpret_cast<unsigned char*>(dst);
  static const int B = 128;
  if (v < (1<<7)) {
    *(ptr++) = v;
  } else if (v < (1<<14)) {
    *(ptr++) = v | B;
    *(ptr++) = v>>7;
  } else if (v < (1<<21)) {
    *(ptr++) = v | B;
    *(ptr++) = (v>>7) | B;
    *(ptr++) = v>>14;
  } else if (v < (1<<28)) {
    *(ptr++) = v | B;
    *(ptr++) = (v>>7) | B;
    *(ptr++) = (v>>14) | B;
    *(ptr++) = v>>21;
  } else {
    *(ptr++) = v | B;
    *(ptr++) = (v>>7) | B;
    *(ptr++) = (v>>14) | B;
    *(ptr++) = (v>>21) | B;
    *(ptr++) = v>>28;
  }
  return reinterpret_cast<char*>(ptr);
}

void PutVarint32(std::string* dst, uint32_t v) {
  char buf[5];
  char* ptr = EncodeVarint32(buf, v);
  dst->append(buf, ptr - buf);
}

void PutLengthPrefixedSlice(std::string* dst, const std::string& value) {
  PutVarint32(dst, value.size());
  dst->append(value.data(), value.size());
}

AEP_LOG::AEP_LOG(pool_base &pop,uint64_t entry_size):pop_(pop),entry_size_(entry_size){
        transaction::run(pop, [&] {
                head = NewEntry();
                tail = head;
                
            });
    }
    persistent_ptr<LOG_ENTRY> AEP_LOG::NewEntry(){
        persistent_ptr<LOG_ENTRY> tmp;
        transaction::run(pop_, [&] {
                tmp = make_persistent<LOG_ENTRY>();
                tmp->data= make_persistent<char[]>(entry_size_);
                tmp->size = entry_size_;
                tmp->w_oft = 0;
                tmp->next = nullptr;
            });
        return tmp;
    }
    uint64_t AEP_LOG::Put(const char *key, size_t size)  {
        persistent_ptr<LOG_ENTRY> tmp;
        if((tail->size - tail->w_oft) > size){
            transaction::run(pop_, [&] {
                uint64_t oft=tail->w_oft;
                memcpy(&tail->data[oft], key, size);
                tail->w_oft = oft + size;
            });
        }
        else{
            transaction::run(pop_, [&] {
                tmp = NewEntry();
                tail->next = tmp;
                tail = tmp;
                uint64_t oft=tail->w_oft;
                memcpy(&tail->data[oft], key, size);
                tail->w_oft = oft + size;
            });
        }
            
    }

    void AEP_LOG::Get()  {
        persistent_ptr<LOG_ENTRY> tmp=head;
        uint64_t size=0,num=0;
        while(tmp != nullptr){
            size=tmp->size;
            num++;
            tmp=tmp->next;
        }
    }

    void AEP_LOG::Delete() {
        persistent_ptr<LOG_ENTRY> tmp = head;
        uint64_t size = 0, num = 0;
        while (tmp != nullptr) {
            tmp = head->next;
            transaction::run(pop_, [&] {
            //    delete_persistent<char[]>(head->data);
                delete_persistent<char[]>(head->data, head->size);
                delete_persistent<LOG_ENTRY>(head);
                head = tmp;
            });
        }
    }

    Memory_SkipList::Memory_SkipList( int32_t max_height, int32_t branching_factor, size_t key_size)
            :
            kMaxHeight_(static_cast<uint16_t>(max_height)),
            kBranching_(static_cast<uint16_t>(branching_factor)),
            key_size_(key_size),
            kScaledInverseBranching_((Random::kMaxNext + 1) / kBranching_),
            max_height_(1){
        head_ = NewNode(" ", max_height);
        prev_ = static_cast<DramNode **>(malloc(sizeof(DramNode *) * max_height));

        for (int i = 0; i < kMaxHeight_; i++) {
            head_->SetNext(i, nullptr);
            prev_[i] = head_;
        }

        prev_height_ = 1;
    }
    Memory_SkipList::~Memory_SkipList() {
        DramNode *start = head_->Next(0);
        DramNode *tmp = nullptr;
        while (start != nullptr) {
            tmp = start;
            start = start->Next(0);
            delete tmp;             // equal to : delete (tmp->key_); free(tmp);
        }
        delete head_;
    }

    DramNode *Memory_SkipList::NewNode(const std::string &key, int height) {
        DramNode *n;
        n = static_cast<DramNode*>(malloc(sizeof(DramNode) + height * sizeof(DramNode*)));
        n->height = height;
        n->key_ = new char[key.size()];
        memcpy(n->key_, key.c_str(), key.size());
        n->key_size = key.size();
        return n;
    }

    int Memory_SkipList::RandomHeight() {
        auto rnd = Random::GetTLSInstance();
        int height = 1;
        while (height < kMaxHeight_ && rnd->Next() < kScaledInverseBranching_) {
            height++;
        }
        return height;
    }

    // when n < key returns true
    // n should be at behind of key means key is after node
    bool Memory_SkipList::KeyIsAfterNode(const std::string &key, DramNode *n) const {
        //printf("n is %s\n", n == nullptr ? "null" : "not null");
        return (n != nullptr) && (strncmp(n->key_, key.c_str(), key_size_) < 0);
    }

    int Memory_SkipList::CompareKeyAndNode(const std::string& key, DramNode* n) {
        int res = strncmp(key.c_str(), n->key_, key_size_);        // 16 equal KEY_SIZE
        return res;
    }

    DramNode *Memory_SkipList::FindLessThan(const std::string &key,
                                            DramNode **prev) const {
        DramNode *x = head_;
        int level = GetMaxHeight() - 1;

        for (int i = level; i >= 0; i--) {
            DramNode *next = x->Next(i);
            while (next != nullptr && KeyIsAfterNode(key, next)) {
                x = next;
                next = x->Next(i);
            }
            prev[i] = x;
        }
    }

    void Memory_SkipList::FindNextNode(const std::string &key, DramNode** prev)
    {
        int level = GetMaxHeight() - 1;
        DramNode* x = prev[level];

        for (int i = level; i >= 0; i--) {
            DramNode *next = x->Next(i);
            while (next != nullptr && KeyIsAfterNode(key, next)) {
                x = next;
                next = x->Next(i);
            }
            prev[i] = x;
        }
    }

    // 相同的key如何处理
    void Memory_SkipList::Insert(const std::string &key) {
        // key < prev[0]->next(0) && prev[0] is head or key < prev[0]
        bool is_same_node = false;
        int res = CompareKeyAndNode(key, prev_[0]);
        if (res > 0) {
            if (prev_[0]->Next(0) == nullptr) {
                // for (uint32_t i = 1; i < prev_height_; i++) {
                //     prev_[i] = prev_[0];
                // }
                ;
            } else {
                int res_next = CompareKeyAndNode(key, prev_[0]->Next(0));
                if (res_next < 0) {     // 节点插入在prev_[0] 与 prev_[0]->Next(0)之间
                    // for (uint32_t i = 1; i < prev_height_; i++) {
                    //     prev_[i] = prev_[0];        // 有无必要？
                    // }
                    ;
                } else if (res_next > 0) {
                    FindNextNode(key, prev_);
                } else {
                    is_same_node = true;
                    printf("impossible key is equal!\n");
                }
            }
        } else if (res < 0) {
            FindLessThan(key, prev_);
        } else {
            is_same_node = true;
            printf("Memory_SkipList::Insert() key is equal\n");
            return;
        }

        if (is_same_node) {
            // 默认所有key的key_size均相等
            memcpy(prev_[0], key.c_str(), key.size());
            prev_[0]->key_size = key.size();
            return ;
        }

        int height = RandomHeight();
        if (height > GetMaxHeight()) {
            for (int i = GetMaxHeight(); i < height; i++) {
                prev_[i] = head_;
            }
            max_height_ = static_cast<uint16_t >(height);
        }

        DramNode *x = NewNode(key, height);
        for (int i = 0; i < height; i++) {
            x->SetNext(i, prev_[i]->Next(i));
            prev_[i]->SetNext(i, x);

            // add it
            prev_[i] = x;
        }
        //prev_[0] = x;
        prev_height_ = static_cast<uint16_t >(height);
    }

    // DramNode *Memory_SkipList::FindGreaterOrEqual(const std::string &key) const {
    //     DramNode *x = head_;
    //     int level = GetMaxHeight() - 1;
    //     DramNode *last_bigger;
    //     while (true) {
    //         DramNode *next = x->Next(level);
    //         int cmp = (next == nullptr || next == last_bigger) ? 1 : strcmp(next->key_, key.c_str());
    //         if (cmp == 0 || (cmp > 0 && level == 0)) {
    //             return next;
    //         } else if (cmp < 0) {
    //             x = next;
    //         } else {
    //             last_bigger = next;
    //             level--;
    //         }
    //     }
    // }
}

ThreadPool *tpool = nullptr;