#include "edgemask.h"

#ifdef INTEL_INTRINSICS

#include "edgemask16_sse.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Mask { namespace Edge {

using namespace Filters::Mask;

#define DEFINE_SSE41_VERSIONS(name) \
Processor16 *name##_10_sse4 = &generic16_sse2<10, \
    process_line_##name##_sse2<CPU_SSE4_1, 10, Border::Left, MemoryMode::SSE2_UNALIGNED>, \
    process_line_##name##_sse2<CPU_SSE4_1, 10, Border::None, MemoryMode::SSE2_UNALIGNED>, \
    process_line_##name##_sse2<CPU_SSE4_1, 10, Border::Right, MemoryMode::SSE2_UNALIGNED> \
>; \
Processor16 *name##_12_sse4 = &generic16_sse2<12, \
    process_line_##name##_sse2<CPU_SSE4_1, 12, Border::Left, MemoryMode::SSE2_UNALIGNED>, \
    process_line_##name##_sse2<CPU_SSE4_1, 12, Border::None, MemoryMode::SSE2_UNALIGNED>, \
    process_line_##name##_sse2<CPU_SSE4_1, 12, Border::Right, MemoryMode::SSE2_UNALIGNED> \
>; \
Processor16 *name##_14_sse4 = &generic16_sse2<14, \
    process_line_##name##_sse2<CPU_SSE4_1, 14, Border::Left, MemoryMode::SSE2_UNALIGNED>, \
    process_line_##name##_sse2<CPU_SSE4_1, 14, Border::None, MemoryMode::SSE2_UNALIGNED>, \
    process_line_##name##_sse2<CPU_SSE4_1, 14, Border::Right, MemoryMode::SSE2_UNALIGNED> \
>; \
Processor16 *name##_16_sse4 = &generic16_sse2<16, \
    process_line_##name##_sse2<CPU_SSE4_1, 16, Border::Left, MemoryMode::SSE2_UNALIGNED>, \
    process_line_##name##_sse2<CPU_SSE4_1, 16, Border::None, MemoryMode::SSE2_UNALIGNED>, \
    process_line_##name##_sse2<CPU_SSE4_1, 16, Border::Right, MemoryMode::SSE2_UNALIGNED> \
>;

DEFINE_SSE41_VERSIONS(sobel)
DEFINE_SSE41_VERSIONS(roberts)
DEFINE_SSE41_VERSIONS(laplace)
DEFINE_SSE41_VERSIONS(prewitt)
DEFINE_SSE41_VERSIONS(half_prewitt)
DEFINE_SSE41_VERSIONS(convolution)
DEFINE_SSE41_VERSIONS(morpho)
DEFINE_SSE41_VERSIONS(cartoon)
#undef DEFINE_SSE41_VERSIONS

} } } } }

#endif // INTEL_INTRINSICS
