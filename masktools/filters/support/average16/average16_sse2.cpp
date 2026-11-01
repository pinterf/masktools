#include "../average/average.h"

#ifdef INTEL_INTRINSICS

#include "../../../common/simd.h"
#include "../../../common/common.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Support  { namespace Average {

namespace {

} // namespace


void average16_native_sse2(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int nWidth, int nHeight, int nOrigHeight)
{
    nWidth *= 2; // really rowsize: width * sizeof(uint16), see also division at C trailer
  
    int wMod16 = (nWidth / 16) * 16;

    auto pDst2 = pDst;
    auto pSrc2 = pSrc;

    for (int j = 0; j < nHeight; ++j) {
        for (int i = 0; i < wMod16; i+=16) {
            auto dst = _mm_loadu_si128(reinterpret_cast<const __m128i*>(reinterpret_cast<const __m128i*>(pDst+i)));
            auto src = _mm_loadu_si128(reinterpret_cast<const __m128i*>(reinterpret_cast<const __m128i*>(pSrc+i)));

            auto result = _mm_avg_epu16(dst, src);

            _mm_storeu_si128(reinterpret_cast<__m128i*>(reinterpret_cast<__m128i*>(pDst+i)), result);
        }
        pDst += nDstPitch;
        pSrc += nSrcPitch;
    }
    if (nWidth > wMod16) {
        average16_native_c(pDst2 + wMod16, nDstPitch, pSrc2 + wMod16, nSrcPitch, (nWidth - wMod16) / sizeof(uint16_t), nHeight, nOrigHeight);
    }
}

} } } } }

#endif // INTEL_INTRINSICS
