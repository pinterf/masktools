#include "motionmask.h"
#include "motionmask_c.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Mask { namespace Motion {

Processor *mask_c    = &mask_t<sad_c_op, mask_c_op>;

} } } } }