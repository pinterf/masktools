#include "../adddiff/adddiff.h"

#ifdef INTEL_INTRINSICS

#include "adddiff16_sse.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Support  { namespace AddDiff {

#pragma warning(disable: 4309)

void adddiff16_stacked_sse2(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int nWidth, int nHeight, int nOrigHeight)
{
    int wMod8 = (nWidth / 8) * 8;
    auto pDst2 = pDst;
    auto pSrc2 = pSrc;

    auto pDstLsb = pDst + nDstPitch * nOrigHeight / 2;
    auto pSrcLsb = pSrc + nSrcPitch * nOrigHeight / 2;

    auto ff = _mm_set1_epi16(0x00FF);
    auto zero = _mm_setzero_si128();
    auto halfrange = _mm_set1_epi16(0x8000);

    for ( int j = 0; j < nHeight / 2; ++j ) {
        for ( int i = 0; i < wMod8; i+=8 ) {
            auto dst = read_word_stacked_simd(pDst, pDstLsb, i);
            auto src = read_word_stacked_simd(pSrc, pSrcLsb, i);
            auto result = adddiff16_core_simd<CPU_SSE2,false>(dst, src, halfrange, halfrange /*dummy*/);

            write_word_stacked_simd(pDst, pDstLsb, i, result, ff, zero);
        }
        pDst += nDstPitch;
        pDstLsb += nDstPitch;
        pSrc += nSrcPitch;
        pSrcLsb += nSrcPitch;
    }

    if (nWidth > wMod8) {
        adddiff16_stacked_c(pDst2 + wMod8, nDstPitch, pSrc2 + wMod8, nSrcPitch, nWidth - wMod8, nHeight, nOrigHeight);
    }
}

#define MAKE_EXPORTS(bits_per_pixel) \
Processor16 *adddiff16_native_##bits_per_pixel##_sse2 = &adddiff16_native_simd<CPU_SSE2,bits_per_pixel>;

MAKE_EXPORTS(10)
MAKE_EXPORTS(12)
MAKE_EXPORTS(14)
MAKE_EXPORTS(16)
#undef MAKE_EXPORTS

} } } } }

#endif // INTEL_INTRINSICS
