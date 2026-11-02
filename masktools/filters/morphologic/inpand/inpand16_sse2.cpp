#include "inpand.h"

#ifdef INTEL_INTRINSICS

#include "../functions16_sse.h"

using namespace Filtering;

namespace {

extern "C" MT_FORCEINLINE __m128i inpand_operator_sse2_16(__m128i a, __m128i b) {
  return _MM_MIN_EPU16(a, b);
}

} // namespace

namespace Filtering { namespace MaskTools { namespace Filters { namespace Morphologic { namespace Inpand {

#define DEFINE_SSE2_VERSIONS(name) \
    Processor16 *inpand_both_##name##_16 = &generic_sse2_16< \
    process_line_xxpand_both_16<Border::Left, inpand_operator_sse2_16, limit_down_sse2_16>, \
    process_line_xxpand_both_16<Border::None, inpand_operator_sse2_16, limit_down_sse2_16>, \
    process_line_xxpand_both_16<Border::Right, inpand_operator_sse2_16, limit_down_sse2_16> \
    >;

DEFINE_SSE2_VERSIONS(sse2)
#undef DEFINE_SSE2_VERSIONS

Processor16 *inpand_vertical_sse2_16 = &xxpand_sse2_vertical_16<inpand_operator_sse2_16, limit_down_sse2_16>;

Processor16 *inpand_horizontal_sse2_16 = &xxpand_sse2_horizontal_16<inpand_operator_sse2_16, limit_down_sse2_16, inpand_c_horizontal_core_16>;

Processor16 *inpand_square_sse2_16 = &xxpand_sse2_square_16<inpand_operator_sse2_16, limit_down_sse2_16, inpand_c_horizontal_core_16>;

} } } } }

#endif // INTEL_INTRINSICS
