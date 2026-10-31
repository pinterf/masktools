#include "inflate.h"

#ifdef INTEL_INTRINSICS

#include "../functions_sse.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Morphologic { namespace Inflate {

Processor *inflate_sse2 = &generic_sse2<
    process_line_xxflate<Border::Left, limit_up_sse2, MemoryMode::SSE2_UNALIGNED>,
    process_line_xxflate<Border::None, limit_up_sse2, MemoryMode::SSE2_UNALIGNED>,
    process_line_xxflate<Border::Right, limit_up_sse2, MemoryMode::SSE2_UNALIGNED>
>;
Processor *inflate_asse2 = &generic_sse2<
    process_line_xxflate<Border::Left, limit_up_sse2, MemoryMode::SSE2_ALIGNED>,
    process_line_xxflate<Border::None, limit_up_sse2, MemoryMode::SSE2_ALIGNED>,
    process_line_xxflate<Border::Right, limit_up_sse2, MemoryMode::SSE2_UNALIGNED>
>;

} } } } }

#endif // INTEL_INTRINSICS
