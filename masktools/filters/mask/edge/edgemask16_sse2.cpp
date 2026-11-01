#include "edgemask.h"

#ifdef INTEL_INTRINSICS

#include "edgemask16_sse.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Mask { namespace Edge {

using namespace Filters::Mask;

#define DEFINE_SSE2_VERSIONS(name) \
Processor16 *name##_10_sse2 = &generic16_sse2<10, \
    process_line_##name##_sse2<CPU_SSE2, 10, Border::Left>, \
    process_line_##name##_sse2<CPU_SSE2, 10, Border::None>, \
    process_line_##name##_sse2<CPU_SSE2, 10, Border::Right> \
>; \
Processor16 *name##_12_sse2 = &generic16_sse2<12, \
    process_line_##name##_sse2<CPU_SSE2, 12, Border::Left>, \
    process_line_##name##_sse2<CPU_SSE2, 12, Border::None>, \
    process_line_##name##_sse2<CPU_SSE2, 12, Border::Right> \
>; \
Processor16 *name##_14_sse2 = &generic16_sse2<14, \
    process_line_##name##_sse2<CPU_SSE2, 14, Border::Left>, \
    process_line_##name##_sse2<CPU_SSE2, 14, Border::None>, \
    process_line_##name##_sse2<CPU_SSE2, 14, Border::Right> \
>; \
Processor16 *name##_16_sse2 = &generic16_sse2<16, \
    process_line_##name##_sse2<CPU_SSE2, 16, Border::Left>, \
    process_line_##name##_sse2<CPU_SSE2, 16, Border::None>, \
    process_line_##name##_sse2<CPU_SSE2, 16, Border::Right> \
>;

DEFINE_SSE2_VERSIONS(sobel)
DEFINE_SSE2_VERSIONS(roberts)
DEFINE_SSE2_VERSIONS(laplace)
DEFINE_SSE2_VERSIONS(prewitt)
DEFINE_SSE2_VERSIONS(half_prewitt)
DEFINE_SSE2_VERSIONS(convolution)
DEFINE_SSE2_VERSIONS(morpho)
DEFINE_SSE2_VERSIONS(cartoon)
#undef DEFINE_SSE2_VERSIONS

} } } } }

#endif // INTEL_INTRINSICS
