#ifndef __Mt_logic32_C_H__
#define __Mt_logic32_C_H__

// logic32: C ops, also for the right edge of the SIMD versions. Private header of the filter.

#include "logic.h"

using namespace Filtering;

namespace {

static MT_FORCEINLINE float add(float a, float b) { return a + b; }
static MT_FORCEINLINE float sub(float a, float b) { return a - b; }
static MT_FORCEINLINE float nop(float a, float b) { UNUSED(b); return a; }

#define CAST_U32(x) (*reinterpret_cast<uint32_t *>(&x))

static MT_FORCEINLINE float cast_to_float(uint32_t x)
{
  uint32_t tmp = x; return *reinterpret_cast<float *>(&tmp);
}
// not much sense on float, but for the sake of generality
static MT_FORCEINLINE float _and(float a, float b, float th1, float th2) { UNUSED(th1); UNUSED(th2); return cast_to_float(CAST_U32(a) & CAST_U32(b)); }
static MT_FORCEINLINE float _or (float a, float b, float th1, float th2) { UNUSED(th1); UNUSED(th2); return cast_to_float(CAST_U32(a) | CAST_U32(b)); }
// "andn" is ~a & b: the operand order of the x86 andnot (pandn), what the SIMD versions always did
static MT_FORCEINLINE float andn(float a, float b, float th1, float th2) { UNUSED(th1); UNUSED(th2); return cast_to_float(~CAST_U32(a) & CAST_U32(b)); }
static MT_FORCEINLINE float _xor(float a, float b, float th1, float th2) { UNUSED(th1); UNUSED(th2); return cast_to_float(CAST_U32(a) ^ CAST_U32(b)); }

template <decltype(add) opa, decltype(add) opb>
static MT_FORCEINLINE float min_t(float a, float b, float th1, float th2) { 
    return min<float>(opa(a, th1), opb(b, th2)); 
}

template <decltype(add) opa, decltype(add) opb>
static MT_FORCEINLINE float max_t(float a, float b, float th1, float th2) { 
    return max<float>(opa(a, th1), opb(b, th2)); 
}

template <decltype(_and) op>
static void logic_t(float *pDst, ptrdiff_t nDstPitch, const float *pSrc, ptrdiff_t nSrcPitch, int nWidth, int nHeight, float nThresholdDestination, float nThresholdSource)
{
   nDstPitch /= sizeof(float);
   nSrcPitch /= sizeof(float);
   for ( int y = 0; y < nHeight; y++ )
   {
      for ( int x = 0; x < nWidth; x++ )
         pDst[x] = op(pDst[x], pSrc[x], nThresholdDestination, nThresholdSource);
      pDst += nDstPitch;
      pSrc += nSrcPitch;
   }
}

} // namespace

#endif // __Mt_logic32_C_H__
