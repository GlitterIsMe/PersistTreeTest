//
// Created by 张艺文 on 19.5.30.
//

#include "statistic.h"

Statistic::Statistic() {
    read_ = 0.0;
    write_ = 0.0;
    num_ = 0;
    total_num_ = 0;
    total_read_ = 0.0;
    total_write_ = 0.0;
    comp_lat_ = 0.0;
    total_comp_lat_ = 0.0;
    comp_num_ = 0;
    total_comp_num_ = 0;

    node_search_ = 0;
}
void Statistic::start(){
    start_ = chrono::high_resolution_clock::now();
}

void Statistic::end() {
    end_ = chrono::high_resolution_clock::now();
}

void Statistic::add_search() {
    chrono::duration<double, std::nano> diff = end_ - start_;
    read_ += diff.count();
    total_read_ += diff.count();
    /*if(num_ % 1000 == 0){
        cout<<"search lat "<<diff.count()<<"\n";
    }*/
}

void Statistic::add_write() {
    chrono::duration<double, std::nano> diff = end_ - start_;
    write_ += diff.count();
    total_write_ += diff.count();
}

void Statistic::add_comp_lat() {
    chrono::duration<double, std::nano> diff = end_ - start_;
    comp_lat_ += diff.count();
    total_comp_lat_ += diff.count();
}

void Statistic::add_entries_num() {
    num_++;
    total_num_++;
}

void Statistic::clear_period() {
    read_ = 0.0;
    write_ = 0.0;
    num_ = 0;
    node_search_ = 0;
    comp_lat_ = 0.0;
    comp_num_ = 0;
}

void Statistic::print_latency() {
    cout
    //<<"num "<<num_
    //<<" period_read_latency(ns) "<<read_
    <<"average_read_latency(ns) "<<read_ / num_
    <<" average_node_search "<<node_search_ / num_
    //<<" period_write_latency(ns) "<<write_
    <<" average_write_latency(ns) "<<write_ / num_
    //<<" average_compare_latency(ns) "<<comp_lat_ / comp_num_
    //<<" average_compare_times "<<comp_num_ / num_
    <<"\n";
}