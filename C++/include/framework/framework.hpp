#pragma once
#include "../common/sketch_defs.hpp"

namespace sketch {
    class Framework {
    public:
        /// @brief Destructor.
        virtual ~Framework() = default;

        /// @brief Append a given item into the sketch.
        /// @param id Item ID.
        /// @param value Item value.
        virtual void append(u32 id, u32 value) = 0;

        /// @brief Estimate the size of a given flow.
        /// @param id Flow ID.
        virtual u32 size(u32 id) const = 0;

        /// @brief Return number of bytes the whole sketch uses.
        virtual u32 memory() const = 0;

        /// @brief Estimate the quantile value of a given normalized rank.
        /// @param id Item ID.
        /// @param nom_rank Normalized rank.
        virtual u32 quantile(u32 id, f64 nom_rank) const = 0;

        /// @brief Return the type of a given flow.
        /// @param id Flow ID.
        virtual FlowType type(u32 id) const {
            u32 sz = size(id);
            if (sz <= 3) {
                return FlowType::TINY;
            } else if (sz <= 255) {
                return FlowType::MID;
            } else {
                return FlowType::HUGE;
            }
        }
    };
} // namespace sketch
