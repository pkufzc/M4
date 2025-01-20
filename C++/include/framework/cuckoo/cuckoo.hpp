#pragma once
#include "../../common/BOBHash32.h"
#include "../../common/sketch_defs.hpp"
#include "../framework.hpp"

namespace sketch {
    template <typename META>
    class Cuckoo : public Framework {
    public:
        /// @brief Constructor.
        /// @param mem_limit Memory limit in bytes.
        /// @param seed Seed for generating hash functions, by default 0.
        Cuckoo(u64 mem_limit, u32 seed = 0, double ddc_alpha = 0.1);

        void append(u32 id, u32 value) override;
        u32 size(u32 id) const override;
        u32 memory() const override;
        u32 quantile(u32 id, f64 nom_rank) const override;

    private:
        class Cell {
        public:
            u32 id;
            META meta;
            u32 memory() const {
                return sizeof(id) + meta.memory();
            }
        };

        static constexpr u32 MAX_TURN_NUM = 8;
        static constexpr u32 HASH_NUM = 2;
        static constexpr f64 alpha = 0.15;
        static constexpr u32 cmtor_cap = 8;
        static constexpr u32 td_cap = 32;
        static constexpr u32 ddc_size = 68;

        BOBHash32 h[HASH_NUM];
        std::vector<Cell> buckets;
        META dft;

        bool preempt(u32 id, u32 bucket_idx, u32 turns_left);
        u32 pos(u32 id, u32 h_idx) const;
    };
}

#include "cuckoo_impl.hpp"
