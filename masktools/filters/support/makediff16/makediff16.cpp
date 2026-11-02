#include "../makediff/makediff.h"
#include "../../../common/common.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Support  { namespace MakeDiff {

void makediff16_native_c(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int nWidth, int nHeight, int bits_per_pixel)
{
    const int max_pixel_value = (1 << bits_per_pixel) - 1;
    const int half = 1 << (bits_per_pixel - 1);
    for (int y = 0; y < nHeight; y++) {
        auto pDstWord = reinterpret_cast<uint16_t*>(pDst);
        auto pSrcWord = reinterpret_cast<const uint16_t*>(pSrc);

        for (int x = 0; x < nWidth; x++) {
            const int dst = pDstWord[x];
            const int src = pSrcWord[x];
            pDstWord[x] = clip<uint16_t, int>(int(dst) - src + half, 0, max_pixel_value);
        }
        pDst += nDstPitch;
        pSrc += nSrcPitch;
    }
}

} } } } }
