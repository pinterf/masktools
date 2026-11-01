#ifndef __Mt_MaskFunctions32_H__
#define __Mt_MaskFunctions32_H__

#include "../../../common/utils/utils.h"
#include "../../common/common.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Mask {
// Anonymous namespace: this header is included from files built with different per file compiler flags.
namespace {

typedef float (Operator)(float, float, float, float, float, float, float, float, float, const float matrix[10], float nLowThreshold, float nHighThreshold);

template<Operator op, class T>
void generic32_c(float *pDst, ptrdiff_t nDstPitch, const float *pSrc, ptrdiff_t nSrcPitch, T &thresholds, const float matrix[10], int nWidth, int nHeight)
{
  nDstPitch /= sizeof(float);
  nSrcPitch /= sizeof(float);
   // rows and columns are clamped at the plane edges: a plane can be 1 pixel wide or high
   for ( int y = 0; y < nHeight; y++ )
   {
      const float *pSrcp = y > 0 ? pSrc - nSrcPitch : pSrc;
      const float *pSrcn = y < nHeight - 1 ? pSrc + nSrcPitch : pSrc;

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