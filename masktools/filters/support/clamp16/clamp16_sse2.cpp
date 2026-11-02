#include "../clamp/clamp.h"

#ifdef INTEL_INTRINSICS

#include "../../../common/simd.h"
#include "../../../common/common.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Support  { namespace Clamp {

namespace {

template<bool lessThan16bits>
static __m128i MT_FORCEINLINE clamp16_core_simd(const __m128i &dst, const __m128i &upperLimit, const __m128i &lowerLimit, const __m128i &overshoot, const __m128i &undershoot, const __m128i &max_pixel_value) {
    auto upper_limit_real = _mm_adds_epu16(upperLimit, overshoot);
    if (lessThan16bits)
      upper_limit_real = _MM_MIN_EPU16(upper_limit_real, max_pixel_value);
    auto lower_limit_real = _mm_subs_epu16(lowerLimit, undershoot);

    auto limited = _MM_MIN_EPU16(dst, upper_limit_real);
    return _MM_MAX_EPU16(limited, lower_limit_real);
}


template<bool lessThan16bits>
static void clamp16_native_simd_t(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pUpLimit, ptrdiff_t nUpLimitPitch, const uint8_t *pLowLimit, ptrdiff_t nLowLimitPitch, int nWidth, int nHeight, int bits_per_pixel, int nOvershoot, int nUndershoot)
{
    nWidth *= 2; // really rowsize: width * sizeof(uint16), see also division at C trailer
  
    int wMod16 = (nWidth / 16) * 16;
    auto pDst2 = pDst;
    auto pUpLimit2 = pUpLimit;
    auto pLowLimit2 = pLowLimit;

    auto overshoot_v = _mm_set1_epi16(uint16_t(nOvershoot));
    auto undershoot_v = _mm_set1_epi16(uint16_t(nUndershoot));
    auto max_pixel_value = _mm_set1_epi16(uint16_t((1 << bits_per_pixel) - 1));

    for ( int j = 0; j < nHeight; ++j ) {
        for ( int i = 0; i < wMod16; i+=16 ) {
            auto upper_limit = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pUpLimit+i));
            auto lower_limit = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pLowLimit+i));
            auto limited = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pDst+i));
            limited = clamp16_core_simd<lessThan16bits>(limited, upper_limit, lower_limit, overshoot_v, undershoot_v, max_pixel_value);
            _mm_storeu_si128(reinterpret_cast<__m128i*>(pDst+i), limited);
        }
        pDst += nDstPitch;
        pUpLimit += nUpLimitPitch;
        pLowLimit += nLowLimitPitch;
    }

    if (nWidth > wMod16) {
        clamp16_native_c(pDst2 + wMod16, nDstPitch, pUpLimit2 + wMod16, nUpLimitPitch, pLowLimit2+wMod16, nLowLimitPitch, (nWidth - wMod16) / sizeof(uint16_t), nHeight, bits_per_pixel, nOvershoot, nUndershoot);
    }
}

// one template switch per call: 16 bit needs no clamping to max_pixel_value
void clamp16_native_sse2_t(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pUpLimit, ptrdiff_t nUpLimitPitch, const uint8_t *pLowLimit, ptrdiff_t nLowLimitPitch, int nWidth, int nHeight, int bits_per_pixel, int nOvershoot, int nUndershoot)
{
    if (bits_per_pixel == 16)
        clamp16_native_simd_t<false>(pDst, nDstPitch, pUpLimit, nUpLimitPitch, pLowLimit, nLowLimitPitch, nWidth, nHeight, bits_per_pixel, nOvershoot, nUndershoot);
    else
        clamp16_native_simd_t<true>(pDst, nDstPitch, pUpLimit, nUpLimitPitch, pLowLimit, nLowLimitPitch, nWidth, nHeight, bits_per_pixel, nOvershoot, nUndershoot);
}

} // namespace

Processor16 *clamp16_native_sse2 = &clamp16_native_sse2_t;

} } } } }

#endif // INTEL_INTRINSICS
