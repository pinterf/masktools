#include "../adddiff/adddiff.h"

#ifdef INTEL_INTRINSICS

#include "adddiff16_sse.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Support  { namespace AddDiff {

#pragma warning(disable: 4309)


#define MAKE_EXPORTS(bits_per_pixel) \
Processor16 *adddiff16_native_##bits_per_pixel##_sse2 = &adddiff16_native_simd<CPU_SSE2,bits_per_pixel>;

MAKE_EXPORTS(10)
MAKE_EXPORTS(12)
MAKE_EXPORTS(14)
MAKE_EXPORTS(16)
#undef MAKE_EXPORTS

} } } } }

#endif // INTEL_INTRINSICS
