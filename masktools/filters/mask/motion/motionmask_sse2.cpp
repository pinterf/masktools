#include "motionmask.h"

#ifdef INTEL_INTRINSICS

#include "../../../common/simd.h"
#include "motionmask_c.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Mask { namespace Motion {

namespace {

  static uint64_t sad_sse2_op(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int nWidth, int nHeight)
  {
    uint64_t sad = 0; // avoid overflow
    int wMod16 = (nWidth / 16) * 16;
    auto pDstSave = pDst;
    auto pSrcSave = pSrc;
    for (int j = 0; j < nHeight; ++j) {
      __m128i acc = _mm_setzero_si128();
      for (int i = 0; i < wMod16; i += 16) {
        auto dst1 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pDst + i));
        auto src1 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pSrc + i));

        auto sad1 = _mm_sad_epu8(dst1, src1);

        acc = _mm_add_epi32(acc, sad1);
      }
      auto idk = _mm_castps_si128(_mm_movehl_ps(_mm_setzero_ps(), _mm_castsi128_ps(acc)));
      auto sum = _mm_add_epi32(acc, idk);
      auto sad32 = _mm_cvtsi128_si32(sum);
      sad += sad32;
      pDst += nDstPitch;
      pSrc += nSrcPitch;
    }

    if (nWidth > wMod16) {
      sad += sad_c_op(pDstSave + wMod16, nDstPitch, pSrcSave + wMod16, nSrcPitch, nWidth - wMod16, nHeight);
    }
    return sad;
  }

  static void mask_sse2_op(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int nLowThreshold, int nHighThreshold, int nWidth, int nHeight)
  {
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
      mask_c_op(pDst2 + wMod16, nDstPitch, pSrc2 + wMod16, nSrcPitch, nLowThreshold, nHighThreshold, nWidth - wMod16, nHeight);
    }
  }

} // namespace

Processor *mask_sse2 = &mask_t<sad_sse2_op, mask_sse2_op>;

} } } } }

#endif // INTEL_INTRINSICS
