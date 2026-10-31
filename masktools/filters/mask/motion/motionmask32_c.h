#ifndef __Mt_motionmask32_C_H__
#define __Mt_motionmask32_C_H__

// motionmask32: C functions, also used by the SIMD versions. Private header of the filter.

#include "motionmask.h"
#include "../../../../common/functions/functions.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Mask { namespace Motion {

namespace {

  static double sad32_c_op(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int nWidth, int nHeight)
  {
    double nSad = 0;
    for (int y = 0; y < nHeight; y++)
    {
      float nSad32 = 0; // faster than double
      for (int x = 0; x < nWidth; x++)
        nSad32 += abs(reinterpret_cast<float *>(pDst)[x] - reinterpret_cast<const float *>(pSrc)[x]);
      nSad += nSad32;
      pDst += nDstPitch;
      pSrc += nSrcPitch;
    }
    return nSad;
  }

  static void mask32_c_op(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, float nLowThreshold, float nHighThreshold, int nWidth, int nHeight)
  {
    for (int y = 0; y < nHeight; y++)
    {
      for (int x = 0; x < nWidth; x++) {
        // threashold function: mask value creation. base=0.0 mask=1.0
        reinterpret_cast<float *>(pDst)[x] = threshold<float, float>(abs<float>(reinterpret_cast<float *>(pDst)[x] - reinterpret_cast<const float *>(pSrc)[x]), nLowThreshold, nHighThreshold);
      }
      pDst += nDstPitch;
      pSrc += nSrcPitch;
    }
  }

template <decltype(sad32_c_op) sad, decltype(mask32_c_op) mask>
bool mask32_t(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, float nLowThreshold, float nHighThreshold, float nMotionThreshold, int nSceneChange, float nSceneChangeValue, int nWidth, int nHeight)
{
    bool scenechange = nSceneChange >= 2 ? nSceneChange == 3 : sad(pDst, nDstPitch, pSrc, nSrcPitch, nWidth, nHeight) > (double)(nMotionThreshold * nWidth * nHeight);

    if (scenechange) {
        Functions::memset_plane_32((uint8_t *)pDst, nDstPitch, nWidth, nHeight, nSceneChangeValue);
    } else {
        mask(pDst, nDstPitch, pSrc, nSrcPitch, nLowThreshold, nHighThreshold, nWidth, nHeight);
    }
    return scenechange;
}

} // namespace

} } } } }

#endif // __Mt_motionmask32_C_H__
