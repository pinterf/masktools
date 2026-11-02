#include "../adddiff/adddiff.h"

#ifdef INTEL_INTRINSICS

#include "adddiff16_sse.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Support  { namespace AddDiff {

Processor16 *adddiff16_native_sse4_1 = &adddiff16_native_simd<CPU_SSE4_1>;

} } } } }

#endif // INTEL_INTRINSICS
