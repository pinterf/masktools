#include "expand.h"

#ifdef INTEL_INTRINSICS

#include "../functions_avx2.h"

using namespace Filtering;

namespace {

extern "C" MT_FORCEINLINE __m256i expand_operator_avx2(__m256i a, __m256i b) {
    return _mm256_max_epu8(a, b);
}

} // namespace

namespace Filtering { namespace MaskTools { namespace Filters { namespace Morphologic { namespace Expand {

#define DEFINE_AVX2_VERSIONS(name) \
    Processor *expand_both_##name = &generic_avx2< \
    process_line_xxpand_both_avx2<Border::Left, expand_operator_avx2, limit_up_avx2>, \
    process_line_xxpand_both_avx2<Border::None, expand_operator_avx2, limit_up_avx2>, \
    process_line_xxpand_both_avx2<Border::Right, expand_operator_avx2, limit_up_avx2> \
    >;

DEFINE_AVX2_VERSIONS(avx2)

Processor *expand_vertical_avx2 = &xxpand_avx2_vertical<expand_operator_avx2, limit_up_avx2>;

Processor *expand_horizontal_avx2 = &xxpand_avx2_horizontal<expand_operator_avx2, limit_up_avx2, expand_c_horizontal_core>;

Processor *expand_square_avx2 = &xxpand_avx2_square<expand_operator_avx2, limit_up_avx2, expand_c_horizontal_core>;

} } } } }

#endif // INTEL_INTRINSICS
