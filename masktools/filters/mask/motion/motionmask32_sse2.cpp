#include "motionmask.h"

#ifdef INTEL_INTRINSICS

#include "../../../common/simd.h"
#include "motionmask32_c.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Mask { namespace Motion {

namespace {

  static MT_FORCEINLINE __m128 simd_abs_diff_32(__m128 a, __m128 b) {
    // maybe not optimal, mask could be generated 
    const __m128 absmask = _mm_castsi128_ps(_mm_set1_epi32(~(1 << 31))); // 0x7FFFFFFF
    return _mm_and_ps(_mm_sub_ps(a, b), absmask);
  }

  static double sad32_sse2_op(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int nWidth, int nHeight)
  {
    nWidth *= sizeof(float);
    double sad = 0; // avoid overflow
    int wMod16 = (nWidth / 16) * 16;
    auto pDstSave = pDst;
    auto pSrcSave = pSrc;
    auto zero = _mm_setzero_ps(); // avoid overflow. calc by line
    for (int y = 0; y < nHeight; ++y) {
      auto acc = zero; // avoid overflow. calc line by line
      for (int x = 0; x < wMod16; x += 16) {
        auto dst1 = _mm_loadu_ps(reinterpret_cast<const float*>(pDst + x));
        auto src1 = _mm_loadu_ps(reinterpret_cast<const float*>(pSrc + x));

        acc = _mm_add_ps(acc, simd_abs_diff_32(dst1, src1));
        // sum0_32, sum1_32, sum2_32, sum3_32
      }
      // summing up partial sums
      // we have 4 floats for sum: a0 a1 a2 a3
      __m128 a0_a1 = _mm_unpacklo_ps(acc, zero); // a0 0 a1 0
      __m128 a2_a3 = _mm_unpackhi_ps(acc, zero); // a2 0 a3 0
      acc = _mm_add_ps(a0_a1, a2_a3); // a0+a2, 0, a1+a3, 0
      // sum here: two 32 bit partial result: sum1 0 sum2 0
      __m128 acc_hi = _mm_movehl_ps(zero, acc);
      acc = _mm_add_ps(acc, acc_hi);
      float rowsum = _mm_cvtss_f32(acc);
      sad += rowsum;

      pDst += nDstPitch;
      pSrc += nSrcPitch;
    }

    if (nWidth > wMod16) {
      sad += sad32_c_op(pDstSave + wMod16, nDstPitch, pSrcSave + wMod16, nSrcPitch, (nWidth - wMod16) / sizeof(float), nHeight);
    }
    return sad;
  }

#if 0
  // todo simd
  static void mask32_sse2_op(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int nLowThreshold, int nHighThreshold, int nWidth, int nHeight)
  {
    nWidth *= sizeof(float);
    int wMod16 = (nWidth / 16) * 16;
    auto pDst2 = pDst;
    auto pSrc2 = pSrc;

    auto v128 = _mm_set1_epi32(0x80808080);
    auto lowThresh = _mm_set1_epi8(uint8_t(nLowThreshold));
    auto highThresh = _mm_set1_epi8(uint8_t(nHighThreshold));
    lowThresh = _mm_sub_epi8(lowThresh, v128);
    highThresh = _mm_sub_epi8(highThresh, v128);

    for (int j = 0; j < nHeight; ++j) {
      for (int i = 0; i < wMod16; i += 16) {
        auto dst1 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pDst + i));
        auto src1 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pSrc + i));

        auto greater = _mm_subs_epu8(dst1, src1);
        auto smaller = _mm_subs_epu8(src1, dst1);
        auto diff = _mm_add_epi8(greater, smaller);

        auto mask = threshold_sse2(diff, lowThresh, highThresh, v128);

        _mm_storeu_si128(reinterpret_cast<__m128i*>(pDst + i), mask);
      }
      pDst += nDstPitch;
      pSrc += nSrcPitch;
    }

    if (nWidth > wMod16) {
      mask16_c_op(pDst2 + wMod16, nDstPitch, pSrc2 + wMod16, nSrcPitch, nLowThreshold, nHighThreshold, (nWidth - wMod16) / sizeof(uint16_t), nHeight);
    }
  }
#endif

} // namespace

Processor32 *mask32_sse2 = &mask32_t<sad32_sse2_op, mask32_c_op /*mask16_sse2_op*/>;

} } } } }

#endif // INTEL_INTRINSICS
