#include "../average/average.h"
#include "../../../common/common.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Support  { namespace Average {


void average16_native_c(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int nWidth, int nHeight)
{
    for (int y = 0; y < nHeight; y++) {
        auto pDstWord = reinterpret_cast<uint16_t*>(pDst);
        auto pSrcWord = reinterpret_cast<const uint16_t*>(pSrc);

        for (int x = 0; x < nWidth; x++) {
            pDstWord[x] = (pDstWord[x] + pSrcWord[x] + 1) >> 1;
        }
        pDst += nDstPitch;
        pSrc += nSrcPitch;
    }
}


} } } } }