#include "../adddiff/adddiff.h"

#ifdef INTEL_INTRINSICS

#include "adddiff16_sse.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Support  { namespace AddDiff {

Processor16 *adddiff16_native_sse2 = &adddiff16_native_simd<CPU_SSE2>;

} } } } }

#endif // INTEL_INTRINSICS
