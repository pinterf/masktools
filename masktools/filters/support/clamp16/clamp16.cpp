#include "../clamp/clamp.h"
#include "../../../common/common.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Support  { namespace Clamp {

static uint16_t MT_FORCEINLINE clamp16_core_c(uint16_t dst, uint16_t upperLimit, uint16_t lowerLimit, int overshoot, int undershoot, uint16_t max_pixel_value) {
    uint16_t result = static_cast<uint16_t>(dst > upperLimit + overshoot ? upperLimit + overshoot : dst);
    return min(static_cast<uint16_t>(result < lowerLimit - undershoot ? lowerLimit - undershoot : result), max_pixel_value);
}

void clamp16_native_c(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pUpLimit, ptrdiff_t nUpLimitPitch, const uint8_t *pLowLimit, ptrdiff_t nLowLimitPitch, int nWidth, int nHeight, int bits_per_pixel, int nOvershoot, int nUndershoot)
{
    const uint16_t max_pixel_value = uint16_t((1 << bits_per_pixel) - 1);
    for (int y = 0; y < nHeight; y++) {
        auto pDstWord = reinterpret_cast<uint16_t*>(pDst);
        auto pUpLimitWord = reinterpret_cast<const uint16_t*>(pUpLimit);
        auto pLowLimitWord = reinterpret_cast<const uint16_t*>(pLowLimit);

        for (int x = 0; x < nWidth; x++) {
            pDstWord[x] = clamp16_core_c(pDstWord[x], pUpLimitWord[x], pLowLimitWord[x], nOvershoot, nUndershoot, max_pixel_value);
        }
        pDst += nDstPitch;
        pUpLimit += nUpLimitPitch;
        pLowLimit += nLowLimitPitch;
    }
}

} } } } }
