#include "inflate.h"

#ifdef INTEL_INTRINSICS

#include "../functions16_sse.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Morphologic { namespace Inflate {

Processor16 *inflate_sse2_16 = &generic_sse2_16<
  process_line_xxflate_16<Border::Left, limit_up_sse2_16>,
  process_line_xxflate_16<Border::None, limit_up_sse2_16>,
  process_line_xxflate_16<Border::Right, limit_up_sse2_16>
>;

} } } } }

#endif // INTEL_INTRINSICS
