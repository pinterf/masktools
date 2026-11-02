#ifndef __Mt_clamp16_SSE_H__
#define __Mt_clamp16_SSE_H__

// clamp16: x86 kernels, instantiated in clamp16_sse2.cpp and clamp16_sse41.cpp

#include "../clamp/clamp.h"
#include "../../../common/simd.h"
#include "../../../common/common.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Support  { namespace Clamp {

namespace {

template<CpuFlags flags, bool lessThan16bits>
static __m128i MT_FORCEINLINE clamp16_core_simd(const __m128i &dst, const __m128i &upperLimit, const __m128i &lowerLimit, const __m128i &overshoot, const __m128i &undershoot, const __m128i &max_pixel_value) {
    auto upper_limit_real = _mm_adds_epu16(upperLimit, overshoot);
    if (lessThan16bits)
      upper_limit_real = simd_min_epu16<flags>(upper_limit_real, max_pixel_value);
    auto lower_limit_real = _mm_subs_epu16(lowerLimit, undershoot);

    auto limited = simd_min_epu16<flags>(dst, upper_limit_real); // _mm_min_epu16: SSE4.1 function. Simulated on SSE2
    return simd_max_epu16<flags>(limited, lower_limit_real); // _mm_max_epu16: SSE4.1 function. Simulated on SSE2
}


template<CpuFlags flags, bool lessThan16bits>
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
            limited = clamp16_core_simd<flags, lessThan16bits>(limited, upper_limit, lower_limit, overshoot_v, undershoot_v, max_pixel_value);
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
template<CpuFlags flags>
void clamp16_native_simd(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pUpLimit, ptrdiff_t nUpLimitPitch, const uint8_t *pLowLimit, ptrdiff_t nLowLimitPitch, int nWidth, int nHeight, int bits_per_pixel, int nOvershoot, int nUndershoot)
{
    if (bits_per_pixel == 16)
        clamp16_native_simd_t<flags, false>(pDst, nDstPitch, pUpLimit, nUpLimitPitch, pLowLimit, nLowLimitPitch, nWidth, nHeight, bits_per_pixel, nOvershoot, nUndershoot);
    else
        clamp16_native_simd_t<flags, true>(pDst, nDstPitch, pUpLimit, nUpLimitPitch, pLowLimit, nLowLimitPitch, nWidth, nHeight, bits_per_pixel, nOvershoot, nUndershoot);
}

} // namespace

} } } } }

#endif // __Mt_clamp16_SSE_H__
