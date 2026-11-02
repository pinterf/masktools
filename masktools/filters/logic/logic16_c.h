#ifndef __Mt_logic16_C_H__
#define __Mt_logic16_C_H__

// logic16: C ops, also for the right edge of the SIMD versions. Private header of the filter.

#include "logic.h"
#include "../../common/common.h"

using namespace Filtering;

namespace {

// max_pixel_value: (1 << bits_per_pixel) - 1; the add threshold is clamped to it in the constructor
static MT_FORCEINLINE uint16_t add16_c(uint16_t a, uint16_t b, uint16_t max_pixel_value) { return (uint16_t)min(a + (int)b, (int)max_pixel_value); }
static MT_FORCEINLINE uint16_t sub16_c(uint16_t a, uint16_t b, uint16_t) { return clip<uint16_t, int>(a - (int)b); }
static MT_FORCEINLINE uint16_t nop16_c(uint16_t a, uint16_t, uint16_t) { return a; }

static MT_FORCEINLINE uint16_t and16_c(uint16_t a, uint16_t b, uint16_t, uint16_t, uint16_t) { return a & b; }
static MT_FORCEINLINE uint16_t or16_c(uint16_t a, uint16_t b, uint16_t, uint16_t, uint16_t) { return a | b; }
// "andn" is ~a & b: the operand order of the x86 andnot (pandn), what the SIMD versions always did
static MT_FORCEINLINE uint16_t andn16_c(uint16_t a, uint16_t b, uint16_t, uint16_t, uint16_t) { return ~a & b; }
static MT_FORCEINLINE uint16_t xor16_c(uint16_t a, uint16_t b, uint16_t, uint16_t, uint16_t) { return a ^ b; }

template <decltype(nop16_c) opa, decltype(nop16_c) opb>
static MT_FORCEINLINE uint16_t min_t(uint16_t a, uint16_t b, uint16_t th1, uint16_t th2, uint16_t max_pixel_value) {
    return min<uint16_t>(opa(a, th1, max_pixel_value), opb(b, th2, max_pixel_value));
}

template <decltype(nop16_c) opa, decltype(nop16_c) opb>
static MT_FORCEINLINE uint16_t max_t(uint16_t a, uint16_t b, uint16_t th1, uint16_t th2, uint16_t max_pixel_value) {
    return max<uint16_t>(opa(a, th1, max_pixel_value), opb(b, th2, max_pixel_value));
}

template <decltype(and16_c) op>
static void logic16_native_t(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int nWidth, int nHeight, int bits_per_pixel, uint16_t nThresholdDestination, uint16_t nThresholdSource)
{
    const uint16_t max_pixel_value = uint16_t((1 << bits_per_pixel) - 1);
    for (int y = 0; y < nHeight; y++) {
        auto pDstWord = reinterpret_cast<uint16_t*>(pDst);
        auto pSrcWord = reinterpret_cast<const uint16_t*>(pSrc);

        for (int x = 0; x < nWidth; x++) {
            pDstWord[x] = op(pDstWord[x], pSrcWord[x], nThresholdDestination, nThresholdSource, max_pixel_value);
        }
        pDst += nDstPitch;
        pSrc += nSrcPitch;
    }
}

} // namespace

#endif // __Mt_logic16_C_H__
