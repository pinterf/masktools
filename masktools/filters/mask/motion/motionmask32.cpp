#include "motionmask.h"
#include "motionmask32_c.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Mask { namespace Motion {

Processor32 *mask32_c = &mask32_t<sad32_c_op, mask32_c_op>;

} } } } }