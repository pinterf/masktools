#ifndef __Mt_MaskFunctions_AVX2_H__
#define __Mt_MaskFunctions_AVX2_H__

#include "../../../common/utils/utils.h"
#include "../../common/simd_avx2.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Mask {
// Anonymous namespace: this header is included from files built with different per file compiler flags.
namespace {

typedef uint8_t (Operator)(uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, const int16_t matrix[10], int nLowThreshold, int nHighThreshold);
typedef void (ProcessLineAvx2)(uint8_t *pDst, const uint8_t *pSrcp, const uint8_t *pSrc, const uint8_t *pSrcn, const int16_t matrix[10], const __m256i &lowThresh, const __m256i &highThresh, int width);

template<Operator op, class T>
void generic_avx2_c(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, T &thresholds, const int16_t matrix[10], int nWidth, int nHeight)
{
   const uint8_t *pSrcp = pSrc - nSrcPitch;
   const uint8_t *pSrcn = pSrc + nSrcPitch;

   /* top-left */
   pDst[0] = op(pSrc[0], pSrc[0], pSrc[1], pSrc[0], pSrc[0], pSrc[1], pSrcn[0], pSrcn[0], pSrcn[1], matrix, thresholds.min(0), thresholds.max(0));

   /* top */
   for ( int x = 1; x < nWidth - 1; x++ )
      pDst[x] = op(pSrc[x-1], pSrc[x], pSrc[x+1], pSrc[x-1], pSrc[x], pSrc[x+1], pSrcn[x-1], pSrcn[x], pSrcn[x+1], matrix, thresholds.min(x), thresholds.max(x));

   /* top-right */
   pDst[nWidth-1] = op(pSrc[nWidth-2], pSrc[nWidth-1], pSrc[nWidth-1], pSrc[nWidth-2], pSrc[nWidth-1], pSrc[nWidth-1], pSrcn[nWidth-2], pSrcn[nWidth-1], pSrcn[nWidth-1], matrix, thresholds.min(nWidth-1), thresholds.max(nWidth-1));

   pDst  += nDstPitch;
   pSrcp += nSrcPitch;
   pSrc  += nSrcPitch;
   pSrcn += nSrcPitch;

   thresholds.nextRow();

   for ( int y = 1; y < nHeight - 1; y++ )
   {
      /* left */
      pDst[0] = op(pSrcp[0], pSrcp[0], pSrcp[1], pSrc[0], pSrc[0], pSrc[1], pSrcn[0], pSrcn[0], pSrcn[1], matrix, thresholds.min(0), thresholds.max(0));

      /* center */
      for ( int x = 1; x < nWidth - 1; x++ )
         pDst[x] = op(pSrcp[x-1], pSrcp[x], pSrcp[x+1], pSrc[x-1], pSrc[x], pSrc[x+1], pSrcn[x-1], pSrcn[x], pSrcn[x+1], matrix, thresholds.min(x), thresholds.max(x));

      /* right */
      pDst[nWidth-1] = op(pSrcp[nWidth-2], pSrcp[nWidth-1], pSrcp[nWidth-1], pSrc[nWidth-2], pSrc[nWidth-1], pSrc[nWidth-1], pSrcn[nWidth-2], pSrcn[nWidth-1], pSrcn[nWidth-1], matrix, thresholds.min(nWidth-1), thresholds.max(nWidth-1));

      pDst  += nDstPitch;
      pSrcp += nSrcPitch;
      pSrc  += nSrcPitch;
      pSrcn += nSrcPitch;

      thresholds.nextRow();
   }

   /* bottom-left */
   pDst[0] = op(pSrcp[0], pSrcp[0], pSrcp[1], pSrc[0], pSrc[0], pSrc[1], pSrc[0], pSrc[0], pSrc[1], matrix, thresholds.min(0), thresholds.max(0));

   /* bottom */
   for ( int x = 1; x < nWidth - 1; x++ )
      pDst[x] = op(pSrcp[x-1], pSrcp[x], pSrcp[x+1], pSrc[x-1], pSrc[x], pSrc[x+1], pSrc[x-1], pSrc[x], pSrc[x+1], matrix, thresholds.min(x), thresholds.max(x));

   /* bottom-right */
   pDst[nWidth-1] = op(pSrcp[nWidth-2], pSrcp[nWidth-1], pSrcp[nWidth-1], pSrc[nWidth-2], pSrc[nWidth-1], pSrc[nWidth-1], pSrc[nWidth-2], pSrc[nWidth-1], pSrc[nWidth-1], matrix, thresholds.min(nWidth-1), thresholds.max(nWidth-1));
}

template<ProcessLineAvx2 process_line_left, ProcessLineAvx2 process_line, ProcessLineAvx2 process_line_right>
static void generic_avx2(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, const int16_t matrix[10], int nLowThreshold, int nHighThreshold, int nWidth, int nHeight) {
    const uint8_t *pSrcp = pSrc - nSrcPitch;
    const uint8_t *pSrcn = pSrc + nSrcPitch;

    auto v128 = _mm256_set1_epi8(uint8_t(128));
    auto low_thr_v = _mm256_set1_epi8(uint8_t(nLowThreshold));
    low_thr_v = _mm256_sub_epi8(low_thr_v, v128);
    auto high_thr_v = _mm256_set1_epi8(uint8_t(nHighThreshold));
    high_thr_v = _mm256_sub_epi8(high_thr_v, v128);

    int avx2_width = (nWidth - 1 - 32) / 32 * 32 + 32;
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