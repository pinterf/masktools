#include "../clamp/clamp.h"

#ifdef INTEL_INTRINSICS

#include "clamp16_sse.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Support  { namespace Clamp {

Processor16 *clamp16_stacked_sse2 = &clamp16_stacked_simd<CPU_SSE2>;
Processor16 *clamp16_native_10_sse2 = &clamp16_native_simd<CPU_SSE2,10>;
Processor16 *clamp16_native_12_sse2 = &clamp16_native_simd<CPU_SSE2, 12>;
Processor16 *clamp16_native_14_sse2 = &clamp16_native_simd<CPU_SSE2, 14>;
Processor16 *clamp16_native_16_sse2 = &clamp16_native_simd<CPU_SSE2, 16>;

} } } } }

#endif // INTEL_INTRINSICS
