#include "logic.h"
#include "logic_c.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Logic {

Processor *and_c  = &logic_t<_and>;
Processor *or_c   = &logic_t<_or>;
Processor *andn_c = &logic_t<andn>;
Processor *xor_c  = &logic_t<_xor>;

#define DEFINE_C_VERSIONS(mode) \
    Processor *mode##_c         = &logic_t<mode##_t<nop, nop>>; \
    Processor *mode##sub_c      = &logic_t<mode##_t<nop, sub>>; \
    Processor *mode##add_c      = &logic_t<mode##_t<nop, add>>; \
    Processor *sub##mode##_c    = &logic_t<mode##_t<sub, nop>>; \
    Processor *sub##mode##sub_c = &logic_t<mode##_t<sub, sub>>; \
    Processor *sub##mode##add_c = &logic_t<mode##_t<sub, add>>; \
    Processor *add##mode##_c    = &logic_t<mode##_t<add, nop>>; \
    Processor *add##mode##sub_c = &logic_t<mode##_t<add, sub>>; \
    Processor *add##mode##add_c = &logic_t<mode##_t<add, add>>;

DEFINE_C_VERSIONS(min);
DEFINE_C_VERSIONS(max);
#undef DEFINE_C_VERSIONS

} } } }