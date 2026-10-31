#ifndef __Mt_logic16_C_H__
#define __Mt_logic16_C_H__

// logic16: C ops, also for the right edge of the SIMD versions. Private header of the filter.

#include "logic.h"
#include "../../common/16bit.h"

using namespace Filtering;

namespace {

// remarks:
// modes containing min, max, add require SSE4
// bits_per_pixel templating for add16: needs because of clamping to a value less than max(uint16_t)

template<int bits_per_pixel>
static MT_FORCEINLINE uint16_t add16_c(uint16_t a, uint16_t b) { return (uint16_t)min(a + (int)b, (1 << bits_per_pixel) - 1); }
static MT_FORCEINLINE uint16_t sub16_c(uint16_t a, uint16_t b) { return clip<uint16_t, int>(a - (int)b); }
static MT_FORCEINLINE uint16_t nop16_c(uint16_t a, uint16_t b) { UNUSED(b); return a; }

static MT_FORCEINLINE uint16_t and16_c(uint16_t a, uint16_t b, uint16_t th1, uint16_t th2) { UNUSED(th1); UNUSED(th2); return a & b; }
static MT_FORCEINLINE uint16_t or16_c(uint16_t a, uint16_t b, uint16_t th1, uint16_t th2) { UNUSED(th1); UNUSED(th2); return a | b; }
// "andn" is ~a & b: the operand order of the x86 andnot (pandn), what the SIMD versions always did
static MT_FORCEINLINE uint16_t andn16_c(uint16_t a, uint16_t b, uint16_t th1, uint16_t th2) { UNUSED(th1); UNUSED(th2); return ~a & b; }
static MT_FORCEINLINE uint16_t xor16_c(uint16_t a, uint16_t b, uint16_t th1, uint16_t th2) { UNUSED(th1); UNUSED(th2); return a ^ b; }

template <decltype(nop16_c) opa, decltype(nop16_c) opb>
static MT_FORCEINLINE uint16_t min_t(uint16_t a, uint16_t b, uint16_t th1, uint16_t th2) { 
    return min<uint16_t>(opa(a, th1), opb(b, th2)); 
}

template <decltype(nop16_c) opa, decltype(nop16_c) opb>
static MT_FORCEINLINE uint16_t max_t(uint16_t a, uint16_t b, uint16_t th1, uint16_t th2) { 
    return max<uint16_t>(opa(a, th1), opb(b, th2)); 
}

template <decltype(and16_c) op>
static void logic16_stacked_t(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int nWidth, int nHeight, int nOrigHeight, uint16_t nThresholdDestination, uint16_t nThresholdSource)
{
    auto pDstLsb = pDst + nDstPitch * nOrigHeight / 2;
    auto pSrcLsb = pSrc + nSrcPitch * nOrigHeight / 2;

    for (int y = 0; y < nHeight / 2; y++) {
        for (int x = 0; x < nWidth; x++) {
            uint16_t dst = read_word_stacked(pDst, pDstLsb, x);
            uint16_t src = read_word_stacked(pSrc, pSrcLsb, x);

            uint16_t avg = op(dst, src, nThresholdDestination, nThresholdSource);

            write_word_stacked(pDst, pDstLsb, x, avg);
        }
        pDst += nDstPitch;
        pDstLsb += nDstPitch;
        pSrc += nSrcPitch;
        pSrcLsb += nSrcPitch;
    }
}

template <decltype(and16_c) op>
static void logic16_native_t(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int nWidth, int nHeight, int nOrigHeight, uint16_t nThresholdDestination, uint16_t nThresholdSource)
{
    UNUSED(nOrigHeight);
    for (int y = 0; y < nHeight; y++) {
        auto pDstWord = reinterpret_cast<uint16_t*>(pDst);
        auto pSrcWord = reinterpret_cast<const uint16_t*>(pSrc);

        for (int x = 0; x < nWidth; x++) {
            pDstWord[x] = op(pDstWord[x], pSrcWord[x], nThresholdDestination, nThresholdSource);
        }
        pDst += nDstPitch;
        pSrc += nSrcPitch;
    }
}

} // namespace

#endif // __Mt_logic16_C_H__
