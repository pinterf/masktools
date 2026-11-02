#include "inpand.h"

#ifdef INTEL_INTRINSICS

#include "../functions_avx2.h"

using namespace Filtering;

namespace {

extern "C" MT_FORCEINLINE __m256i inpand_operator_avx2(__m256i a, __m256i b) {
    return _mm256_min_epu8(a, b);
}

} // namespace

namespace Filtering { namespace MaskTools { namespace Filters { namespace Morphologic { namespace Inpand {

#define DEFINE_AVX2_VERSIONS(name) \
    Processor *inpand_both_##name = &generic_avx2< \
    process_line_xxpand_both_avx2<Border::Left, inpand_operator_avx2, limit_down_avx2>, \
    process_line_xxpand_both_avx2<Border::None, inpand_operator_avx2, limit_down_avx2>, \
    process_line_xxpand_both_avx2<Border::Right, inpand_operator_avx2, limit_down_avx2> \
    >;

DEFINE_AVX2_VERSIONS(avx2)

Processor *inpand_vertical_avx2 = &xxpand_avx2_vertical<inpand_operator_avx2, limit_down_avx2>;

Processor *inpand_horizontal_avx2 = &xxpand_avx2_horizontal<inpand_operator_avx2, limit_down_avx2, inpand_c_horizontal_core>;

Processor *inpand_square_avx2 = &xxpand_avx2_square<inpand_operator_avx2, limit_down_avx2, inpand_c_horizontal_core>;

} } } } }

#endif // INTEL_INTRINSICS
