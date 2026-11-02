#include "logic.h"

#ifdef INTEL_INTRINSICS

#include "logic16_sse.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Logic {

// min, max and the modes with add need SSE4.1 (_mm_min_epu16, _mm_max_epu16), the _sse2 names are kept

#define LOGIC16_SSE2(name, mode, A, B) \
    Processor16 *name##_native_sse2 = &logic16_native_sse2<mode##_t_sse2<A##16_sse2<false>, B##16_sse2<false>>, \
                                                           mode##_t_sse2<A##16_sse2<true>, B##16_sse2<true>>, mode##_t<A##16_c, B##16_c>>;

#define DEFINE_SSE2_MODES(mode) \
    LOGIC16_SSE2(mode, mode, nop, nop) \
    LOGIC16_SSE2(mode##sub, mode, nop, sub) \
    LOGIC16_SSE2(mode##add, mode, nop, add) \
    LOGIC16_SSE2(sub##mode, mode, sub, nop) \
    LOGIC16_SSE2(sub##mode##sub, mode, sub, sub) \
    LOGIC16_SSE2(sub##mode##add, mode, sub, add) \
    LOGIC16_SSE2(add##mode, mode, add, nop) \
    LOGIC16_SSE2(add##mode##sub, mode, add, sub) \
    LOGIC16_SSE2(add##mode##add, mode, add, add)

DEFINE_SSE2_MODES(min)
DEFINE_SSE2_MODES(max)
#undef DEFINE_SSE2_MODES
#undef LOGIC16_SSE2

} } } }

#endif // INTEL_INTRINSICS
