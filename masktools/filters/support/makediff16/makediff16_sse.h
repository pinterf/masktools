#ifndef __Mt_makediff16_SSE_H__
#define __Mt_makediff16_SSE_H__

// makediff16: x86 kernels, instantiated in makediff16_sse2.cpp and makediff16_sse41.cpp

#include "../makediff/makediff.h"
#include "../../../common/simd.h"
#include "../../../common/common.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Support  { namespace MakeDiff {

// C versions, for the right edge
template<int bits_per_pixel>
void makediff16_native_c(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int nWidth, int nHeight, int );

namespace {

template<CpuFlags flags, bool lessThan16bits>
static MT_FORCEINLINE __m128i makediff16_core_simd(const __m128i &dst, const __m128i &src, const __m128i &halfrange, const __m128i &max_pixel_value) {
  if (lessThan16bits) {
    auto val = _mm_add_epi16(dst, halfrange);
    auto result = _mm_subs_epu16(val, src);
    return simd_min_epu16<flags>(result, max_pixel_value);
  }
  else {
    auto dstval = _mm_sub_epi16(dst, halfrange);
    auto srcval = _mm_sub_epi16(src, halfrange);
    auto result = _mm_subs_epi16(dstval, srcval);
    return _mm_add_epi16(result, halfrange);
  }
}

// for 16 bits SSE2 is enough, 10-14 can benefit from SSE4_1
template<CpuFlags flags, int bits_per_pixel>
void makediff16_native_simd(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int nWidth, int nHeight, int nOrigHeight)
{
    nWidth *= 2; // really rowsize: width * sizeof(uint16), see also division at C trailer

    int wMod16 = (nWidth / 16) * 16;

    auto pDst2 = pDst;
    auto pSrc2 = pSrc;
    auto halfrange = _mm_set1_epi16((short)(1 << (bits_per_pixel - 1)));
    int _max_pixel_value = (1 << bits_per_pixel) - 1;
#pragma warning(disable: 4244)
    auto max_pixel_value = _mm_set1_epi16(_max_pixel_value);
#pragma warning(default: 4244)

    for (int j = 0; j < nHeight; ++j) {
        for (int i = 0; i < wMod16; i+=16) {
            auto dst = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pDst+i));
            auto src = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pSrc+i));

            __m128i result;
            if (bits_per_pixel < 16)
              result = makediff16_core_simd<flags, true>(dst, src, halfrange, max_pixel_value);
            else
              result = makediff16_core_simd<flags, false>(dst, src, halfrange, max_pixel_value);

            _mm_storeu_si128(reinterpret_cast<__m128i*>(pDst + i), result);
        }
        pDst += nDstPitch;
        pSrc += nSrcPitch;
    }
    if (nWidth > wMod16) {
        makediff16_native_c<bits_per_pixel>(pDst2 + wMod16, nDstPitch, pSrc2 + wMod16, nSrcPitch, (nWidth - wMod16) / sizeof(uint16_t), nHeight, nOrigHeight);
    }
}

} // namespace

} } } } }

#endif // __Mt_makediff16_SSE_H__
