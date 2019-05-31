//
// Created by 张艺文 on 19.5.30.
//

#ifndef PERSISTTREETEST_STATISTIC_H
#define PERSISTTREETEST_STATISTIC_H

#include <chrono>
#include <ratio>
#include <iostream>
using namespace std;

// ATTENTION: only for single thread!!!

class Statistic{
public:
    Statistic();
    ~Statistic() = default;

    void start();

    void end();

    void add_search();

    void add_write();

    void add_comp_lat();

    void add_comp_num(){
        comp_num_++;
        total_comp_num_++;
    };

    void add_entries_num();

    void print_latency();

    void clear_period();

    void add_node_search(){node_search_++;}

    void print_cur(){
        chrono::duration<double, std::nano> diff = end_ - start_;
        cout<<"total_time: "<<diff.count()<<"\n";
    }

private:
    double read_;
    double write_;
    double total_read_;
    double total_write_;

    double comp_lat_;
    double total_comp_lat_;
    uint64_t comp_num_;
    uint64_t total_comp_num_;

    chrono::high_resolution_clock::time_point start_;
    chrono::high_resolution_clock::time_point end_;
    uint64_t num_;
    uint64_t total_num_;

    uint64_t node_search_;
};

#endif //PERSISTTREETEST_STATISTIC_H


