#include "edgemask.h"

#ifdef INTEL_INTRINSICS

#include "edgemask_sse.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Mask { namespace Edge {

using namespace Filters::Mask;

#define DEFINE_SSSE3_VERSIONS(name) \
Processor *name##_ssse3 = &generic_sse2< \
    process_line_##name##_sse2<CPU_SSSE3, Border::Left>, \
    process_line_##name##_sse2<CPU_SSSE3, Border::None>, \
    process_line_##name##_sse2<CPU_SSSE3, Border::Right> \
>;

DEFINE_SSSE3_VERSIONS(sobel)
DEFINE_SSSE3_VERSIONS(roberts)
DEFINE_SSSE3_VERSIONS(laplace)
DEFINE_SSSE3_VERSIONS(prewitt)
DEFINE_SSSE3_VERSIONS(half_prewitt)
#undef DEFINE_SSSE3_VERSIONS

} } } } }

#endif // INTEL_INTRINSICS
