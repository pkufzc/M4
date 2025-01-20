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
                                             const vector<FlowItem>& dataset,
                                             double ddc_alpha) {
        models[ANDOR] = new AndorSketch<META>(mem_limit, hash_num, seed, ddc_alpha);
        models[DLEFT] = new DLeftSketch<META>(mem_limit, seed, ddc_alpha);
        models[CUCKOO] = new Cuckoo<META>(mem_limit, seed, ddc_alpha);
        
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

    // 新增：计算 AAE（平均绝对误差）
    template <typename META>
    f64 SketchSingleTest<META>::AAE(u32 model, u32 type) const {
        u32 flow_cnt = 0;
        f64 error = 0;
        for (u32 id : id_list) {
            if ((getType(id) & type) == 0) {
                continue;
            }
            error += FlowAAE(models[model], id);
            ++flow_cnt;
        }
        return error / flow_cnt;
    }

    // 新增：计算 ARE（平均相对误差）
    template <typename META>
    f64 SketchSingleTest<META>::ARE(u32 model, u32 type) const {
        u32 flow_cnt = 0;
        f64 error = 0;
        for (u32 id : id_list) {
            if ((getType(id) & type) == 0) {
                continue;
            }
            error += FlowARE(models[model], id);
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

    // 新增：计算单个流的 AAE（绝对误差）
    template <typename META>
    f64 SketchSingleTest<META>::FlowAAE(const Framework* sketch, u32 id) const {
        f64 quan = sketch->quantile(id, given_p);
        f64 quan_real = real.quantile(id, given_p);
        return std::fabs(quan - quan_real);
    }

    // 新增：计算单个流的 ARE（相对误差）
    template <typename META>
    f64 SketchSingleTest<META>::FlowARE(const Framework* sketch, u32 id) const {
        f64 quan = sketch->quantile(id, given_p);
        f64 quan_real = real.quantile(id, given_p);
        if (quan_real == 0) {
            return 0; // 避免除以零
        }
        return std::fabs((quan - quan_real) / quan_real);
    }

    template <typename META>
    SketchTest<META>::SketchTest(u64 mem_limit_, u32 hash_num_, u32 seed_,
                         const string& dataset_name_,
                         u32 repeat_, double ddc_alpha_)
        : mem_limit(mem_limit_), hash_num(hash_num_), seed(seed_), 
          dataset(dataset_name_), repeat(repeat_), ddc_alpha(ddc_alpha_) { }

    template <typename META>
    void SketchTest<META>::run() {
        auto dataset_loaded = load_dataset(dataset);

        cout << "mem_limit: " << (mem_limit / 1024) << "KB" << endl;
        
        for (u32 i = 0; i < repeat; ++i) {
            cout << "Running test " << i << "..." << endl;
            SketchSingleTest<META> test(mem_limit, hash_num,
                                        seed, dataset_loaded, ddc_alpha);
            cout << "Calculating metrics..." << endl;
            addMetrics(test);
        }

        summarize();
        cout << "Test finished" << endl;
    }

    template <typename META>
    void SketchTest<META>::addMetrics(const SketchSingleTest<META>& test) {
        for (u32 i = 0; i < NUM_MODELS; ++i) {
            m_ALE[i] += test.ALE(i, MID | HUGE);
            m_APE[i] += test.APE(i, MID | HUGE);
            m_AAE[i] += test.AAE(i, MID | HUGE); // 新增：累加 AAE
            m_ARE[i] += test.ARE(i, MID | HUGE); // 新增：累加 ARE
            m_appendTp[i] += test.appendTp(i);
            m_queryTp[i] += test.queryTp(i);
        }
    }

    template <typename META>
    void SketchTest<META>::summarize() {
        for (i32 i = 0; i < NUM_MODELS; ++i) {
            m_ALE[i] /= repeat;
            m_APE[i] /= repeat;
            m_AAE[i] /= repeat; // 新增：计算平均 AAE
            m_ARE[i] /= repeat; // 新增：计算平均 ARE
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

    // 新增：获取 AAE
    template <typename META>
    f64 SketchTest<META>::AAE(u32 model) const {
        return m_AAE[model];
    }

    // 新增：获取 ARE
    template <typename META>
    f64 SketchTest<META>::ARE(u32 model) const {
        return m_ARE[model];
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