#include "logic.h"
#include "logic16_c.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Logic {

Processor16 *and16_stacked_c      = &logic16_stacked_t<and16_c>;
Processor16 *or16_stacked_c       = &logic16_stacked_t<or16_c>;
Processor16 *andn16_stacked_c     = &logic16_stacked_t<andn16_c>;
Processor16 *xor16_stacked_c      = &logic16_stacked_t<xor16_c>;

Processor16 *and16_native_c  = &logic16_native_t<and16_c>;
Processor16 *or16_native_c   = &logic16_native_t<or16_c>;
Processor16 *andn16_native_c = &logic16_native_t<andn16_c>;
Processor16 *xor16_native_c  = &logic16_native_t<xor16_c>;

#define DEFINE_SILLY_C_MODE(mode, layout) \
    Processor16 *mode##_##layout##_c         = &logic16_##layout##_t<mode##_t<nop16_c, nop16_c>>; \
    Processor16 *mode##sub_##layout##_c      = &logic16_##layout##_t<mode##_t<nop16_c, sub16_c>>; \
    Processor16 *mode##add10_##layout##_c   = &logic16_##layout##_t<mode##_t<nop16_c, add16_c<10>>>; \
    Processor16 *mode##add12_##layout##_c   = &logic16_##layout##_t<mode##_t<nop16_c, add16_c<12>>>; \
    Processor16 *mode##add14_##layout##_c   = &logic16_##layout##_t<mode##_t<nop16_c, add16_c<14>>>; \
    Processor16 *mode##add16_##layout##_c   = &logic16_##layout##_t<mode##_t<nop16_c, add16_c<16>>>; \
    Processor16 *sub##mode##_##layout##_c    = &logic16_##layout##_t<mode##_t<sub16_c, nop16_c>>; \
    Processor16 *sub##mode##sub_##layout##_c = &logic16_##layout##_t<mode##_t<sub16_c, sub16_c>>; \
    Processor16 *sub##mode##add10_##layout##_c = &logic16_##layout##_t<mode##_t<sub16_c, add16_c<10>>>; \
    Processor16 *sub##mode##add12_##layout##_c = &logic16_##layout##_t<mode##_t<sub16_c, add16_c<12>>>; \
    Processor16 *sub##mode##add14_##layout##_c = &logic16_##layout##_t<mode##_t<sub16_c, add16_c<14>>>; \
    Processor16 *sub##mode##add16_##layout##_c = &logic16_##layout##_t<mode##_t<sub16_c, add16_c<16>>>; \
    Processor16 *add10##mode##_##layout##_c    = &logic16_##layout##_t<mode##_t<add16_c<10>, nop16_c>>; \
    Processor16 *add12##mode##_##layout##_c    = &logic16_##layout##_t<mode##_t<add16_c<12>, nop16_c>>; \
    Processor16 *add14##mode##_##layout##_c    = &logic16_##layout##_t<mode##_t<add16_c<14>, nop16_c>>; \
    Processor16 *add16##mode##_##layout##_c    = &logic16_##layout##_t<mode##_t<add16_c<16>, nop16_c>>; \
    Processor16 *add10##mode##sub_##layout##_c = &logic16_##layout##_t<mode##_t<add16_c<10>, sub16_c>>; \
    Processor16 *add12##mode##sub_##layout##_c = &logic16_##layout##_t<mode##_t<add16_c<12>, sub16_c>>; \
    Processor16 *add14##mode##sub_##layout##_c = &logic16_##layout##_t<mode##_t<add16_c<14>, sub16_c>>; \
    Processor16 *add16##mode##sub_##layout##_c = &logic16_##layout##_t<mode##_t<add16_c<16>, sub16_c>>; \
    Processor16 *add10##mode##add10_##layout##_c = &logic16_##layout##_t<mode##_t<add16_c<10>, add16_c<10>>>; \
    Processor16 *add12##mode##add12_##layout##_c = &logic16_##layout##_t<mode##_t<add16_c<12>, add16_c<12>>>; \
    Processor16 *add14##mode##add14_##layout##_c = &logic16_##layout##_t<mode##_t<add16_c<14>, add16_c<14>>>; \
    Processor16 *add16##mode##add16_##layout##_c = &logic16_##layout##_t<mode##_t<add16_c<16>, add16_c<16>>>;

DEFINE_SILLY_C_MODE(min, stacked)
DEFINE_SILLY_C_MODE(min, native)
DEFINE_SILLY_C_MODE(max, stacked)
DEFINE_SILLY_C_MODE(max, native)
#undef DEFINE_SILLY_C_MODE

} } } }