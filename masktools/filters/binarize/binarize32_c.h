#ifndef __Mt_binarize32_C_H__
#define __Mt_binarize32_C_H__

// binarize32: C ops, also for the right edge of the SIMD versions. Private header of the filter.

#include "binarize.h"

using namespace Filtering;

namespace {

typedef float (Operator32)(float, float);

inline float binarize32_upper(float x, float t) { return x > t ? 0 : 1.0f; }
inline float binarize32_lower(float x, float t) { return x > t ? 1.0f : 0; }

inline float binarize32_0_x(float x, float t) { return x > t ? 0 : x; }
inline float binarize32_t_x(float x, float t) { return x > t ? t : x; }
inline float binarize32_x_0(float x, float t) { return x > t ? x : 0; }
inline float binarize32_x_t(float x, float t) { return x > t ? x : t; }

inline float binarize32_t_0(float x, float t) { return x > t ? t : 0; }
inline float binarize32_0_t(float x, float t) { return x > t ? 0 : t; }

inline float binarize32_x_255(float x, float t) { return x > t ? x : 1.0f; }
inline float binarize32_t_255(float x, float t) { return x > t ? t : 1.0f; }
inline float binarize32_255_x(float x, float t) { return x > t ? 1.0f : x; }
inline float binarize32_255_t(float x, float t) { return x > t ? 1.0f : t; }

template <Operator32 op>
void binarize32_t(uint8_t *pDst, ptrdiff_t nDstPitch, float nThreshold, int nWidth, int nHeight)
{
   for ( int j = 0; j < nHeight; j++, pDst += nDstPitch )
      for ( int i = 0; i < nWidth; i++ )
         reinterpret_cast<float *>(pDst)[i] = op(reinterpret_cast<float *>(pDst)[i], nThreshold);
}

} // namespace

#endif // __Mt_binarize32_C_H__
