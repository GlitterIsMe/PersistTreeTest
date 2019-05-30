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
    Statistic() = default;
    ~Statistic() = default;

    void start();

    void end();

    void add_search();

    void add_write();

    void add_entries_num();

    void print_latency();

    void clear_period();

private:
    double read;
    double write;
    double total_read;
    double total_write;
    chrono::time_point start;
    chrono::time_point end;
    uint64_t num;
    uint64_t total_num;
};

#endif //PERSISTTREETEST_STATISTIC_H


