#pragma once
#include "../../common/sketch_defs.hpp"
#include "../../common/histogram.hpp"

namespace sketch {
        template <typename META>
        class AndorSketch;
    class DDCSketch {
        friend class AndorSketch<DDCSketch>;
    public:
        /// @brief Default constructor.
        /// @warning Members are potential uninitialized after construction.
        ///          Make sure you know what you are doing.
        DDCSketch() = default;

        /// @brief Constructor.
        /// @param cap_ Capacity, i.e. maximum number of items that
        ///             can be held in the DDSketch.
        /// @param alpha_ Argument for interval division.
        DDCSketch::DDCSketch(u32 cap_, f64 alpha_, double ddc_alpha);

        /// @brief Destructor.
        ~DDCSketch() = default;

        /// @brief Return the number of items in the DDSketch.
        u32 size() const;
        /// @brief Return the capacity of the DDSketch.
        u32 capacity() const;

        /// @brief Return whether the DDSketch is empty.
        bool empty() const;
        /// @brief Return whether the DDSketch is full.
        bool full() const;

        /// @brief Return the number of bytes the DDSketch uses.
        u32 memory() const;

        /// @brief Append an item to the DDSketch.
        /// @param item The item to append.
        void append(u32 item);

        /// @brief Estimate the quantile value of a given normalized rank.
        u32 quantile(f64 nom_rank) const;

        /// @brief Convert the DDSketch to a histogram.
        operator Histogram() const;

    private:
        vec_u32 counters;       ///< Counters.
        u32 totalSize = 0;      ///< Total number of items.
        u32 maxCnt = 0;         ///< Maximum counter.
        u32 cap;                ///< Capacity.
        f64 alpha;              ///< Argument for interval division.
        f64 gamma;              ///< gamma = (1 + alpha) / (1 - alpha).

        /// @brief Return the index of an item in @c counters.
        u8 pos(u32 item) const;
        u8 find_id(u8 posx) const;
        void insert_id(pair32 counter, u8 idx);
        

        /// @brief Append an item to given position.
        void append(u32 item, u32 pos);
    };
}   // namespace sketch

#include "ddsketch_impl.hpp"