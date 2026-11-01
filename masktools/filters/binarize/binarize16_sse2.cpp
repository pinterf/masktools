#include "binarize.h"

#ifdef INTEL_INTRINSICS

#include "../../common/simd.h"
#include "../../common/common.h"
#include "binarize16_c.h"

namespace {

/* SSE2 functions */
template<bool lessthan16bits>
static inline __m128i binarize_upper_sse2_op(__m128i x, __m128i t, __m128i, __m128i &max_pixel_value) {
    auto r = _mm_subs_epu16(x, t);
    if (!lessthan16bits) {
      return _mm_cmpeq_epi16(r, _mm_setzero_si128()); // set FFFF where equal to 0, that is x<t
    } 
    else {
      return _mm_and_si128(_mm_cmpeq_epi16(r, _mm_setzero_si128()), max_pixel_value);
    }
}

template<bool lessthan16bits>
static MT_FORCEINLINE __m128i binarize_lower_sse2_op(__m128i x, __m128i, __m128i halfrange, __m128i &max_pixel_value) {
#pragma warning(disable: 4309)
  auto r = _mm_add_epi16(x,  _mm_set1_epi16(0x8000)); // 0x8000 for (at 8 bit version:0x80) and 16bit branch signed cmp
#pragma warning(default: 4309)
  if (!lessthan16bits) {
      return _mm_cmpgt_epi16(r, halfrange);
    } 
    else {
      return _mm_and_si128(_mm_cmpgt_epi16(r, halfrange), max_pixel_value);
    }
}

template<bool lessthan16bits>
static inline __m128i binarize_0_x_sse2_op(__m128i x, __m128i t, __m128i halfrange, __m128i &max_pixel_value) {
    auto upper = binarize_upper_sse2_op<false>(x, t, halfrange, max_pixel_value); // the 16 bit (unmasked) 0xFFFF is needed for the and
    return _mm_and_si128(upper, x);
}

template<bool lessthan16bits>
#if defined(GCC) || defined(CLANG)
__attribute__((__target__("sse4.1")))
#endif
static inline __m128i binarize_t_x_sse2_op(__m128i x, __m128i t, __m128i, __m128i &) {
    return _mm_min_epu16(t, x);
}

template<bool lessthan16bits>
static inline __m128i binarize_x_0_sse2_op(__m128i x, __m128i t, __m128i halfrange, __m128i &max_pixel_value) {
    auto lower = binarize_lower_sse2_op<false>(x, t, halfrange, max_pixel_value); // the 16 bit (unmasked) 0xFFFF is needed for the and
    return _mm_and_si128(lower, x);
}

template<bool lessthan16bits>
#if defined(GCC) || defined(CLANG)
__attribute__((__target__("sse4.1")))
#endif
static inline __m128i binarize_x_t_sse2_op(__m128i x, __m128i t, __m128i, __m128i &) {
    return _mm_max_epu16(t, x);
}

template<bool lessthan16bits>
static inline __m128i binarize_t_0_sse2_op(__m128i x, __m128i t, __m128i halfrange, __m128i &max_pixel_value) {
    auto lower = binarize_lower_sse2_op<false>(x, t, halfrange, max_pixel_value); // the 16 bit (unmasked) 0xFFFF is needed for the and
    return _mm_and_si128(lower, t);
}

template<bool lessthan16bits>
static inline __m128i binarize_0_t_sse2_op(__m128i x, __m128i t, __m128i halfrange, __m128i &max_pixel_value) {
    auto upper = binarize_upper_sse2_op<false>(x, t, halfrange, max_pixel_value); // the 16 bit (unmasked) 0xFFFF is needed for the and
    return _mm_and_si128(upper, t);
}

template<bool lessthan16bits>
static inline __m128i binarize_x_255_sse2_op(__m128i x, __m128i t, __m128i halfrange, __m128i &max_pixel_value) {
    auto upper = binarize_upper_sse2_op<lessthan16bits>(x, t, halfrange, max_pixel_value); 
    return _mm_or_si128(upper, x);
}

template<bool lessthan16bits>
static inline __m128i binarize_255_x_sse2_op(__m128i x, __m128i t, __m128i halfrange, __m128i &max_pixel_value) {
    auto lower = binarize_lower_sse2_op<lessthan16bits>(x, t, halfrange, max_pixel_value);
    return _mm_or_si128(lower, x);
}

template<bool lessthan16bits>
static inline __m128i binarize_t_255_sse2_op(__m128i x, __m128i t, __m128i halfrange, __m128i &max_pixel_value) {
    auto upper = binarize_upper_sse2_op<lessthan16bits>(x, t, halfrange, max_pixel_value);
    return _mm_or_si128(upper, t);
}

template<bool lessthan16bits>
static inline __m128i binarize_255_t_sse2_op(__m128i x, __m128i t, __m128i halfrange, __m128i &max_pixel_value) {
    auto lower = binarize_lower_sse2_op<lessthan16bits>(x, t, halfrange, max_pixel_value);
    return _mm_or_si128(lower, t);
}

#pragma warning(disable: 4309)

template<bool lessthan16bits, decltype(binarize_upper_sse2_op<false>) op, Operator op_c>
void binarize_sse2_native_t(uint8_t *pDst, ptrdiff_t nDstPitch, uint16_t nThreshold, int nWidth, int nHeight, int bits_per_pixel)
{
    nWidth *= 2; // really rowsize: width * sizeof(uint16)
    int wMod16 = (nWidth / 16) * 16;
    auto pDst2 = pDst;
    auto t = _mm_set1_epi16(uint16_t(nThreshold));
    auto halfrange = _mm_add_epi16(t, _mm_set1_epi16(0x8000)); // shift for signed int cmp
    auto max_pixel_value = _mm_set1_epi16(uint16_t((1 << bits_per_pixel) - 1));

    for (int j = 0; j < nHeight; ++j) {
        for (int i = 0; i < wMod16; i+=16) {
            auto src = _mm_loadu_si128(reinterpret_cast<const __m128i*>(reinterpret_cast<const __m128i*>(pDst+i)));

            auto result = op(src, t, halfrange, max_pixel_value);

            _mm_storeu_si128(reinterpret_cast<__m128i*>(reinterpret_cast<__m128i*>(pDst+i)), result);
        }
        pDst += nDstPitch;
    }
    if (nWidth > wMod16) {
        binarize_native_t<op_c>(pDst2 + wMod16, nDstPitch, nThreshold, (nWidth - wMod16)/sizeof(uint16_t), nHeight, bits_per_pixel); // rowsize/2
    }
}

// one template switch per call: 16 bit needs no masking with max_pixel_value
template<decltype(binarize_upper_sse2_op<false>) op16, decltype(binarize_upper_sse2_op<true>) op, Operator op_c>
void binarize_sse2_native(uint8_t *pDst, ptrdiff_t nDstPitch, uint16_t nThreshold, int nWidth, int nHeight, int bits_per_pixel)
{
  if (bits_per_pixel == 16)
    binarize_sse2_native_t<false, op16, op_c>(pDst, nDstPitch, nThreshold, nWidth, nHeight, bits_per_pixel);
  else
    binarize_sse2_native_t<true, op, op_c>(pDst, nDstPitch, nThreshold, nWidth, nHeight, bits_per_pixel);
}

} // namespace

namespace Filtering { namespace MaskTools { namespace Filters { namespace Binarize {

#define DEFINE_PROCESSORS(name) \
Processor16 *binarize_##name##_native_sse2 = &binarize_sse2_native<binarize_##name##_sse2_op<false>, binarize_##name##_sse2_op<true>, binarize_##name>;

DEFINE_PROCESSORS(upper)
DEFINE_PROCESSORS(lower)
DEFINE_PROCESSORS(0_x)
DEFINE_PROCESSORS(t_x)
DEFINE_PROCESSORS(x_0)
DEFINE_PROCESSORS(x_t)
DEFINE_PROCESSORS(t_0)
DEFINE_PROCESSORS(0_t)
DEFINE_PROCESSORS(x_255)
DEFINE_PROCESSORS(t_255)
DEFINE_PROCESSORS(255_x)
DEFINE_PROCESSORS(255_t)
#undef DEFINE_PROCESSORS

} } } }

#endif // INTEL_INTRINSICS
