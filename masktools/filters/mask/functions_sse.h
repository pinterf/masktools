#ifndef __Mt_functions_sse_H__
#define __Mt_functions_sse_H__

#include "functions.h"
#include "../../common/simd.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Mask {
// Anonymous namespace: this header is included from files built with different per file compiler flags.
namespace {

typedef void (ProcessLineSse2)(uint8_t *pDst, const uint8_t *pSrcp, const uint8_t *pSrc, const uint8_t *pSrcn, const int16_t matrix[10], const __m128i &lowThresh, const __m128i &highThresh, int width);

// nWidth >= 17 (one vector + 1; processor constraint): with one vector the right pass recomputes x=0, reading pSrc-1
template<ProcessLineSse2 process_line_left, ProcessLineSse2 process_line, ProcessLineSse2 process_line_right>
static void generic_sse2(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, const int16_t matrix[10], int nLowThreshold, int nHighThreshold, int nWidth, int nHeight) {
    const uint8_t *pSrcp = pSrc - nSrcPitch;
    const uint8_t *pSrcn = pSrc + nSrcPitch;

    auto v128 = _mm_set1_epi8(uint8_t(128));
    auto low_thr_v = _mm_set1_epi8(uint8_t(nLowThreshold));
    low_thr_v = _mm_sub_epi8(low_thr_v, v128);
    auto high_thr_v = _mm_set1_epi8(uint8_t(nHighThreshold));
    high_thr_v = _mm_sub_epi8(high_thr_v, v128);

    int sse2_width = (nWidth - 1 - 16) / 16 * 16 + 16;
    /* top-left */
    process_line_left(pDst, pSrc, pSrc, pSrcn, matrix, low_thr_v, high_thr_v, 16);
    /* top */
    process_line(pDst + 16, pSrc+16, pSrc+16, pSrcn+16, matrix, low_thr_v, high_thr_v, sse2_width - 16);

    /* top-right */
    process_line_right(pDst + nWidth - 16, pSrc + nWidth - 16, pSrc + nWidth - 16, pSrcn + nWidth - 16, matrix, low_thr_v, high_thr_v, 16);

    pDst  += nDstPitch;
    pSrcp += nSrcPitch;
    pSrc  += nSrcPitch;
    pSrcn += nSrcPitch;

    for ( int y = 1; y < nHeight-1; y++ )
    {
        /* left */
        process_line_left(pDst, pSrcp, pSrc, pSrcn, matrix, low_thr_v, high_thr_v, 16);
        /* center */
        process_line(pDst + 16, pSrcp+16, pSrc+16, pSrcn+16, matrix, low_thr_v, high_thr_v, sse2_width - 16);
        /* right */
        process_line_right(pDst + nWidth - 16, pSrcp + nWidth - 16, pSrc + nWidth - 16, pSrcn + nWidth - 16, matrix, low_thr_v, high_thr_v, 16);

        pDst  += nDstPitch;
        pSrcp += nSrcPitch;
        pSrc  += nSrcPitch;
        pSrcn += nSrcPitch;
    }

    /* bottom-left */
    process_line_left(pDst, pSrcp, pSrc, pSrc, matrix, low_thr_v, high_thr_v, 16);
    /* bottom */
    process_line(pDst + 16, pSrcp+16, pSrc+16, pSrc+16, matrix, low_thr_v, high_thr_v, sse2_width - 16);
    /* bottom-right */
    process_line_right(pDst + nWidth - 16, pSrcp + nWidth - 16, pSrc + nWidth - 16, pSrc + nWidth - 16, matrix, low_thr_v, high_thr_v, 16);
}

} // namespace

} } } }

#endif // __Mt_functions_sse_H__
