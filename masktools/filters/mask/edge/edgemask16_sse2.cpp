#include "edgemask.h"

#ifdef INTEL_INTRINSICS

#include "edgemask16_sse.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Mask { namespace Edge {

using namespace Filters::Mask;

#define DEFINE_SSE2_VERSIONS(name) \
Processor16 *name##_16_sse2 = &generic16_sse2< \
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
