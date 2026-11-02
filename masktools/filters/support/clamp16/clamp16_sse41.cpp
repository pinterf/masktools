#include "../clamp/clamp.h"

#ifdef INTEL_INTRINSICS

#include "clamp16_sse.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Support  { namespace Clamp {

Processor16 *clamp16_native_sse4_1 = &clamp16_native_simd<CPU_SSE4_1>;

} } } } }

#endif // INTEL_INTRINSICS
