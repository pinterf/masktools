#include "inflate.h"

#ifdef INTEL_INTRINSICS

#include "../functions16_avx2.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Morphologic { namespace Inflate {

Processor16 *inflate_avx2_16 = &generic_16_avx2<
  process_line_xxflate_16_avx2<Border::Left, limit_up_16_avx2>,
  process_line_xxflate_16_avx2<Border::None, limit_up_16_avx2>,
  process_line_xxflate_16_avx2<Border::Right, limit_up_16_avx2>
>;

} } } } }

#endif // INTEL_INTRINSICS
