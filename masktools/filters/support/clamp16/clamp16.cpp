#include "../clamp/clamp.h"
#include "../../../common/common.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Support  { namespace Clamp {

template<int bits_per_pixel>
static uint16_t MT_FORCEINLINE clamp16_core_c(uint16_t dst, uint16_t upperLimit, uint16_t lowerLimit, int overshoot, int undershoot) {
    uint16_t result = static_cast<uint16_t>(dst > upperLimit + overshoot ? upperLimit + overshoot : dst);
    if (bits_per_pixel < 16)
      return min(static_cast<uint16_t>(result < lowerLimit - undershoot ? lowerLimit - undershoot : result), uint16_t((1 << bits_per_pixel) - 1));
    else
      return static_cast<uint16_t>(result < lowerLimit - undershoot ? lowerLimit - undershoot : result);
}


template<int bits_per_pixel>
void clamp16_native_c(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pUpLimit, ptrdiff_t nUpLimitPitch, const uint8_t *pLowLimit, ptrdiff_t nLowLimitPitch, int nWidth, int nHeight, int nOrigHeight, int nOvershoot, int nUndershoot)
{
    UNUSED(nOrigHeight);
    for (int y = 0; y < nHeight; y++) {
        auto pDstWord = reinterpret_cast<uint16_t*>(pDst);
        auto pUpLimitWord = reinterpret_cast<const uint16_t*>(pUpLimit);
        auto pLowLimitWord = reinterpret_cast<const uint16_t*>(pLowLimit);

        for (int x = 0; x < nWidth; x++) {
            pDstWord[x] = clamp16_core_c<bits_per_pixel>(pDstWord[x], pUpLimitWord[x], pLowLimitWord[x], nOvershoot, nUndershoot);
        }
        pDst += nDstPitch;
        pUpLimit += nUpLimitPitch;
        pLowLimit += nLowLimitPitch;
    }
}

template void clamp16_native_c<10>(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pUpLimit, ptrdiff_t nUpLimitPitch, const uint8_t *pLowLimit, ptrdiff_t nLowLimitPitch, int nWidth, int nHeight, int nOrigHeight, int nOvershoot, int nUndershoot);
template void clamp16_native_c<12>(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pUpLimit, ptrdiff_t nUpLimitPitch, const uint8_t *pLowLimit, ptrdiff_t nLowLimitPitch, int nWidth, int nHeight, int nOrigHeight, int nOvershoot, int nUndershoot);
template void clamp16_native_c<14>(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pUpLimit, ptrdiff_t nUpLimitPitch, const uint8_t *pLowLimit, ptrdiff_t nLowLimitPitch, int nWidth, int nHeight, int nOrigHeight, int nOvershoot, int nUndershoot);
template void clamp16_native_c<16>(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pUpLimit, ptrdiff_t nUpLimitPitch, const uint8_t *pLowLimit, ptrdiff_t nLowLimitPitch, int nWidth, int nHeight, int nOrigHeight, int nOvershoot, int nUndershoot);

Processor16 *clamp16_native_10_c = &clamp16_native_c<10>;
Processor16 *clamp16_native_12_c = &clamp16_native_c<12>;
Processor16 *clamp16_native_14_c = &clamp16_native_c<14>;
Processor16 *clamp16_native_16_c = &clamp16_native_c<16>;


} } } } }