#pragma once
#include "dleft_sketch.hpp"
#include <type_traits>
#include <algorithm>
#include "../framework_utils.hpp"

namespace sketch {
    template <typename META>
    DLeftSketch<META>::DLeftSketch(u64 mem_limit, u32 seed, double ddc_alpha) {
        ddcAlpha = ddc_alpha;
        dft = createMeta(UINT32_MAX, alpha, cmtor_cap, td_cap, ddc_alpha);
        u32 bucket_num = mem_limit / (dft.memory() + sizeof(u32)) / HASH_NUM;
        for (u32 i = 0; i < HASH_NUM; ++i) {
            buckets[i] = vector<META>(bucket_num);
            ids[i] = vector<u32>(bucket_num);
            for (u32 j = 0; j < bucket_num; ++j) {
                evict(i, j);
            }
            hash[i].initialize(seed + i);
        }

        dft = createMeta(UINT32_MAX, 0.5, 2, 4, ddc_alpha);
    }

    template <typename META>
    void DLeftSketch<META>::evict(u32 bucket_id, u32 pos) {
        auto& sketch = buckets[bucket_id][pos];
        sketch = createMeta(UINT32_MAX, alpha, cmtor_cap, td_cap, ddcAlpha);
        ids[bucket_id][pos] = UINT32_MAX;
    }

    template <typename META>
    void DLeftSketch<META>::append(u32 id, u32 value) {
        min_item = std::min(min_item, value);
        max_item = std::max(max_item, value);
        dft.append(value);

        u32 tmp[HASH_NUM];
        for (u32 i = 0; i < HASH_NUM; ++i) {
            tmp[i] = pos(i, id);
            if (ids[i][tmp[i]] == id) {
                buckets[i][tmp[i]].append(value);
                return;
            }
        }

        for (u32 i = 0; i < HASH_NUM; ++i) {
            if (ids[i][tmp[i]] == UINT32_MAX) {
                ids[i][tmp[i]] = id;
                buckets[i][tmp[i]].append(value);
                return;
            }
        }

        u32 bucket_id = gen();
        u32 tmp_id = tmp[bucket_id];
        evict(bucket_id, tmp_id);
        ids[bucket_id][tmp_id] = id;
        buckets[bucket_id][tmp_id].append(value);
    }

    template <typename META>
    u32 DLeftSketch<META>::quantile(u32 id, f64 nom_rank) const {
        for (u32 i = 0; i < HASH_NUM; ++i) {
            u32 tmp = pos(i, id);
            if (ids[i][tmp] == id) {
                return buckets[i][tmp].quantile(nom_rank);
            }
        }

        return dft.quantile(nom_rank);
    }

    template <typename META>
    u32 DLeftSketch<META>::pos(u32 bucket_id, u32 id) const {
        return hash[bucket_id].run(id) % buckets[bucket_id].size();
    }

    template <typename META>
    u32 DLeftSketch<META>::memory() const {
        // This function will not be called.
        assert(false);
        return 0;
    }

    template <typename META>
    u32 DLeftSketch<META>::size(u32 id) const {
        // This function will not be called.
        assert(false);
        return 0;
    }
}   // namespace sketch
