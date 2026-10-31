#ifndef __Mt_MorphologicFunctions_H__
#define __Mt_MorphologicFunctions_H__

#include "../../../common/utils/utils.h"
#include "../../common/common.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Morphologic {
// Anonymous namespace: this header is included from files built with different per file compiler flags.
namespace {

typedef uint8_t (Operator)(uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, int);

enum Directions {
    Vertical = 1,
    Horizontal = 2,
    Both = Vertical | Horizontal,
    Square = 7
};

template<Operator op>
void generic_c(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int nMaxDeviation, const int *pCoordinates, int nCoordinates, int nWidth, int nHeight)
{
   const uint8_t *pSrcp = pSrc - nSrcPitch;
   const uint8_t *pSrcn = pSrc + nSrcPitch;

   UNUSED(nCoordinates); UNUSED(pCoordinates);

   /* top-left */
   pDst[0] = op(pSrc[0], pSrc[0], pSrc[1], pSrc[0], pSrc[0], pSrc[1], pSrcn[0], pSrcn[0], pSrcn[1], nMaxDeviation);

   /* top */
   for ( int x = 1; x < nWidth - 1; x++ )
      pDst[x] = op(pSrc[x-1], pSrc[x], pSrc[x+1], pSrc[x-1], pSrc[x], pSrc[x+1], pSrcn[x-1], pSrcn[x], pSrcn[x+1], nMaxDeviation);

   /* top-right */
   pDst[nWidth-1] = op(pSrc[nWidth-2], pSrc[nWidth-1], pSrc[nWidth-1], pSrc[nWidth-2], pSrc[nWidth-1], pSrc[nWidth-1], pSrcn[nWidth-2], pSrcn[nWidth-1], pSrcn[nWidth-1], nMaxDeviation);

   pDst  += nDstPitch;
   pSrcp += nSrcPitch;
   pSrc  += nSrcPitch;
   pSrcn += nSrcPitch;

   for ( int y = 1; y < nHeight - 1; y++ )
   {
      /* left */
      pDst[0] = op(pSrcp[0], pSrcp[0], pSrcp[1], pSrc[0], pSrc[0], pSrc[1], pSrcn[0], pSrcn[0], pSrcn[1], nMaxDeviation);

      /* center */
      for ( int x = 1; x < nWidth - 1; x++ )
         pDst[x] = op(pSrcp[x-1], pSrcp[x], pSrcp[x+1], pSrc[x-1], pSrc[x], pSrc[x+1], pSrcn[x-1], pSrcn[x], pSrcn[x+1], nMaxDeviation);

      /* right */
      pDst[nWidth-1] = op(pSrcp[nWidth-2], pSrcp[nWidth-1], pSrcp[nWidth-1], pSrc[nWidth-2], pSrc[nWidth-1], pSrc[nWidth-1], pSrcn[nWidth-2], pSrcn[nWidth-1], pSrcn[nWidth-1], nMaxDeviation);

      pDst  += nDstPitch;
      pSrcp += nSrcPitch;
      pSrc  += nSrcPitch;
      pSrcn += nSrcPitch;
   }

   /* bottom-left */
   pDst[0] = op(pSrcp[0], pSrcp[0], pSrcp[1], pSrc[0], pSrc[0], pSrc[1], pSrc[0], pSrc[0], pSrc[1], nMaxDeviation);

   /* bottom */
   for ( int x = 1; x < nWidth - 1; x++ )
      pDst[x] = op(pSrcp[x-1], pSrcp[x], pSrcp[x+1], pSrc[x-1], pSrc[x], pSrc[x+1], pSrc[x-1], pSrc[x], pSrc[x+1], nMaxDeviation);

   /* bottom-right */
   pDst[nWidth-1] = op(pSrcp[nWidth-2], pSrcp[nWidth-1], pSrcp[nWidth-1], pSrc[nWidth-2], pSrc[nWidth-1], pSrc[nWidth-1], pSrc[nWidth-2], pSrc[nWidth-1], pSrc[nWidth-1], nMaxDeviation);
}

/* Horizontal mt_xxpand */

static MT_FORCEINLINE uint8_t expand_c_horizontal_core(uint8_t left, uint8_t center, uint8_t right, uint8_t max_dev) {
    uint8_t ma = left;

    if (center > ma) ma = center;
    if (right > ma) ma = right;
    
    if (ma - center > max_dev) ma = center + max_dev;
    return static_cast<uint8_t>(ma);
}

static MT_FORCEINLINE uint8_t inpand_c_horizontal_core(uint8_t left, uint8_t center, uint8_t right, uint8_t max_dev) {
    uint8_t mi = left;

    if (center < mi) mi = center;
    if (right < mi) mi = right;

    if (center - mi > max_dev) mi = center - max_dev;
    return static_cast<uint8_t>(mi);
}

template<class T>
void generic_custom_c(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int nMaxDeviation, const int *pCoordinates, int nCoordinates, int nWidth, int nHeight)
{
   for ( int j = 0; j < nHeight; j++ )
   {
      for ( int i = 0; i < nWidth; i++ )
      {
         T new_value(pSrc[i], nMaxDeviation);
         for ( int k = 0; k < nCoordinates; k+=2 )
         {
            if ( pCoordinates[k] + i >= 0 && pCoordinates[k] + i < nWidth &&
                 pCoordinates[k+1] + j >= 0 && pCoordinates[k+1] + j < nHeight )
               new_value.add(pSrc[i + pCoordinates[k] + pCoordinates[k+1] * nSrcPitch]);
         }
         pDst[i] = new_value.finalize();
      }
      pSrc += nSrcPitch;
      pDst += nDstPitch;
   }
}

} // namespace

} } } } // namespace Morphologic, Filters, MaskTools, Filtering

#endif