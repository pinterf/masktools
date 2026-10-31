#include "motionmask.h"
#include "motionmask16_c.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Mask { namespace Motion {

Processor16* mask10_c = &mask16_t<sad16_c_op, mask16_c_op<10>>;
Processor16* mask12_c = &mask16_t<sad16_c_op, mask16_c_op<12>>;
Processor16* mask14_c = &mask16_t<sad16_c_op, mask16_c_op<14>>;
Processor16* mask16_c = &mask16_t<sad16_c_op, mask16_c_op<16>>;

} } } } }