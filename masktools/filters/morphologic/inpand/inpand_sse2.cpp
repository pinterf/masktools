#include "inpand.h"

#ifdef INTEL_INTRINSICS

#include "../functions_sse.h"

using namespace Filtering;

namespace {

extern "C" MT_FORCEINLINE __m128i inpand_operator_sse2(__m128i a, __m128i b) {
    return _mm_min_epu8(a, b);
}

} // namespace

namespace Filtering { namespace MaskTools { namespace Filters { namespace Morphologic { namespace Inpand {

#define DEFINE_SSE2_VERSIONS(name, mem_mode) \
    Processor *inpand_both_##name = &generic_sse2< \
    process_line_xxpand_both<Border::Left, inpand_operator_sse2, limit_down_sse2, mem_mode>, \
    process_line_xxpand_both<Border::None, inpand_operator_sse2, limit_down_sse2, mem_mode>, \
    process_line_xxpand_both<Border::Right, inpand_operator_sse2, limit_down_sse2, MemoryMode::SSE2_UNALIGNED> \
    >;

DEFINE_SSE2_VERSIONS(sse2, MemoryMode::SSE2_UNALIGNED)
DEFINE_SSE2_VERSIONS(asse2, MemoryMode::SSE2_ALIGNED)

Processor *inpand_vertical_sse2 = &xxpand_sse2_vertical<inpand_operator_sse2, limit_down_sse2, MemoryMode::SSE2_UNALIGNED>;
Processor *inpand_vertical_asse2 = &xxpand_sse2_vertical<inpand_operator_sse2, limit_down_sse2, MemoryMode::SSE2_ALIGNED>;

Processor *inpand_horizontal_sse2 = &xxpand_sse2_horizontal<inpand_operator_sse2, limit_down_sse2, MemoryMode::SSE2_UNALIGNED, inpand_c_horizontal_core>;
Processor *inpand_horizontal_asse2 = &xxpand_sse2_horizontal<inpand_operator_sse2, limit_down_sse2, MemoryMode::SSE2_ALIGNED, inpand_c_horizontal_core>;

Processor *inpand_square_sse2 = &xxpand_sse2_square<inpand_operator_sse2, limit_down_sse2, MemoryMode::SSE2_UNALIGNED, inpand_c_horizontal_core>;
Processor *inpand_square_asse2 = &xxpand_sse2_square<inpand_operator_sse2, limit_down_sse2, MemoryMode::SSE2_ALIGNED, inpand_c_horizontal_core>;

} } } } }

#endif // INTEL_INTRINSICS
