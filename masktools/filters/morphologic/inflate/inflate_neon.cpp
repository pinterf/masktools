#include "inflate.h"

#ifdef NEON_INTRINSICS

#include "../functions_neon.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Morphologic { namespace Inflate {

Processor *inflate_neon = &morpho8_neon<InflateOp>;
Processor16 *inflate_16_neon = &morpho16_neon<InflateOp>;

} } } } }

#endif // NEON_INTRINSICS
