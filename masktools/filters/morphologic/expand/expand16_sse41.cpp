#include "expand.h"

#ifdef INTEL_INTRINSICS

#include "../functions16_sse.h"

using namespace Filtering;

namespace {

extern "C" MT_FORCEINLINE __m128i expand_operator_sse4_16(__m128i a, __m128i b) {
  return _mm_max_epu16(a, b);
}

} // namespace

namespace Filtering { namespace MaskTools { namespace Filters { namespace Morphologic { namespace Expand {

#define DEFINE_SSE4_VERSIONS(name, mem_mode) \
    Processor16 *expand_both_##name##_16 = &generic_sse4_16< \
    process_line_xxpand_both_16<Border::Left, expand_operator_sse4_16, limit_up_sse4_16, mem_mode>, \
    process_line_xxpand_both_16<Border::None, expand_operator_sse4_16, limit_up_sse4_16, mem_mode>, \
    process_line_xxpand_both_16<Border::Right, expand_operator_sse4_16, limit_up_sse4_16, MemoryMode::SSE2_UNALIGNED> \
    >;

DEFINE_SSE4_VERSIONS(sse4, MemoryMode::SSE2_UNALIGNED)
#undef DEFINE_SSE4_VERSIONS

Processor16 *expand_vertical_sse4_16 = &xxpand_sse4_vertical_16<expand_operator_sse4_16, limit_up_sse4_16, MemoryMode::SSE2_UNALIGNED>;

Processor16 *expand_horizontal_sse4_16 = &xxpand_sse4_horizontal_16<expand_operator_sse4_16, limit_up_sse4_16, MemoryMode::SSE2_UNALIGNED, expand_c_horizontal_core_16>;

Processor16 *expand_square_sse4_16 = &xxpand_sse4_square_16<expand_operator_sse4_16, limit_up_sse4_16, MemoryMode::SSE2_UNALIGNED, expand_c_horizontal_core_16>;

} } } } }

#endif // INTEL_INTRINSICS
