#ifndef __Mt_functions32_sse_H__
#define __Mt_functions32_sse_H__

#include "functions32.h"
#include "../../common/simd.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Mask {
// Anonymous namespace: this header is included from files built with different per file compiler flags.
namespace {

typedef void (ProcessLineSse2)(uint8_t *pDst, const uint8_t *pSrcp, const uint8_t *pSrc, const uint8_t *pSrcn, const float matrix[10], const __m128 &lowThresh, const __m128 &highThresh, int width);

// nWidth >= 5 (one vector + 1; processor constraint): with one vector the right pass recomputes x=0, reading pSrc-1
// nHeight >= 2: separate top and bottom row passes (smaller planes go to C, filter.h plane_flags)
template<ProcessLineSse2 process_line_left, ProcessLineSse2 process_line, ProcessLineSse2 process_line_right>
static void generic32_sse2(float *pDst0, ptrdiff_t nDstPitch, const float *pSrc0, ptrdiff_t nSrcPitch, const float matrix[10], float nLowThreshold, float nHighThreshold, int nWidth, int nHeight) {
    uint8_t *pDst = reinterpret_cast<uint8_t *>(pDst0);
    const uint8_t *pSrc = reinterpret_cast<const uint8_t *>(pSrc0);

    nWidth *= sizeof(float); // byte size for sse
  
    const uint8_t *pSrcp = pSrc - nSrcPitch; // prev
    const uint8_t *pSrcn = pSrc + nSrcPitch; // next
    auto low_thr_v = _mm_set1_ps(nLowThreshold);
    auto high_thr_v = _mm_set1_ps(nHighThreshold);

    size_t sse2_width = (nWidth - sizeof(float) - 16) / 16 * 16 + 16;
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

#endif // __Mt_functions32_sse_H__
