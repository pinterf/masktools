#include "../adddiff/adddiff.h"
#include "../../../common/common.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Support  { namespace AddDiff {

template<int bits_per_pixel>
static MT_FORCEINLINE uint16_t adddiff16_core_c(uint16_t dst, uint16_t src) {
    const int max_pixel_value = (1 << bits_per_pixel) - 1;
    const int half = 1 << (bits_per_pixel - 1);
    return clip<uint16_t, int>(int(dst) + src - (half), 0, max_pixel_value);
}



template<int bits_per_pixel>
void adddiff16_native_c(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int nWidth, int nHeight, int)
{
    for (int y = 0; y < nHeight; y++) {
        auto pDstWord = reinterpret_cast<uint16_t*>(pDst);
        auto pSrcWord = reinterpret_cast<const uint16_t*>(pSrc);

        for (int x = 0; x < nWidth; x++) { // width is real width
          pDstWord[x] = adddiff16_core_c<bits_per_pixel>(pDstWord[x], pSrcWord[x]);
        }
        pDst += nDstPitch;
        pSrc += nSrcPitch;
    }
}

#define MAKE_TEMPLATES(bits_per_pixel) \
template void adddiff16_native_c<bits_per_pixel>(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int nWidth, int nHeight, int nOrigHeight);

MAKE_TEMPLATES(10)
MAKE_TEMPLATES(12)
MAKE_TEMPLATES(14)
MAKE_TEMPLATES(16)
#undef MAKE_TEMPLATES

#define MAKE_EXPORTS(bits_per_pixel) \
Processor16 *adddiff16_native_##bits_per_pixel##_c = &adddiff16_native_c<bits_per_pixel>;

MAKE_EXPORTS(10)
MAKE_EXPORTS(12)
MAKE_EXPORTS(14)
MAKE_EXPORTS(16)
#undef MAKE_EXPORTS
} } } } }