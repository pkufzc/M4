#pragma once
#include "sketch_test.hpp"
#include <cmath>
#include "../framework/andor/andor_sketch.hpp"
#include "../framework/dleft/dleft_sketch.hpp"
#include "../framework/cuckoo/cuckoo.hpp"

namespace sketch {
    template <typename META>
    SketchSingleTest<META>::SketchSingleTest(u64 mem_limit, u32 hash_num,
                                             u32 seed,
                                             const vector<FlowItem>& dataset) {
        models[ANDOR] = new AndorSketch<META>(mem_limit, hash_num, seed);
        models[DLEFT] = new DLeftSketch<META>(mem_limit, seed);
        models[CUCKOO] = new Cuckoo<META>(mem_limit, seed);
        
        // append all items to real
        for (auto [id, value] : dataset) {
            real.append(id, value);
            id_list.insert(id);
        }

        f64 size = static_cast<f64>(dataset.size());

        // append all items to models and calculate appending throughput
        for (u32 i = 0; i < NUM_MODELS; ++i) {
            auto start = high_resolution_clock::now();
            for (auto [id, value] : dataset) {
                models[i]->append(id, value);
            }
            auto end = high_resolution_clock::now();
            auto duration = duration_cast<microseconds>(end - start);
            append_tp[i] = size / duration.count();
        }
    }

    template <typename META>
    SketchSingleTest<META>::~SketchSingleTest() {
            delete models[ANDOR];
            delete models[DLEFT];
            delete models[CUCKOO];
        }

    template <typename META>
    FlowType SketchSingleTest<META>::getType(u32 id) const {
        return real.type(id);
    }

    template <typename META>
    f64 SketchSingleTest<META>::ALE(u32 model, u32 type) const {
        u32 flow_cnt = 0;
        f64 error = 0;
        for (u32 id : id_list) {
            if ((getType(id) & type) == 0) {
                continue;
            }
            error += FlowALE(models[model], id);
            ++flow_cnt;
        }
        return error / flow_cnt;
    }

    template <typename META>
    f64 SketchSingleTest<META>::APE(u32 model, u32 type) const {
        u32 flow_cnt = 0;
        f64 error = 0;
        for (u32 id : id_list) {
            if ((getType(id) & type) == 0) {
                continue;
            }
            error += FlowAPE(models[model], id);
            ++flow_cnt;
        }
        return error / flow_cnt;
    }

    template <typename META>
    f64 SketchSingleTest<META>::appendTp(u32 model) const {
        return append_tp[model];
    }

    template <typename META>
    f64 SketchSingleTest<META>::queryTp(u32 model) const {
        volatile u32 unused;    // just for avoiding optimization
        u32 flow_cnt = 0;
        auto start = high_resolution_clock::now();
        for (u32 i = 0; i < 10; ++i) {
            for (u32 id : id_list) {
                if (getType(id) == FlowType::TINY) {
                    continue;
                }
                unused = models[model]->quantile(id, given_p);
                (void) unused;
                ++flow_cnt;
            }
        }
        auto end = high_resolution_clock::now();
        auto duration = duration_cast<microseconds>(end - start);
        return static_cast<f64>(flow_cnt) / duration.count();
    }

    template <typename META>
    f64 SketchSingleTest<META>::FlowALE(const Framework* sketch, u32 id) const {
        f64 quan = sketch->quantile(id, given_p);
        if (quan == 0) {
            return 0;
        }
        f64 quan_real = real.quantile(id, given_p);
        return std::fabs(std::log2(quan / quan_real));
    }

    template <typename META>
    f64 SketchSingleTest<META>::FlowAPE(const Framework* sketch , u32 id) const {
        f64 quan = sketch->quantile(id, given_p);
        if (quan == 0) {
            return 0;
        }
        f64 nom_rank = real.nomRank(id, quan);
        return std::fabs(nom_rank - given_p);
    }

    template <typename META>
    SketchTest<META>::SketchTest(u64 mem_limit_, u32 hash_num_, u32 seed_,
                         const string& dataset_name_,
                         u32 repeat_)
        : mem_limit(mem_limit_), hash_num(hash_num_), seed(seed_), 
          dataset(dataset_name_), repeat(repeat_) { }

    template <typename META>
    void SketchTest<META>::run() {
        auto dataset_loaded = load_dataset(dataset);

        cout << "mem_limit: " << (mem_limit / 1024) << "KB" << endl;
        
        for (u32 i = 0; i < repeat; ++i) {
            cout << "Running test " << i << "..." << endl;
            SketchSingleTest<META> test(mem_limit, hash_num,
                                        seed, dataset_loaded);
            cout << "Calculating metrics..." << endl;
            addMetrics(test);
        }

        summarize();
        cout << "Test finished" << endl;
    }

    template <typename META>
    void SketchTest<META>::addMetrics(const SketchSingleTest<META>& test) {
        for (u32 i = 0; i < NUM_MODELS; ++i) {
            // if (i == ANDOR) {
            //     continue;
            // }
            m_ALE[i] += test.ALE(i, MID | HUGE);
            m_APE[i] += test.APE(i, MID | HUGE);
            m_appendTp[i] += test.appendTp(i);
            m_queryTp[i] += test.queryTp(i);
        }
    }

    template <typename META>
    void SketchTest<META>::summarize() {
        for (i32 i = 0; i < NUM_MODELS; ++i) {
            m_ALE[i] /= repeat;
            m_APE[i] /= repeat;
            m_appendTp[i] /= repeat;
            m_queryTp[i] /= repeat;
        }
    }

    template <typename META>
    f64 SketchTest<META>::ALE(u32 model) const {
        return m_ALE[model];
    }

    template <typename META>
    f64 SketchTest<META>::APE(u32 model) const {
        return m_APE[model];
    }

    template <typename META>
    f64 SketchTest<META>::appendTp(u32 model) const {
        return m_appendTp[model];
    }

    template <typename META>
    f64 SketchTest<META>::queryTp(u32 model) const {
        return m_queryTp[model];
    }
}   // namespace sketch
