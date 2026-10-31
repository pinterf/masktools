#include "../makediff/makediff.h"

#ifdef INTEL_INTRINSICS

#include "makediff16_sse.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Support  { namespace MakeDiff {

#pragma warning(disable: 4309)


#define MAKE_EXPORTS(bits_per_pixel) \
Processor16 *makediff16_native_##bits_per_pixel##_sse2 = &makediff16_native_simd<CPU_SSE2,bits_per_pixel>;

MAKE_EXPORTS(10)
MAKE_EXPORTS(12)
MAKE_EXPORTS(14)
MAKE_EXPORTS(16)
#undef MAKE_EXPORTS

} } } } }

#endif // INTEL_INTRINSICS
