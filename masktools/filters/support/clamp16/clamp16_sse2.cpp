#include "../clamp/clamp.h"

#ifdef INTEL_INTRINSICS

#include "clamp16_sse.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Support  { namespace Clamp {

Processor16 *clamp16_native_sse2 = &clamp16_native_simd<CPU_SSE2>;

} } } } }

#endif // INTEL_INTRINSICS
