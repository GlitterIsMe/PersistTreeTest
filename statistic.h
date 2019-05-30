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

    void add_entries_num();

    void print_latency();

    void clear_period();

private:
    double read_;
    double write_;
    double total_read_;
    double total_write_;
    chrono::high_resolution_clock::time_point start_;
    chrono::high_resolution_clock::time_point end_;
    uint64_t num_;
    uint64_t total_num_;
};

#endif //PERSISTTREETEST_STATISTIC_H


