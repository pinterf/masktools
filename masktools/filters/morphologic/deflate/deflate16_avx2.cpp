#include "deflate.h"

#ifdef INTEL_INTRINSICS

#include "../functions16_avx2.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Morphologic { namespace Deflate {

Processor16 *deflate_avx2_16 = &generic_16_avx2<
  process_line_xxflate_16_avx2<Border::Left, limit_down_16_avx2>,
  process_line_xxflate_16_avx2<Border::None, limit_down_16_avx2>,
  process_line_xxflate_16_avx2<Border::Right, limit_down_16_avx2>
>;

} } } } }

#endif // INTEL_INTRINSICS
