#include "inpand.h"

#ifdef INTEL_INTRINSICS

#include "../functions16_avx2.h"

using namespace Filtering;

namespace {

typedef uint16_t (local_minimum_f)(uint16_t a1, uint16_t a2, uint16_t a3, uint16_t a4, uint16_t a5, uint16_t a6, uint16_t a7, uint16_t a8, uint16_t a9);

static inline uint16_t minimum_square(uint16_t a1, uint16_t a2, uint16_t a3, uint16_t a4, uint16_t a5, uint16_t a6, uint16_t a7, uint16_t a8, uint16_t a9)
{
    uint16_t nMin = a1;
    if ( a2 < nMin ) nMin = a2;
    if ( a3 < nMin ) nMin = a3;
    if ( a4 < nMin ) nMin = a4;
    if ( a5 < nMin ) nMin = a5;
    if ( a6 < nMin ) nMin = a6;
    if ( a7 < nMin ) nMin = a7;
    if ( a8 < nMin ) nMin = a8;
    if ( a9 < nMin ) nMin = a9;
    return nMin;
}

static inline uint16_t minimum_horizontal(uint16_t a1, uint16_t a2, uint16_t a3, uint16_t a4, uint16_t a5, uint16_t a6, uint16_t a7, uint16_t a8, uint16_t a9)
{
    uint16_t nMin = a4;

    UNUSED(a1); UNUSED(a2); UNUSED(a3); UNUSED(a7); UNUSED(a8); UNUSED(a9); 

    if ( a5 < nMin ) nMin = a5;
    if ( a6 < nMin ) nMin = a6;
    return nMin;
}

static inline uint16_t minimum_vertical(uint16_t a1, uint16_t a2, uint16_t a3, uint16_t a4, uint16_t a5, uint16_t a6, uint16_t a7, uint16_t a8, uint16_t a9)
{
    uint16_t nMin = a2;

    UNUSED(a1); UNUSED(a3); UNUSED(a4); UNUSED(a6); UNUSED(a7); UNUSED(a9); 

    if ( a5 < nMin ) nMin = a5;
    if ( a8 < nMin ) nMin = a8;
    return nMin;
}

static inline uint16_t minimum_both(uint16_t a1, uint16_t a2, uint16_t a3, uint16_t a4, uint16_t a5, uint16_t a6, uint16_t a7, uint16_t a8, uint16_t a9)
{
    uint16_t nMin = a2;

    UNUSED(a1); UNUSED(a3); UNUSED(a7); UNUSED(a9); 

    if ( a4 < nMin ) nMin = a4;
    if ( a5 < nMin ) nMin = a5;
    if ( a6 < nMin ) nMin = a6;
    if ( a8 < nMin ) nMin = a8;
    return nMin;
}

template<local_minimum_f Minimum>
static inline uint16_t minimumThresholded(uint16_t a1, uint16_t a2, uint16_t a3, uint16_t a4, uint16_t a5, uint16_t a6, uint16_t a7, uint16_t a8, uint16_t a9, int nMaxDeviation)
{
    int nMinimum = Minimum(a1, a2, a3, a4, a5, a6, a7, a8, a9);
    if ( a5 - nMinimum > nMaxDeviation ) nMinimum = a5 - nMaxDeviation;
    return static_cast<uint16_t>(nMinimum);
}

extern "C" MT_FORCEINLINE __m256i inpand_operator_avx2_16(__m256i a, __m256i b) {
  return _mm256_min_epu16(a, b);
}

} // namespace

namespace Filtering { namespace MaskTools { namespace Filters { namespace Morphologic { namespace Inpand {

// anonymous: inpand16.cpp has its own copy (the C path must not get this AVX2 compiled one)
  namespace {
    class NewValue16 {
      int nMin;
      int nMaxDeviation;
      uint16_t nValue;
    public:
      NewValue16(uint16_t nValue, int nMaxDeviation) : nMin(65536), nMaxDeviation(nMaxDeviation), nValue(nValue) {}
      void add(uint16_t _nValue) { if (_nValue < nMin) nMin = _nValue; }
      uint16_t finalize() const { return static_cast<uint16_t>(nMin > 65535 ? nValue : (nValue - nMin > nMaxDeviation ? nValue - nMaxDeviation : nMin)); }
    };
  } // namespace


Processor16 *inpand_square_16_avx2_c = &MorphologicProcessor<uint16_t>::generic_16_c<
    process_line_morpho_16_avx2_c<Border::Left, minimumThresholded<::minimum_square>>,
    process_line_morpho_16_avx2_c<Border::None, minimumThresholded<::minimum_square>>,
    process_line_morpho_16_avx2_c<Border::Right, minimumThresholded<::minimum_square>>
    >;

Processor16 *inpand_horizontal_16_avx2_c = &MorphologicProcessor<uint16_t>::generic_16_c<
    process_line_morpho_16_avx2_c<Border::Left, minimumThresholded<::minimum_horizontal>>,
    process_line_morpho_16_avx2_c<Border::None, minimumThresholded<::minimum_horizontal>>,
    process_line_morpho_16_avx2_c<Border::Right, minimumThresholded<::minimum_horizontal>>
    >;

Processor16 *inpand_vertical_16_avx2_c = &MorphologicProcessor<uint16_t>::generic_16_c<
    process_line_morpho_16_avx2_c<Border::Left, minimumThresholded<::minimum_vertical>>,
    process_line_morpho_16_avx2_c<Border::None, minimumThresholded<::minimum_vertical>>,
    process_line_morpho_16_avx2_c<Border::Right, minimumThresholded<::minimum_vertical>>
    >;

Processor16 *inpand_both_16_avx2_c = &MorphologicProcessor<uint16_t>::generic_16_c<
    process_line_morpho_16_avx2_c<Border::Left, minimumThresholded<::minimum_both>>,
    process_line_morpho_16_avx2_c<Border::None, minimumThresholded<::minimum_both>>,
    process_line_morpho_16_avx2_c<Border::Right, minimumThresholded<::minimum_both>>
    >;

#define DEFINE_AVX2_VERSIONS(name) \
    Processor16 *inpand_both_##name##_16 = &generic_16_avx2< \
    process_line_xxpand_both_16_avx2<Border::Left, inpand_operator_avx2_16, limit_down_16_avx2>, \
    process_line_xxpand_both_16_avx2<Border::None, inpand_operator_avx2_16, limit_down_16_avx2>, \
    process_line_xxpand_both_16_avx2<Border::Right, inpand_operator_avx2_16, limit_down_16_avx2> \
    >;

DEFINE_AVX2_VERSIONS(avx2)
#undef DEFINE_AVX2_VERSIONS

Processor16 *inpand_vertical_avx2_16 = &xxpand_avx2_vertical_16<inpand_operator_avx2_16, limit_down_16_avx2>;

Processor16 *inpand_horizontal_avx2_16 = &xxpand_avx2_horizontal_16<inpand_operator_avx2_16, limit_down_16_avx2, inpand_c_horizontal_core_16_avx2>;

Processor16 *inpand_square_avx2_16 = &xxpand_avx2_square_16<inpand_operator_avx2_16, limit_down_16_avx2, inpand_c_horizontal_core_16_avx2>;

Processor16 *inpand_custom_16_avx2_c = &generic_custom_16_avx2_c<NewValue16>;

} } } } }

#endif // INTEL_INTRINSICS