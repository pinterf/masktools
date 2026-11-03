#include "edgemask.h"

#ifdef INTEL_INTRINSICS

#include "edgemask_sse.h"
#include "sobel_real_simd.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Mask { namespace Edge {

using namespace Filters::Mask;

Processor *sobel_real_sse2 = &sobel_real8_simd<SobelSSE>;
Processor16 *sobel_real_16_sse2 = &sobel_real_simd<SobelSSE, uint16_t, int16_t, int>;
Processor32 *sobel_real_32_sse2 = &sobel_real32_simd<SobelSSE>;

#define DEFINE_SSE2_VERSIONS(name) \
Processor *name##_sse2 = &generic_sse2< \
    process_line_##name##_sse2<Border::Left>, \
    process_line_##name##_sse2<Border::None>, \
    process_line_##name##_sse2<Border::Right> \
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
