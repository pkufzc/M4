#pragma once
#include <unordered_set>
#include "../common/real_dist.hpp"
#include "../framework/framework.hpp"

namespace sketch {
    template <typename META>
    class SketchSingleTest {
    public:
        /// @brief Constructor.
        /// @param mem_limit Memory limit in bytes.
        /// @param hash_num Number of hash functions per level.
        /// @param seed Seed for generating hash functions.
        /// @param dataset Dataset to be tested.
        SketchSingleTest(u64 mem_limit, u32 hash_num, u32 seed,
                         const vector<FlowItem>& dataset, double ddc_alpha_);

        ~SketchSingleTest();

        /// @brief Calculate ALE of a given model on a given flow type.
        f64 ALE(u32 model, u32 type) const;
        /// @brief Calculate APE of a given model on a given flow type.
        f64 APE(u32 model, u32 type) const;
        f64 AAE(u32 model, u32 type) const;
        f64 ARE(u32 model, u32 type) const;
        /// @brief Calculate appending throughput of a given model in Mops.
        f64 appendTp(u32 model) const;
        /// @brief Calculate query throughput of a given model in Mops.
        f64 queryTp(u32 model) const;

    private:
        Framework* models[NUM_MODELS];  ///< Sketch models.
        real_dist real;                 ///< Real distribution.

        std::unordered_set<u32> id_list;    ///< List of flow id.

        f64 append_tp[NUM_MODELS];     ///< Appending throughput.

        /// Given percentage, used when calculating ALE and APE.
        static constexpr f64 given_p = 0.5;

        /// @brief Return the type of a given flow.
        /// @param id Flow ID.
        FlowType getType(u32 id) const;

        f64 FlowALE(const Framework* sketch, u32 id) const;

        f64 FlowAPE(const Framework* sketch, u32 id) const;

        f64 FlowAAE(const Framework* sketch, u32 id) const;

        f64 FlowARE(const Framework* sketch, u32 id) const;
    };

    template <typename META>
    class SketchTest {
    public:
        /// @brief Constructor.
        /// @param mem_limit_ Memory limit in bytes.
        /// @param hash_num_ Number of hash functions per level,
        ///                 used only by andor model.
        /// @param seed_ Seed for generating hash functions.
        /// @param dataset_ Dataset to be tested.
        /// @param repeat_time_ Number of times to repeat the test.
        SketchTest(u64 mem_limit_, u32 hash_num_, u32 seed_,
                  const string& dataset_,
                  u32 repeat_time_, double ddc_alpha_);

        /// @brief Run the test.
        void run();

        /// @brief Calculate ALE of a given model.
        f64 ALE(u32 model) const;
        /// @brief Calculate APE of a given model.
        f64 APE(u32 model) const;
        f64 AAE(u32 model) const;
        f64 ARE(u32 model) const;
        /// @brief Calculate appending throughput of a given model in Mops.
        f64 appendTp(u32 model) const;
        /// @brief Calculate query throughput of a given model in Mops.
        f64 queryTp(u32 model) const;

    private:
        u64 mem_limit;      ///< Memory limit.
        u32 hash_num;       ///< Number of hash functions per level.
        u32 seed;           ///< Seed for generating hash functions.
        string dataset;     ///< Dataset to be tested.
        u32 repeat;         ///< Number of times to repeat the test.
        double ddc_alpha;

        f64 m_ALE[NUM_MODELS], m_APE[NUM_MODELS], m_AAE[NUM_MODELS], m_ARE[NUM_MODELS];
        f64 m_appendTp[NUM_MODELS], m_queryTp[NUM_MODELS];

        void addMetrics(const SketchSingleTest<META>& test);
        void summarize();
    };
}   // namespace sketch

#include "sketch_test_impl.hpp"
