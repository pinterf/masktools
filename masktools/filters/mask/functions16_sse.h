#ifndef __Mt_functions16_sse_H__
#define __Mt_functions16_sse_H__

#include "functions16.h"
#include "../../common/simd.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Mask {
// Anonymous namespace: this header is included from files built with different per file compiler flags.
namespace {

typedef void (ProcessLineSse2)(uint8_t *pDst, const uint8_t *pSrcp, const uint8_t *pSrc, const uint8_t *pSrcn, const int16_t matrix[10], const __m128i &lowThresh, const __m128i &highThresh, int width);

template<int bits_per_pixel, ProcessLineSse2 process_line_left, ProcessLineSse2 process_line, ProcessLineSse2 process_line_right>
void generic16_sse2(uint16_t *pDst0, ptrdiff_t nDstPitch, const uint16_t *pSrc0, ptrdiff_t nSrcPitch, const int16_t matrix[10], int nLowThreshold, int nHighThreshold, int nWidth, int nHeight) {
    uint8_t *pDst = reinterpret_cast<uint8_t *>(pDst0);
    const uint8_t *pSrc = reinterpret_cast<const uint8_t *>(pSrc0);

    nWidth *= sizeof(uint16_t); // byte size for sse

    const uint8_t *pSrcp = pSrc - nSrcPitch;
    const uint8_t *pSrcn = pSrc + nSrcPitch;

#pragma warning(disable: 4309)
    auto vHalf = _mm_set1_epi16(1 << (bits_per_pixel - 1));
#pragma warning(default: 4309)
    auto low_thr_v = _mm_set1_epi16(uint16_t(nLowThreshold)); // for faster sse2 signed comparison
    low_thr_v = _mm_sub_epi16(low_thr_v, vHalf);
    auto high_thr_v = _mm_set1_epi16(uint16_t(nHighThreshold));
    high_thr_v = _mm_sub_epi16(high_thr_v, vHalf);

    int sse2_width = (nWidth - sizeof(uint16_t) - 16) / 16 * 16 + 16;
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

#endif // __Mt_functions16_sse_H__
