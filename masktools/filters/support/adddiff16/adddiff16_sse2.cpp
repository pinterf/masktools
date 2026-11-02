#include "../adddiff/adddiff.h"

#ifdef INTEL_INTRINSICS

#include "../../../common/simd.h"
#include "../../../common/common.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Support  { namespace AddDiff {

namespace {

template<bool lessThan16bits>
static MT_FORCEINLINE __m128i adddiff16_core_simd(const __m128i &dst, const __m128i &src, const __m128i &halfrange, const __m128i &max_pixel_value) {
  if (lessThan16bits) {
    auto result = _mm_subs_epu16(_mm_add_epi16(src, dst), halfrange);
    return _MM_MIN_EPU16(result, max_pixel_value);
  }
  else {
    auto dstval = _mm_sub_epi16(dst, halfrange);
    auto srcval = _mm_sub_epi16(src, halfrange);
    auto result = _mm_adds_epi16(dstval, srcval);
    return _mm_add_epi16(result, halfrange);
  }
}

template<bool lessThan16bits>
static void adddiff16_native_simd_t(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int nWidth, int nHeight, int bits_per_pixel)
{
    nWidth *= 2; // really rowsize: width * sizeof(uint16), see also division at C trailer

    int wMod16 = (nWidth / 16) * 16;

    auto pDst2 = pDst;
    auto pSrc2 = pSrc;
    auto halfrange = _mm_set1_epi16(int16_t(1 << (bits_per_pixel - 1)));
    auto max_pixel_value = _mm_set1_epi16(int16_t((1 << bits_per_pixel) - 1));

    for (int j = 0; j < nHeight; ++j) {
        for (int i = 0; i < wMod16; i+=16) {
            auto dst = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pDst+i));
            auto src = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pSrc+i));
            auto result = adddiff16_core_simd<lessThan16bits>(dst, src, halfrange, max_pixel_value);
            _mm_storeu_si128(reinterpret_cast<__m128i*>(pDst + i), result);
        }
        pDst += nDstPitch;
        pSrc += nSrcPitch;
    }
    if (nWidth > wMod16) {
        adddiff16_native_c(pDst2 + wMod16, nDstPitch, pSrc2 + wMod16, nSrcPitch, (nWidth - wMod16) / sizeof(uint16_t), nHeight, bits_per_pixel);
    }
}

// one template switch per call: 16 bit uses the signed pivot, 10-14 bit the unsigned add + min
void adddiff16_native_sse2_t(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int nWidth, int nHeight, int bits_per_pixel)
{
    if (bits_per_pixel == 16)
        adddiff16_native_simd_t<false>(pDst, nDstPitch, pSrc, nSrcPitch, nWidth, nHeight, bits_per_pixel);
    else
        adddiff16_native_simd_t<true>(pDst, nDstPitch, pSrc, nSrcPitch, nWidth, nHeight, bits_per_pixel);
}

} // namespace

Processor16 *adddiff16_native_sse2 = &adddiff16_native_sse2_t;

} } } } }

#endif // INTEL_INTRINSICS
