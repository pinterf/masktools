#include "deflate.h"

#ifdef NEON_INTRINSICS

#include "../functions_neon.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Morphologic { namespace Deflate {

Processor *deflate_neon = &morpho8_neon<DeflateOp>;
Processor16 *deflate_16_neon = &morpho16_neon<DeflateOp>;

} } } } }

#endif // NEON_INTRINSICS
