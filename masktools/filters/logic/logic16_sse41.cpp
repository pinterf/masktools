#include "logic.h"

#ifdef INTEL_INTRINSICS

#include "logic16_sse.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Logic {

// min, max and the modes with add need SSE4.1 (_mm_min_epu16, _mm_max_epu16), the _sse2 names are kept

#define DEFINE_SILLY_SSE2_VERSIONS(mode, layout) \
    Processor16 *mode##_##layout##_sse2         = &logic16_##layout##_t_sse2<mode##_t_sse2<nop16_sse2, nop16_sse2>, mode##_t<nop16_c, nop16_c>>; \
    Processor16 *mode##sub_##layout##_sse2      = &logic16_##layout##_t_sse2<mode##_t_sse2<nop16_sse2, sub16_sse2>, mode##_t<nop16_c, sub16_c>>; \
    Processor16 *mode##add10_##layout##_sse2   = &logic16_##layout##_t_sse2<mode##_t_sse2<nop16_sse2, add16_sse2<10>>, mode##_t<nop16_c, add16_c<10>>>; \
    Processor16 *mode##add12_##layout##_sse2   = &logic16_##layout##_t_sse2<mode##_t_sse2<nop16_sse2, add16_sse2<12>>, mode##_t<nop16_c, add16_c<12>>>; \
    Processor16 *mode##add14_##layout##_sse2   = &logic16_##layout##_t_sse2<mode##_t_sse2<nop16_sse2, add16_sse2<14>>, mode##_t<nop16_c, add16_c<14>>>; \
    Processor16 *mode##add16_##layout##_sse2   = &logic16_##layout##_t_sse2<mode##_t_sse2<nop16_sse2, add16_sse2<16>>, mode##_t<nop16_c, add16_c<16>>>; \
    Processor16 *sub##mode##_##layout##_sse2    = &logic16_##layout##_t_sse2<mode##_t_sse2<sub16_sse2, nop16_sse2>, mode##_t<sub16_c, nop16_c>>; \
    Processor16 *sub##mode##sub_##layout##_sse2 = &logic16_##layout##_t_sse2<mode##_t_sse2<sub16_sse2, sub16_sse2>, mode##_t<sub16_c, sub16_c>>; \
    Processor16 *sub##mode##add10_##layout##_sse2 = &logic16_##layout##_t_sse2<mode##_t_sse2<sub16_sse2, add16_sse2<10>>, mode##_t<sub16_c, add16_c<10>>>; \
    Processor16 *sub##mode##add12_##layout##_sse2 = &logic16_##layout##_t_sse2<mode##_t_sse2<sub16_sse2, add16_sse2<12>>, mode##_t<sub16_c, add16_c<12>>>; \
    Processor16 *sub##mode##add14_##layout##_sse2 = &logic16_##layout##_t_sse2<mode##_t_sse2<sub16_sse2, add16_sse2<14>>, mode##_t<sub16_c, add16_c<14>>>; \
    Processor16 *sub##mode##add16_##layout##_sse2 = &logic16_##layout##_t_sse2<mode##_t_sse2<sub16_sse2, add16_sse2<16>>, mode##_t<sub16_c, add16_c<16>>>; \
    Processor16 *add10##mode##_##layout##_sse2    = &logic16_##layout##_t_sse2<mode##_t_sse2<add16_sse2<10>, nop16_sse2>, mode##_t<add16_c<10>, nop16_c>>; \
    Processor16 *add12##mode##_##layout##_sse2    = &logic16_##layout##_t_sse2<mode##_t_sse2<add16_sse2<12>, nop16_sse2>, mode##_t<add16_c<12>, nop16_c>>; \
    Processor16 *add14##mode##_##layout##_sse2    = &logic16_##layout##_t_sse2<mode##_t_sse2<add16_sse2<14>, nop16_sse2>, mode##_t<add16_c<14>, nop16_c>>; \
    Processor16 *add16##mode##_##layout##_sse2    = &logic16_##layout##_t_sse2<mode##_t_sse2<add16_sse2<16>, nop16_sse2>, mode##_t<add16_c<16>, nop16_c>>; \
    Processor16 *add10##mode##sub_##layout##_sse2 = &logic16_##layout##_t_sse2<mode##_t_sse2<add16_sse2<10>, sub16_sse2>, mode##_t<add16_c<10>, sub16_c>>; \
    Processor16 *add12##mode##sub_##layout##_sse2 = &logic16_##layout##_t_sse2<mode##_t_sse2<add16_sse2<12>, sub16_sse2>, mode##_t<add16_c<12>, sub16_c>>; \
    Processor16 *add14##mode##sub_##layout##_sse2 = &logic16_##layout##_t_sse2<mode##_t_sse2<add16_sse2<14>, sub16_sse2>, mode##_t<add16_c<14>, sub16_c>>; \
    Processor16 *add16##mode##sub_##layout##_sse2 = &logic16_##layout##_t_sse2<mode##_t_sse2<add16_sse2<16>, sub16_sse2>, mode##_t<add16_c<16>, sub16_c>>; \
    Processor16 *add10##mode##add10_##layout##_sse2 = &logic16_##layout##_t_sse2<mode##_t_sse2<add16_sse2<10>, add16_sse2<10>>, mode##_t<add16_c<10>, add16_c<10>>>; \
    Processor16 *add12##mode##add12_##layout##_sse2 = &logic16_##layout##_t_sse2<mode##_t_sse2<add16_sse2<12>, add16_sse2<12>>, mode##_t<add16_c<12>, add16_c<12>>>; \
    Processor16 *add14##mode##add14_##layout##_sse2 = &logic16_##layout##_t_sse2<mode##_t_sse2<add16_sse2<14>, add16_sse2<14>>, mode##_t<add16_c<14>, add16_c<14>>>; \
    Processor16 *add16##mode##add16_##layout##_sse2 = &logic16_##layout##_t_sse2<mode##_t_sse2<add16_sse2<16>, add16_sse2<16>>, mode##_t<add16_c<16>, add16_c<16>>>;

DEFINE_SILLY_SSE2_VERSIONS(min, native)
DEFINE_SILLY_SSE2_VERSIONS(max, native)
#undef DEFINE_SILLY_SSE2_VERSIONS

} } } }

#endif // INTEL_INTRINSICS
