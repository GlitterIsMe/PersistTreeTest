//
// Created by 张艺文 on 19.5.30.
//

#include "statistic.h"

void Statistic::start_search() {
    start = chrono::high_resolution_clock::now();
}

void Statistic::end() {
    end = chrono::high_resolution_colock::now();
}

void Statistic::add_search() {
    chrono::duration<double, std::nano> diff = end - start;
    read += diff.count();
    total_read += diff.count();
}

void Statistic::add_write() {
    chrono::duration<double, std::nano> diff = end - start;
    write += diff.count();
    total_write += diff.count();
}

void Statistic::add_entries_num() {
    num++;
    total_num++;
}

void Statistic::clear_period() {
    read = 0.0;
    write = 0.0;
    num = 0;
}

void Statistic::print_latency() {
    cout<<"num "<<num
    <<" period_read_latency "<<read<<" ns"
    <<" average_read_latency "<<read / num<<" ns"
    <<" period_write_latency "<<write<<" ns"
    <<" average_write_latency "<<write / num<<" ns"
    <<"\n"
}