#include "inpand.h"

#ifdef INTEL_INTRINSICS

#include "../functions16_sse.h"

using namespace Filtering;

namespace {

extern "C" MT_FORCEINLINE __m128i inpand_operator_sse4_16(__m128i a, __m128i b) {
  return _mm_min_epu16(a, b);
}

} // namespace

namespace Filtering { namespace MaskTools { namespace Filters { namespace Morphologic { namespace Inpand {

#define DEFINE_SSE4_VERSIONS(name) \
    Processor16 *inpand_both_##name##_16 = &generic_sse4_16< \
    process_line_xxpand_both_16<Border::Left, inpand_operator_sse4_16, limit_down_sse4_16>, \
    process_line_xxpand_both_16<Border::None, inpand_operator_sse4_16, limit_down_sse4_16>, \
    process_line_xxpand_both_16<Border::Right, inpand_operator_sse4_16, limit_down_sse4_16> \
    >;

DEFINE_SSE4_VERSIONS(sse4)
#undef DEFINE_SSE4_VERSIONS

Processor16 *inpand_vertical_sse4_16 = &xxpand_sse4_vertical_16<inpand_operator_sse4_16, limit_down_sse4_16>;

Processor16 *inpand_horizontal_sse4_16 = &xxpand_sse4_horizontal_16<inpand_operator_sse4_16, limit_down_sse4_16, inpand_c_horizontal_core_16>;

Processor16 *inpand_square_sse4_16 = &xxpand_sse4_square_16<inpand_operator_sse4_16, limit_down_sse4_16, inpand_c_horizontal_core_16>;

} } } } }

#endif // INTEL_INTRINSICS
