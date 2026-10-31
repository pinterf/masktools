#include "logic.h"

#ifdef INTEL_INTRINSICS

#include "logic16_sse.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Logic {

Processor16 *and16_native_sse2  = &logic16_native_t_sse2<and16_sse2, and16_c>;
Processor16 *or16_native_sse2   = &logic16_native_t_sse2<or16_sse2, or16_c>;
Processor16 *andn16_native_sse2 = &logic16_native_t_sse2<andn16_sse2, andn16_c>;
Processor16 *xor16_native_sse2  = &logic16_native_t_sse2<xor16_sse2, xor16_c>;

} } } }

#endif // INTEL_INTRINSICS
