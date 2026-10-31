#include "inflate.h"

#ifdef INTEL_INTRINSICS

#include "../functions16_sse.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Morphologic { namespace Inflate {

Processor16 *inflate_sse4_16 = &generic_sse4_16<
  process_line_xxflate_16<Border::Left, limit_up_sse4_16, MemoryMode::SSE2_UNALIGNED>,
  process_line_xxflate_16<Border::None, limit_up_sse4_16, MemoryMode::SSE2_UNALIGNED>,
  process_line_xxflate_16<Border::Right, limit_up_sse4_16, MemoryMode::SSE2_UNALIGNED>
>;
Processor16 *inflate_asse4_16 = &generic_sse4_16<
  process_line_xxflate_16<Border::Left, limit_up_sse4_16, MemoryMode::SSE2_ALIGNED>,
  process_line_xxflate_16<Border::None, limit_up_sse4_16, MemoryMode::SSE2_ALIGNED>,
  process_line_xxflate_16<Border::Right, limit_up_sse4_16, MemoryMode::SSE2_UNALIGNED>
>;

} } } } }

#endif // INTEL_INTRINSICS
