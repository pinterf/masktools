#include "expand.h"

#ifdef INTEL_INTRINSICS

#include "../functions_sse.h"

using namespace Filtering;

namespace {

extern "C" MT_FORCEINLINE __m128i expand_operator_sse2(__m128i a, __m128i b) {
    return _mm_max_epu8(a, b);
}

} // namespace

namespace Filtering { namespace MaskTools { namespace Filters { namespace Morphologic { namespace Expand {

#define DEFINE_SSE2_VERSIONS(name, mem_mode) \
    Processor *expand_both_##name = &generic_sse2< \
    process_line_xxpand_both<Border::Left, expand_operator_sse2, limit_up_sse2, mem_mode>, \
    process_line_xxpand_both<Border::None, expand_operator_sse2, limit_up_sse2, mem_mode>, \
    process_line_xxpand_both<Border::Right, expand_operator_sse2, limit_up_sse2, MemoryMode::SSE2_UNALIGNED> \
    >;

DEFINE_SSE2_VERSIONS(sse2, MemoryMode::SSE2_UNALIGNED)
DEFINE_SSE2_VERSIONS(asse2, MemoryMode::SSE2_ALIGNED)

Processor *expand_vertical_sse2 = &xxpand_sse2_vertical<expand_operator_sse2, limit_up_sse2, MemoryMode::SSE2_UNALIGNED>;
Processor *expand_vertical_asse2 = &xxpand_sse2_vertical<expand_operator_sse2, limit_up_sse2, MemoryMode::SSE2_ALIGNED>;

Processor *expand_horizontal_sse2 = &xxpand_sse2_horizontal<expand_operator_sse2, limit_up_sse2, MemoryMode::SSE2_UNALIGNED, expand_c_horizontal_core>;
Processor *expand_horizontal_asse2 = &xxpand_sse2_horizontal<expand_operator_sse2, limit_up_sse2, MemoryMode::SSE2_ALIGNED, expand_c_horizontal_core>;

Processor *expand_square_sse2 = &xxpand_sse2_square<expand_operator_sse2, limit_up_sse2, MemoryMode::SSE2_UNALIGNED, expand_c_horizontal_core>;
Processor *expand_square_asse2 = &xxpand_sse2_square<expand_operator_sse2, limit_up_sse2, MemoryMode::SSE2_ALIGNED, expand_c_horizontal_core>;

} } } } }

#endif // INTEL_INTRINSICS
