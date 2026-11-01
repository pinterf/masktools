#ifndef __Mt_MorphologicFunctions32_H__
#define __Mt_MorphologicFunctions32_H__

#include "../../../common/utils/utils.h"
#include "../../common/common.h"
#include "../../common/common.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Morphologic {
// Anonymous namespace: this header is included from files built with different per file compiler flags.
namespace {

typedef float (Operator32)(float, float, float, float, float, float, float, float, float, float); // last is nMaxDeviation

enum Directions {
    Vertical = 1,
    Horizontal = 2,
    Both = Vertical | Horizontal,
    Square = 7
};

template<Border borderMode, Operator32 op>
void process_line_morpho_32_c(float *pDst, const float *pSrcp, const float *pSrc, const float *pSrcn, float maxDeviation, int width, int height, ptrdiff_t srcPitch, ptrdiff_t dstPitch) {
    UNUSED(dstPitch); UNUSED(srcPitch); UNUSED(height);

    const int leftOffset = borderMode == Border::Left ? 0 : 1;
    const int rightOffset = borderMode == Border::Right ? 0 : 1;

    for (int x = 0; x < width; ++x) {
        pDst[x] = op(
            reinterpret_cast<const float*>(pSrcp)[x-leftOffset],
            reinterpret_cast<const float*>(pSrcp)[x],
            reinterpret_cast<const float*>(pSrcp)[x+rightOffset],
            reinterpret_cast<const float*>(pSrc)[x-leftOffset],
            reinterpret_cast<const float*>(pSrc)[x],
            reinterpret_cast<const float*>(pSrc)[x+rightOffset],
            reinterpret_cast<const float*>(pSrcn)[x-leftOffset],
            reinterpret_cast<const float*>(pSrcn)[x],
            reinterpret_cast<const float*>(pSrcn)[x+rightOffset],
            maxDeviation);
    }
}

template<class T>
struct MorphologicProcessor {
    typedef void (ProcessLineC)(T *pDst, const T *pSrcp, const T *pSrc, const T *pSrcn, float maxDeviation, int width, int height, ptrdiff_t srcPitch, ptrdiff_t dstPitch);

    template<ProcessLineC process_line_left, ProcessLineC process_line, ProcessLineC process_line_right>
    static void generic_32_c(T *pDst, ptrdiff_t nDstPitch, const T *pSrc, ptrdiff_t nSrcPitch, float nMaxDeviation, const int *pCoordinates, int nCoordinates, int nWidth, int nHeight)
    {
        UNUSED(nCoordinates); UNUSED(pCoordinates);

        // rows and columns are clamped at the plane edges: a plane can be 1 pixel wide or high
        for ( int y = 0; y < nHeight; y++ )
        {
            const T *pSrcp = y > 0 ? pSrc - nSrcPitch : pSrc;
            const T *pSrcn = y < nHeight - 1 ? pSrc + nSrcPitch : pSrc;

            if ( nWidth == 1 )
            {
                // the left border line reads x and x+1: give it the pixel twice
                const T p[2] = { pSrcp[0], pSrcp[0] }, c[2] = { pSrc[0], pSrc[0] }, n[2] = { pSrcn[0], pSrcn[0] };
                process_line_left(pDst, p, c, n, nMaxDeviation, 1, nHeight, nSrcPitch, nDstPitch);
            }
            else
            {
                /* left */
                process_line_left(pDst, pSrcp, pSrc, pSrcn, nMaxDeviation, 1, nHeight, nSrcPitch, nDstPitch);
                /* center */
                process_line(pDst + 1, pSrcp+1, pSrc+1, pSrcn+1, nMaxDeviation, nWidth - 2, nHeight, nSrcPitch, nDstPitch);
                /* right */
                process_line_right(pDst + nWidth - 1, pSrcp + nWidth - 1, pSrc + nWidth - 1, pSrcn + nWidth - 1, nMaxDeviation, 1, nHeight, nSrcPitch, nDstPitch);
            }

            pDst += nDstPitch;
            pSrc += nSrcPitch;
        }
    }
};

template<class T>
void generic_custom_32_c(float *pDst, ptrdiff_t nDstPitch, const float *pSrc, ptrdiff_t nSrcPitch, float nMaxDeviation, const int *pCoordinates, int nCoordinates, int nWidth, int nHeight)
{
    for ( int j = 0; j < nHeight; j++ )
    {
        for ( int i = 0; i < nWidth; i++ )
        {
            T new_value(pSrc[i], nMaxDeviation);
            for ( int k = 0; k < nCoordinates; k+=2 )
            {
                if ( pCoordinates[k] + i >= 0 && pCoordinates[k] + i < nWidth &&
                    pCoordinates[k+1] + j >= 0 && pCoordinates[k+1] + j < nHeight ) {
                        new_value.add(pSrc[i + pCoordinates[k] + pCoordinates[k+1] * nSrcPitch]);
                }
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