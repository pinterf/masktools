#include "logic.h"
#include "logic16_c.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Logic {


Processor16 *and16_native_c  = &logic16_native_t<and16_c>;
Processor16 *or16_native_c   = &logic16_native_t<or16_c>;
Processor16 *andn16_native_c = &logic16_native_t<andn16_c>;
Processor16 *xor16_native_c  = &logic16_native_t<xor16_c>;

#define LOGIC16_C(name, mode, A, B) \
    Processor16 *name##_native_c = &logic16_native_t<mode##_t<A##16_c, B##16_c>>;

#define DEFINE_C_MODES(mode) \
    LOGIC16_C(mode, mode, nop, nop) \
    LOGIC16_C(mode##sub, mode, nop, sub) \
    LOGIC16_C(mode##add, mode, nop, add) \
    LOGIC16_C(sub##mode, mode, sub, nop) \
    LOGIC16_C(sub##mode##sub, mode, sub, sub) \
    LOGIC16_C(sub##mode##add, mode, sub, add) \
    LOGIC16_C(add##mode, mode, add, nop) \
    LOGIC16_C(add##mode##sub, mode, add, sub) \
    LOGIC16_C(add##mode##add, mode, add, add)

DEFINE_C_MODES(min)
DEFINE_C_MODES(max)
#undef DEFINE_C_MODES
#undef LOGIC16_C

} } } }