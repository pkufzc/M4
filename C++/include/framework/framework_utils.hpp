#pragma once
#include "../common/sketch_defs.hpp"
#include "../meta/dd/ddsketch.hpp"
#include "../meta/mreq/mreq_sketch.hpp"
#include "../meta/tdigest/tdigest.hpp"
#include "../meta/dd_collapse/ddsketch_collapse.hpp"

namespace sketch {
#ifdef TEST_DD
    DDSketch createMeta(u32 cap, f64 alpha, u32, u32, double) {
        return DDSketch(cap, alpha);
    }
#elif defined(TEST_MREQ)
    mReqSketch createMeta(u32 cap, f64, u32 cmtor_cap, u32, double) {
        return mReqSketch(cap, cmtor_cap);
    }
#elif defined(TEST_TD)
    TDigest createMeta(u32 cap, f64, u32, u32 delta, double) {
        return TDigest(cap, delta);
    }
#elif defined(TEST_DDC)
    DDCSketch createMeta(u32 cap, f64 alpha, u32, u32, double ddc_alpha) {
        return DDCSketch(cap, alpha, ddc_alpha);
    }
#endif
}   // namespace sketch
