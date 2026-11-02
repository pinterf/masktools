#include "motionmask.h"
#include "motionmask16_c.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Mask { namespace Motion {

Processor16* mask16_c = &mask16_t<sad16_c_op, mask16_c_op>;

} } } } }