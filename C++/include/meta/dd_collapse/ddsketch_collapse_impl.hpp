#pragma once
#include "ddsketch_collapse.hpp"
#include <stdexcept>
#include <cmath>
#include <algorithm>

namespace sketch {
    DDCSketch::DDCSketch(u32 cap_, f64 alpha_, double ddc_alpha)
        : cap(cap_), alpha(alpha_), 
          gamma((1.0 + alpha) / (1.0 - alpha)) {
        if (alpha <= 0.0 || alpha >= 1.0) {
            throw std::invalid_argument("alpha must be in (0, 1)");
        }
        

        u32 num = std::ceil(std::log2(1e9) / std::log2(gamma)) + 1;

        alpha -= ddc_alpha;
        gamma = (1.0 + alpha) / (1.0 - alpha);
        // counters.resize(num);
        // for(u8 i = 0;i < num; ++i) counters[i].first = i, counters[i].second = 0;

        // M
        max_counter_num = num;
        // cout << "-------------------"  << endl;
        // cout << max_counter_num << endl;
        // cout << gamma << endl;
        // cout << alpha << endl;
        // cout << num << endl;

        counters.clear();
    }

    u32 DDCSketch::size() const {
        return totalSize;
    }

    u32 DDCSketch::capacity() const {
        return cap;
    }

    bool DDCSketch::empty() const {
        return size() == 0;
    }

    bool DDCSketch::full() const {
        return maxCnt >= cap;
    }

    u32 DDCSketch::memory() const {
        u32 counter_bits = std::ceil(std::log2(static_cast<f64>(cap) + 1));
        return (counter_bits * max_counter_num + 7) / 8;
    }

    u8 DDCSketch::pos(u32 item) const {
        return std::ceil(std::log2(item) / std::log2(gamma));
    }

    u8 DDCSketch::find_id(u8 posx) const {
        for(u8 i = 0;i < counters.size(); ++i)
            if(counters[i].first >= posx) return i;
        return counters.size();
    }

    void DDCSketch::insert_id(pair32 counter, u8 idx) {
        // printf("qwq %d\n", idx);
        if(counters.size() > idx && counters[idx].first == counter.first) {
            ++counters[idx].second;
            ++totalSize;
            maxCnt = std::max(maxCnt, counters[idx].second);
        } else{
            counters.insert(counters.begin() + idx, counter);
            // for(int i = 0;i < counters.size(); ++i)
            //     cout << "???" << (u32)counters[i].first << " " << counters[i].second << " " << (u32)idx << endl;
            // getchar();
            // exit(0);
            if(counters.size() > max_counter_num) {
                pair32 tmp_counter = counters[0];
                counters.erase(counters.begin());
                counters[0].second += tmp_counter.second;
                maxCnt = std::max(maxCnt, counters[0].second);
            }
            ++totalSize;
        }
    }

    void DDCSketch::append(u32 item) {
        u8 posx = pos(item);
        u8 idx = find_id(posx);
        if (idx < counters.size() && counters[idx].second >= cap) {
            throw std::runtime_error("append to a full DDCSketch");
        }
        insert_id(make_pair(posx, 1), idx);
    }

    // void DDCSketch::append(u32 item, u32 pos) {
    //     ++counters[pos];
    //     ++totalSize;
    //     maxCnt = std::max(maxCnt, counters[pos]);
    // }

    u32 DDCSketch::quantile(f64 nom_rank) const {
        if (nom_rank < 0.0 || nom_rank > 1.0) {
            throw std::invalid_argument("normalized rank out of range");
        }

        u32 rank = nom_rank * (totalSize - 1);
        u32 idx = 0;

        for (u32 sum = counters[0].second; sum <= rank; sum += counters[++idx].second);

        f64 res = counters[idx].first == 0 ? 1 : 2 * std::pow(gamma, counters[idx].first) / (gamma + 1);
        return std::lrint(res);
    }

    DDCSketch::operator Histogram() const {
    u32 sz = counters.size();

    vec_f64 split;
    vec_u32 height;

    split.push_back(0); 

    if (sz > 0) {
        split.push_back(std::pow(gamma, counters[0].first));
        height.push_back(counters[0].second);
    }

    for (u32 i = 1; i < sz; ++i) {
        if(counters[i-1].first != counters[i].first - 1) {
            split.push_back(std::pow(gamma, counters[i].first-1));
            height.push_back(0);
        }
        split.push_back(std::pow(gamma, counters[i].first));
        height.push_back(counters[i].second);
    }
    return Histogram(split, height);
}

}   // namespace sketch
