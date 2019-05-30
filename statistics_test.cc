//
// Created by 张艺文 on 19.5.30.
//

#include "statistic.h"
#include <thread>
#include <chrono>

int main(){
    Statistic stats;
    stats.start();
    std::this_thread::sleep_for(std::chrono::microseconds(100));
    stats.end();
    stats.add_search();
    stats.add_entries_num();

    stats.start();
    std::this_thread::sleep_for(std::chrono::microseconds(200));
    stats.end();
    stats.add_search();
    stats.add_entries_num();
    stats.add_entries_num();

    stats.print_latency();
}
