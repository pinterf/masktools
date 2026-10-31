#include "logic.h"
#include "logic32_c.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Logic {

Processor32 *and_32_c  = &logic_t<_and>;
Processor32 *or_32_c   = &logic_t<_or>;
Processor32 *andn_32_c = &logic_t<andn>;
Processor32 *xor_32_c  = &logic_t<_xor>;

#define DEFINE_C_VERSIONS(mode) \
    Processor32 *mode##_32_c         = &logic_t<mode##_t<nop, nop>>; \
    Processor32 *mode##sub_32_c      = &logic_t<mode##_t<nop, sub>>; \
    Processor32 *mode##add_32_c      = &logic_t<mode##_t<nop, add>>; \
    Processor32 *sub##mode##_32_c    = &logic_t<mode##_t<sub, nop>>; \
    Processor32 *sub##mode##sub_32_c = &logic_t<mode##_t<sub, sub>>; \
    Processor32 *sub##mode##add_32_c = &logic_t<mode##_t<sub, add>>; \
    Processor32 *add##mode##_32_c    = &logic_t<mode##_t<add, nop>>; \
    Processor32 *add##mode##sub_32_c = &logic_t<mode##_t<add, sub>>; \
    Processor32 *add##mode##add_32_c = &logic_t<mode##_t<add, add>>;

DEFINE_C_VERSIONS(min);
DEFINE_C_VERSIONS(max);
#undef DEFINE_C_VERSIONS

} } } }