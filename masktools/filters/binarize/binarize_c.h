#ifndef __Mt_binarize_C_H__
#define __Mt_binarize_C_H__

// binarize: C ops, also for the right edge of the SIMD versions. Private header of the filter.

#include "binarize.h"

using namespace Filtering;

namespace {

typedef uint8_t (Operator)(uint8_t, uint8_t);

inline uint8_t binarize_upper(uint8_t x, uint8_t t) { return x > t ? 0 : 255; }
inline uint8_t binarize_lower(uint8_t x, uint8_t t) { return x > t ? 255 : 0; }

inline uint8_t binarize_0_x(uint8_t x, uint8_t t) { return x > t ? 0 : x; }
inline uint8_t binarize_t_x(uint8_t x, uint8_t t) { return x > t ? t : x; }
inline uint8_t binarize_x_0(uint8_t x, uint8_t t) { return x > t ? x : 0; }
inline uint8_t binarize_x_t(uint8_t x, uint8_t t) { return x > t ? x : t; }

inline uint8_t binarize_t_0(uint8_t x, uint8_t t) { return x > t ? t : 0; }
inline uint8_t binarize_0_t(uint8_t x, uint8_t t) { return x > t ? 0 : t; }

inline uint8_t binarize_x_255(uint8_t x, uint8_t t) { return x > t ? x : 255; }
inline uint8_t binarize_t_255(uint8_t x, uint8_t t) { return x > t ? t : 255; }
inline uint8_t binarize_255_x(uint8_t x, uint8_t t) { return x > t ? 255 : x; }
inline uint8_t binarize_255_t(uint8_t x, uint8_t t) { return x > t ? 255 : t; }

template <Operator op>
void binarize_t(uint8_t *pDst, ptrdiff_t nDstPitch, uint8_t nThreshold, int nWidth, int nHeight)
{
   for ( int j = 0; j < nHeight; j++, pDst += nDstPitch )
      for ( int i = 0; i < nWidth; i++ )
         pDst[i] = op(pDst[i], nThreshold);
}

} // namespace

#endif // __Mt_binarize_C_H__
