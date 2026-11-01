#include "motionmask.h"

#ifdef INTEL_INTRINSICS

#include "../../../common/simd.h"
#include "motionmask16_c.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Mask { namespace Motion {

namespace {

  template <MemoryMode mem_mode>
  static uint64_t sad16_sse2_op(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int nWidth, int nHeight)
  {
    nWidth *= sizeof(uint16_t);
    uint64_t sad = 0; // avoid overflow
    int wMod16 = (nWidth / 16) * 16;
    auto pDstSave = pDst;
    auto pSrcSave = pSrc;
    auto zero = _mm_setzero_si128();
    for (int y = 0; y < nHeight; ++y) {
      __m128i acc = zero; // avoid overflow. calc line by line
      for (int x = 0; x < wMod16; x += 16) {
        auto dst1 = simd_load_si128<mem_mode>(pDst + x);
        auto src1 = simd_load_si128<mem_mode>(pSrc + x);

        __m128i greater_t = _mm_subs_epu16(dst1, src1); // unsigned sub with saturation
        __m128i smaller_t = _mm_subs_epu16(src1, dst1);
        __m128i absdiff = _mm_or_si128(greater_t, smaller_t); //abs(s1-s2)  == (satsub(s1,s2) | satsub(s2,s1))
                                                              // 8 x uint16 absolute differences
        acc = _mm_add_epi32(acc, _mm_unpacklo_epi16(absdiff, zero));
        acc = _mm_add_epi32(acc, _mm_unpackhi_epi16(absdiff, zero));
        // sum0_32, sum1_32, sum2_32, sum3_32
      }
      // summing up partial sums
      // we have 4 integers for sum: a0 a1 a2 a3
      __m128i a0_a1 = _mm_unpacklo_epi32(acc, zero); // a0 0 a1 0
      __m128i a2_a3 = _mm_unpackhi_epi32(acc, zero); // a2 0 a3 0
      acc = _mm_add_epi32(a0_a1, a2_a3); // a0+a2, 0, a1+a3, 0
                                         /* SSSE3: told to be not too fast
                                         sum = _mm_hadd_epi32(sum, zero);  // A1+A2, B1+B2, 0+0, 0+0
                                         sum = _mm_hadd_epi32(sum, zero);  // A1+A2+B1+B2, 0+0+0+0, 0+0+0+0, 0+0+0+0
                                         */

                                         // sum here: two 32 bit partial result: sum1 0 sum2 0
      __m128i acc_hi = _mm_unpackhi_epi64(acc, zero);
      // or: __m128i sum_hi = _mm_castps_si128(_mm_movehl_ps(_mm_setzero_ps(), _mm_castsi128_ps(sum)));
      acc = _mm_add_epi32(acc, acc_hi);
      int rowsum = _mm_cvtsi128_si32(acc);
      sad += rowsum;

      pDst += nDstPitch;
      pSrc += nSrcPitch;
    }

    if (nWidth > wMod16) {
      sad += sad16_c_op(pDstSave + wMod16, nDstPitch, pSrcSave + wMod16, nSrcPitch, (nWidth - wMod16) / sizeof(uint16_t), nHeight);
    }
    return sad;
  }

#if 0
  todo simd
  template <MemoryMode mem_mode>
  static void mask16_sse2_op(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int nLowThreshold, int nHighThreshold, int nWidth, int nHeight)
  {
    nWidth *= sizeof(uint16_t);
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
        auto dst1 = simd_load_si128<mem_mode>(pDst + i);
        auto src1 = simd_load_si128<mem_mode>(pSrc + i);

        auto greater = _mm_subs_epu8(dst1, src1);
        auto smaller = _mm_subs_epu8(src1, dst1);
        auto diff = _mm_add_epi8(greater, smaller);

        auto mask = threshold_sse2(diff, lowThresh, highThresh, v128);

        simd_store_si128<mem_mode>(pDst + i, mask);
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

Processor16* mask10_sse2 = &mask16_t<sad16_sse2_op<MemoryMode::SSE2_UNALIGNED>, mask16_c_op<10> /* mask16_sse2_op<MemoryMode::SSE2_UNALIGNED>*/>;
Processor16* mask12_sse2 = &mask16_t<sad16_sse2_op<MemoryMode::SSE2_UNALIGNED>, mask16_c_op<12> /* mask16_sse2_op<MemoryMode::SSE2_UNALIGNED>*/>;
Processor16* mask14_sse2 = &mask16_t<sad16_sse2_op<MemoryMode::SSE2_UNALIGNED>, mask16_c_op<14> /* mask16_sse2_op<MemoryMode::SSE2_UNALIGNED>*/>;
Processor16* mask16_sse2 = &mask16_t<sad16_sse2_op<MemoryMode::SSE2_UNALIGNED>, mask16_c_op<16> /* mask16_sse2_op<MemoryMode::SSE2_UNALIGNED>*/>;

} } } } }

#endif // INTEL_INTRINSICS
