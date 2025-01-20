#pragma once
#include "../../common/BOBHash32.h"
#include "../../common/sketch_utils.hpp"
#include "../framework.hpp"

namespace sketch {
    template <typename META>
    class DLeftSketch : public Framework {
    public:
        /// @brief Constructor.
        /// @param mem_limit Memory limit in bytes.
        /// @param seed Seed for generating hash functions, by default 0.
        DLeftSketch(u64 mem_limit, u32 seed = 0, double ddc_alpha = 0.1);

        void append(u32 id, u32 value) override;
        u32 size(u32 id) const override;
        u32 memory() const override;
        u32 quantile(u32 id, f64 nom_rank) const override;

    private:
        double ddcAlpha = 0.1;
        static constexpr f64 alpha = 0.3;
        static constexpr u32 cmtor_cap = 4;
        static constexpr u32 td_cap = 16;
        static constexpr u32 ddc_size = 35;

        static constexpr u32 HASH_NUM = 3;

        vector<META> buckets[HASH_NUM];          ///< Buckets.
        META dft;                                ///< Default bucket.
        vector<u32> ids[HASH_NUM];               ///< Flow IDs.
        BOBHash32 hash[HASH_NUM];                ///< Hash functions.
        rand_u32_generator gen{0, HASH_NUM - 1}; ///< Random number generator.

        u32 min_item = UINT32_MAX;               ///< Minimum item.
        u32 max_item = 0;                        ///< Maximum item.


        /// @brief Return the bucket position of a given item.
        /// @param id Item ID.
        u32 pos(u32 bucket_id, u32 id) const;

        void evict(u32 bucket_id, u32 pos);
    };
}   // namespace sketch

#include "dleft_sketch_impl.hpp"
