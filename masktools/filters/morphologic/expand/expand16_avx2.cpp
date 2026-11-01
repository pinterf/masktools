#include "expand.h"

#ifdef INTEL_INTRINSICS

#include "../functions16_avx2.h"

using namespace Filtering;

namespace {

typedef uint16_t (local_maximum_f)(uint16_t a1, uint16_t a2, uint16_t a3, uint16_t a4, uint16_t a5, uint16_t a6, uint16_t a7, uint16_t a8, uint16_t a9);

static inline uint16_t maximum_square(uint16_t a1, uint16_t a2, uint16_t a3, uint16_t a4, uint16_t a5, uint16_t a6, uint16_t a7, uint16_t a8, uint16_t a9)
{
    uint16_t nMax = a1;
    if ( a2 > nMax ) nMax = a2;
    if ( a3 > nMax ) nMax = a3;
    if ( a4 > nMax ) nMax = a4;
    if ( a5 > nMax ) nMax = a5;
    if ( a6 > nMax ) nMax = a6;
    if ( a7 > nMax ) nMax = a7;
    if ( a8 > nMax ) nMax = a8;
    if ( a9 > nMax ) nMax = a9;
    return nMax;
}

static inline uint16_t maximum_horizontal(uint16_t a1, uint16_t a2, uint16_t a3, uint16_t a4, uint16_t a5, uint16_t a6, uint16_t a7, uint16_t a8, uint16_t a9)
{
    uint16_t nMax = a4;

    UNUSED(a1); UNUSED(a2); UNUSED(a3); UNUSED(a7); UNUSED(a8); UNUSED(a9); 

    if ( a5 > nMax ) nMax = a5;
    if ( a6 > nMax ) nMax = a6;
    return nMax;
}

static inline uint16_t maximum_vertical(uint16_t a1, uint16_t a2, uint16_t a3, uint16_t a4, uint16_t a5, uint16_t a6, uint16_t a7, uint16_t a8, uint16_t a9)
{
    uint16_t nMax = a2;

    UNUSED(a1); UNUSED(a3); UNUSED(a4); UNUSED(a6); UNUSED(a7); UNUSED(a9); 

    if ( a5 > nMax ) nMax = a5;
    if ( a8 > nMax ) nMax = a8;
    return nMax;
}

static inline uint16_t maximum_both(uint16_t a1, uint16_t a2, uint16_t a3, uint16_t a4, uint16_t a5, uint16_t a6, uint16_t a7, uint16_t a8, uint16_t a9)
{
    uint16_t nMax = a2;

    UNUSED(a1); UNUSED(a3); UNUSED(a7); UNUSED(a9); 

    if ( a4 > nMax ) nMax = a4;
    if ( a5 > nMax ) nMax = a5;
    if ( a6 > nMax ) nMax = a6;
    if ( a8 > nMax ) nMax = a8;
    return nMax;
}

template<local_maximum_f Maximum>
static inline uint16_t maximumThresholded(uint16_t a1, uint16_t a2, uint16_t a3, uint16_t a4, uint16_t a5, uint16_t a6, uint16_t a7, uint16_t a8, uint16_t a9, int nMaxDeviation)
{
    int nMaximum = Maximum(a1, a2, a3, a4, a5, a6, a7, a8, a9);
    if ( nMaximum - a5 > nMaxDeviation ) nMaximum = a5 + nMaxDeviation;
    return static_cast<uint16_t>(nMaximum);
}

extern "C" MT_FORCEINLINE __m256i expand_operator_avx2_16(__m256i a, __m256i b) {
  return _mm256_max_epu16(a, b);
}

} // namespace

namespace Filtering { namespace MaskTools { namespace Filters { namespace Morphologic { namespace Expand {

    class NewValue16 {
        int nMax;
        int nMaxDeviation;
        uint16_t nValue;
    public:
        NewValue16(uint16_t nValue, int nMaxDeviation) : nMax(-1), nMaxDeviation(nMaxDeviation), nValue(nValue) { }
        void add(uint16_t _nValue) { if ( _nValue > nMax ) nMax = _nValue; }
        uint16_t finalize() const { return static_cast<uint16_t>(nMax < 0 ? nValue : (nMax - nValue > nMaxDeviation ? nValue + nMaxDeviation : nMax)); }
    };


Processor16 *expand_square_16_avx2_c = &MorphologicProcessor<uint16_t>::generic_16_c<
    process_line_morpho_16_avx2_c<Border::Left, maximumThresholded<::maximum_square>>,
    process_line_morpho_16_avx2_c<Border::None, maximumThresholded<::maximum_square>>,
    process_line_morpho_16_avx2_c<Border::Right, maximumThresholded<::maximum_square>>
    >;

Processor16 *expand_horizontal_16_avx2_c = &MorphologicProcessor<uint16_t>::generic_16_c<
    process_line_morpho_16_avx2_c<Border::Left, maximumThresholded<::maximum_horizontal>>,
    process_line_morpho_16_avx2_c<Border::None, maximumThresholded<::maximum_horizontal>>,
    process_line_morpho_16_avx2_c<Border::Right, maximumThresholded<::maximum_horizontal>>
    >;

Processor16 *expand_vertical_16_avx2_c = &MorphologicProcessor<uint16_t>::generic_16_c<
    process_line_morpho_16_avx2_c<Border::Left, maximumThresholded<::maximum_vertical>>,
    process_line_morpho_16_avx2_c<Border::None, maximumThresholded<::maximum_vertical>>,
    process_line_morpho_16_avx2_c<Border::Right, maximumThresholded<::maximum_vertical>>
    >;

Processor16 *expand_both_16_avx2_c = &MorphologicProcessor<uint16_t>::generic_16_c<
    process_line_morpho_16_avx2_c<Border::Left, maximumThresholded<::maximum_both>>,
    process_line_morpho_16_avx2_c<Border::None, maximumThresholded<::maximum_both>>,
    process_line_morpho_16_avx2_c<Border::Right, maximumThresholded<::maximum_both>>
    >;

#define DEFINE_AVX2_VERSIONS(name, mem_mode) \
    Processor16 *expand_both_##name##_16 = &generic_16_avx2< \
    process_line_xxpand_both_16_avx2<Border::Left, expand_operator_avx2_16, limit_up_16_avx2, mem_mode>, \
    process_line_xxpand_both_16_avx2<Border::None, expand_operator_avx2_16, limit_up_16_avx2, mem_mode>, \
    process_line_xxpand_both_16_avx2<Border::Right, expand_operator_avx2_16, limit_up_16_avx2, MemoryMode::SSE2_UNALIGNED> \
    >;

DEFINE_AVX2_VERSIONS(avx2, MemoryMode::SSE2_UNALIGNED)
#undef DEFINE_AVX2_VERSIONS

Processor16 *expand_vertical_avx2_16 = &xxpand_avx2_vertical_16<expand_operator_avx2_16, limit_up_16_avx2, MemoryMode::SSE2_UNALIGNED>;

Processor16 *expand_horizontal_avx2_16 = &xxpand_avx2_horizontal_16<expand_operator_avx2_16, limit_up_16_avx2, MemoryMode::SSE2_UNALIGNED, expand_c_horizontal_core_16_avx2>;

Processor16 *expand_square_avx2_16 = &xxpand_avx2_square_16<expand_operator_avx2_16, limit_up_16_avx2, MemoryMode::SSE2_UNALIGNED, expand_c_horizontal_core_16_avx2>;

Processor16 *expand_custom_16_avx2_c   = &generic_custom_16_avx2_c<NewValue16>;

} } } } }

#endif // INTEL_INTRINSICS