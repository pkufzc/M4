#pragma once
#include "cuckoo.hpp"
#include "../framework_utils.hpp"
#include "../../common/sketch_utils.hpp"

namespace sketch {
    template <typename META>
    Cuckoo<META>::Cuckoo(u64 mem_limit, u32 seed, double ddc_alpha) {
        rand_u32_generator gen(seed, MAX_PRIME32 - 1);
        for (u32 i = 0; i < HASH_NUM; ++i) {
            h[i].initialize(gen());
        }

        META meta = createMeta(UINT32_MAX, alpha, cmtor_cap, td_cap, ddc_alpha);
        Cell cell{UINT32_MAX, meta};
        u32 num = mem_limit / cell.memory();
        buckets = std::vector<Cell>(num, cell);
        dft = createMeta(UINT32_MAX, 0.5, 2, 4, ddc_alpha);
    }

    template <typename META>
    void Cuckoo<META>::append(u32 id, u32 value) {
        dft.append(value);

        u32 idx[HASH_NUM];
        for (u32 i = 0; i < HASH_NUM; ++i) {
            idx[i] = pos(id, i);
        }
        if (idx[0] == idx[1]) { // Relies on HASH_NUM == 2.
            return;
        }

        for (u32 i = 0; i < HASH_NUM; ++i) {
            u32 bucket_id = buckets[idx[i]].id;
            if (bucket_id == id) {
                buckets[idx[i]].meta.append(value);
                return;
            }
        }

        for (u32 i = 0; i < HASH_NUM; ++i) {
            u32& bucket_id = buckets[idx[i]].id;
            if (bucket_id == UINT32_MAX) {
                buckets[idx[i]].meta.append(value);
                bucket_id = id;
                return;
            }
        }

        rand_u32_generator gen(0, HASH_NUM - 1);
        u32 bucket_idx = idx[gen()];
        if (preempt(id, bucket_idx, MAX_TURN_NUM)) {
            buckets[bucket_idx].meta.append(value);
            buckets[bucket_idx].id = id;
        }
    }

    template <typename META>
    u32 Cuckoo<META>::size(u32 id) const {
        // This function will not be called.
        assert(false);
        return 0;
    }

    template <typename META>
    u32 Cuckoo<META>::memory() const {
        // This function will not be called.
        assert(false);
        return 0;
    }

    template <typename META>
    u32 Cuckoo<META>::quantile(u32 id, f64 nom_rank) const {
        for (u32 i = 0; i < HASH_NUM; ++i) {
            u32 tmp = pos(id, i);
            if (buckets[tmp].id == id) {
                return buckets[tmp].meta.quantile(nom_rank);
            }
        }

        return dft.quantile(nom_rank);
    }

    template <typename META>
    bool Cuckoo<META>::preempt(u32 id, u32 bucket_idx, u32 turns_left) {
        if (turns_left == 0) {
            return false;
        }

        u32 kick_id = buckets[bucket_idx].id;
        u32 another_bucket_idx = 0;
        for (u32 i = 0; i < HASH_NUM; ++i) {
            u32 tmp = pos(kick_id, i);
            if (tmp != bucket_idx) { // Relies on HASH_NUM == 2.
                assert(buckets[tmp].id != kick_id);
                another_bucket_idx = tmp;
                break;
            }
        }


        bool success = buckets[another_bucket_idx].id == UINT32_MAX
            || preempt(kick_id, another_bucket_idx, turns_left - 1);

        if (success) {
            std::swap(buckets[bucket_idx], buckets[another_bucket_idx]);
        }

        return success;
    }

    template <typename META>
    u32 Cuckoo<META>::pos(u32 id, u32 h_idx) const {
        return h[h_idx].run(id) % buckets.size();
    }
}
