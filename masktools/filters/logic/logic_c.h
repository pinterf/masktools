#ifndef __Mt_logic_C_H__
#define __Mt_logic_C_H__

// logic: C ops, also for the right edge of the SIMD versions. Private header of the filter.

#include "logic.h"

using namespace Filtering;

namespace {

static MT_FORCEINLINE uint8_t add(uint8_t a, uint8_t b) { return clip<uint8_t, int>(a + (int)b); }
static MT_FORCEINLINE uint8_t sub(uint8_t a, uint8_t b) { return clip<uint8_t, int>(a - (int)b); }
static MT_FORCEINLINE uint8_t nop(uint8_t a, uint8_t b) { UNUSED(b); return a; }

static MT_FORCEINLINE uint8_t _and(uint8_t a, uint8_t b, uint8_t th1, uint8_t th2) { UNUSED(th1); UNUSED(th2); return a & b; }
static MT_FORCEINLINE uint8_t _or(uint8_t a, uint8_t b, uint8_t th1, uint8_t th2) { UNUSED(th1); UNUSED(th2); return a | b; }
// "andn" is ~a & b: the operand order of the x86 andnot (pandn), what the SIMD versions always did
static MT_FORCEINLINE uint8_t andn(uint8_t a, uint8_t b, uint8_t th1, uint8_t th2) { UNUSED(th1); UNUSED(th2); return ~a & b; }
static MT_FORCEINLINE uint8_t _xor(uint8_t a, uint8_t b, uint8_t th1, uint8_t th2) { UNUSED(th1); UNUSED(th2); return a ^ b; }

template <decltype(add) opa, decltype(add) opb>
static MT_FORCEINLINE uint8_t min_t(uint8_t a, uint8_t b, uint8_t th1, uint8_t th2) { 
    return min<uint8_t>(opa(a, th1), opb(b, th2)); 
}

template <decltype(add) opa, decltype(add) opb>
static MT_FORCEINLINE uint8_t max_t(uint8_t a, uint8_t b, uint8_t th1, uint8_t th2) { 
    return max<uint8_t>(opa(a, th1), opb(b, th2)); 
}

template <decltype(_and) op>
void logic_t(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int nWidth, int nHeight, uint8_t nThresholdDestination, uint8_t nThresholdSource)
{
   for ( int y = 0; y < nHeight; y++ )
   {
      for ( int x = 0; x < nWidth; x++ )
         pDst[x] = op(pDst[x], pSrc[x], nThresholdDestination, nThresholdSource);
      pDst += nDstPitch;
      pSrc += nSrcPitch;
   }
}

} // namespace

#endif // __Mt_logic_C_H__
