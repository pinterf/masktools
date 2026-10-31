#include "../clamp/clamp.h"

#ifdef INTEL_INTRINSICS

#include "clamp16_sse.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Support  { namespace Clamp {

Processor16 *clamp16_stacked_sse4_1 = &clamp16_stacked_simd<CPU_SSE4_1>;
Processor16 *clamp16_native_10_sse4_1 = &clamp16_native_simd<CPU_SSE4_1,10>;
Processor16 *clamp16_native_12_sse4_1 = &clamp16_native_simd<CPU_SSE4_1, 12>;
Processor16 *clamp16_native_14_sse4_1 = &clamp16_native_simd<CPU_SSE4_1, 14>;
Processor16 *clamp16_native_16_sse4_1 = &clamp16_native_simd<CPU_SSE4_1, 16>;

} } } } }

#endif // INTEL_INTRINSICS
