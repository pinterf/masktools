#ifndef __Mt_MaskFunctions16_AVX2_H__
#define __Mt_MaskFunctions16_AVX2_H__

#include "../../../common/utils/utils.h"
#include "../../common/simd_avx2.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Mask {
// Anonymous namespace: this header is included from files built with different per file compiler flags.
namespace {

typedef void (ProcessLineAvx2)(uint8_t *pDst, const uint8_t *pSrcp, const uint8_t *pSrc, const uint8_t *pSrcn, const int16_t matrix[10], const __m256i &lowThresh, const __m256i &highThresh, int width);

// todo bitdepth template
// nWidth >= 17 (one vector + 1; processor constraint): with one vector the right pass recomputes x=0, reading pSrc-1
// nHeight >= 2: separate top and bottom row passes (smaller planes go to C, filter.h plane_flags)
template<int bits_per_pixel, ProcessLineAvx2 process_line_left, ProcessLineAvx2 process_line, ProcessLineAvx2 process_line_right>
void generic16_avx2(uint16_t *pDst0, ptrdiff_t nDstPitch, const uint16_t *pSrc0, ptrdiff_t nSrcPitch, const int16_t matrix[10], int nLowThreshold, int nHighThreshold, int nWidth, int nHeight) {
    uint8_t *pDst = reinterpret_cast<uint8_t *>(pDst0);
    const uint8_t *pSrc = reinterpret_cast<const uint8_t *>(pSrc0);

    nWidth *= sizeof(uint16_t); // byte size for sse

    const uint8_t *pSrcp = pSrc - nSrcPitch;
    const uint8_t *pSrcn = pSrc + nSrcPitch;

    // signed comparison in threshold16_avx2
    auto low_thr_v = _mm256_set1_epi16(int16_t(nLowThreshold ^ 0x8000));
    auto high_thr_v = _mm256_set1_epi16(int16_t(nHighThreshold ^ 0x8000));

    int avx2_width = (nWidth - sizeof(uint16_t) - 32) / 32 * 32 + 32;
    /* top-left */
    process_line_left(pDst, pSrc, pSrc, pSrcn, matrix, low_thr_v, high_thr_v, 32);
    /* top */
    process_line(pDst + 32, pSrc+32, pSrc+32, pSrcn+32, matrix, low_thr_v, high_thr_v, avx2_width - 32);

    /* top-right */
    process_line_right(pDst + nWidth - 32, pSrc + nWidth - 32, pSrc + nWidth - 32, pSrcn + nWidth - 32, matrix, low_thr_v, high_thr_v, 32);

    pDst  += nDstPitch;
    pSrcp += nSrcPitch;
    pSrc  += nSrcPitch;
    pSrcn += nSrcPitch;

    for ( int y = 1; y < nHeight-1; y++ )
    {
        /* left */
        process_line_left(pDst, pSrcp, pSrc, pSrcn, matrix, low_thr_v, high_thr_v, 32);
        /* center */
        process_line(pDst + 32, pSrcp+32, pSrc+32, pSrcn+32, matrix, low_thr_v, high_thr_v, avx2_width - 32);
        /* right */
        process_line_right(pDst + nWidth - 32, pSrcp + nWidth - 32, pSrc + nWidth - 32, pSrcn + nWidth - 32, matrix, low_thr_v, high_thr_v, 32);

        pDst  += nDstPitch;
        pSrcp += nSrcPitch;
        pSrc  += nSrcPitch;
        pSrcn += nSrcPitch;
    }

    /* bottom-left */
    process_line_left(pDst, pSrcp, pSrc, pSrc, matrix, low_thr_v, high_thr_v, 32);
    /* bottom */
    process_line(pDst + 32, pSrcp+32, pSrc+32, pSrc+32, matrix, low_thr_v, high_thr_v, avx2_width - 32);
    /* bottom-right */
    process_line_right(pDst + nWidth - 32, pSrcp + nWidth - 32, pSrc + nWidth - 32, pSrc + nWidth - 32, matrix, low_thr_v, high_thr_v, 32);

}

} // namespace

} } } }

#endif