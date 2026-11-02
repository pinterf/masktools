#include "../makediff/makediff.h"

#ifdef INTEL_INTRINSICS

#include "makediff16_sse.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Support  { namespace MakeDiff {

Processor16 *makediff16_native_sse4_1 = &makediff16_native_simd<CPU_SSE4_1>;

} } } } }

#endif // INTEL_INTRINSICS
