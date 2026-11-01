#ifndef __Mt_MaskFunctions16_H__
#define __Mt_MaskFunctions16_H__

#include "../../../common/utils/utils.h"
#include "../../common/common.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Mask {
// Anonymous namespace: this header is included from files built with different per file compiler flags.
namespace {

typedef uint16_t (Operator)(uint16_t, uint16_t, uint16_t, uint16_t, uint16_t, uint16_t, uint16_t, uint16_t, uint16_t, const int16_t matrix[10], int nLowThreshold, int nHighThreshold);

template<Operator op, class T>
void generic16_c(uint16_t *pDst, ptrdiff_t nDstPitch, const uint16_t *pSrc, ptrdiff_t nSrcPitch, T &thresholds, const int16_t matrix[10], int nWidth, int nHeight)
{
  nDstPitch /= sizeof(uint16_t);
  nSrcPitch /= sizeof(uint16_t);
   // rows and columns are clamped at the plane edges: a plane can be 1 pixel wide or high
   for ( int y = 0; y < nHeight; y++ )
   {
      const uint16_t *pSrcp = y > 0 ? pSrc - nSrcPitch : pSrc;
      const uint16_t *pSrcn = y < nHeight - 1 ? pSrc + nSrcPitch : pSrc;

      if ( nWidth == 1 )
         pDst[0] = op(pSrcp[0], pSrcp[0], pSrcp[0], pSrc[0], pSrc[0], pSrc[0], pSrcn[0], pSrcn[0], pSrcn[0], matrix, thresholds.min(0), thresholds.max(0));
      else
      {
         /* left */
         pDst[0] = op(pSrcp[0], pSrcp[0], pSrcp[1], pSrc[0], pSrc[0], pSrc[1], pSrcn[0], pSrcn[0], pSrcn[1], matrix, thresholds.min(0), thresholds.max(0));

         /* center */
         for ( int x = 1; x < nWidth - 1; x++ )
            pDst[x] = op(pSrcp[x-1], pSrcp[x], pSrcp[x+1], pSrc[x-1], pSrc[x], pSrc[x+1], pSrcn[x-1], pSrcn[x], pSrcn[x+1], matrix, thresholds.min(x), thresholds.max(x));

         /* right */
         pDst[nWidth-1] = op(pSrcp[nWidth-2], pSrcp[nWidth-1], pSrcp[nWidth-1], pSrc[nWidth-2], pSrc[nWidth-1], pSrc[nWidth-1], pSrcn[nWidth-2], pSrcn[nWidth-1], pSrcn[nWidth-1], matrix, thresholds.min(nWidth-1), thresholds.max(nWidth-1));
      }

      pDst += nDstPitch;
      pSrc += nSrcPitch;

      thresholds.nextRow();
   }
}

} // namespace

} } } }

#endif