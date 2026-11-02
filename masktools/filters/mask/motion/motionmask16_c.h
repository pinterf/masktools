#ifndef __Mt_motionmask16_C_H__
#define __Mt_motionmask16_C_H__

// motionmask16: C functions, also used by the SIMD versions. Private header of the filter.

#include "motionmask.h"
#include "../../../../common/functions/functions.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Mask { namespace Motion {

namespace {

  static uint64_t sad16_c_op(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int nWidth, int nHeight)
  {
    uint64_t nSad = 0;
    for (int y = 0; y < nHeight; y++)
    {
      uint32_t nSad32 = 0; // faster than int64
      for (int x = 0; x < nWidth; x++)
        nSad32 += abs(reinterpret_cast<uint16_t *>(pDst)[x] - reinterpret_cast<const uint16_t *>(pSrc)[x]);
      nSad += nSad32;
      pDst += nDstPitch;
      pSrc += nSrcPitch;
    }
    return nSad;
  }

  static void mask16_c_op(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int nLowThreshold, int nHighThreshold, int nWidth, int nHeight, int bits_per_pixel)
  {
    const int pixelMax = (1 << bits_per_pixel) - 1;
    for (int y = 0; y < nHeight; y++)
    {
      for (int x = 0; x < nWidth; x++) {
        reinterpret_cast<uint16_t *>(pDst)[x] = threshold16(abs<int>(reinterpret_cast<uint16_t *>(pDst)[x] - reinterpret_cast<const uint16_t *>(pSrc)[x]), nLowThreshold, nHighThreshold, pixelMax);
      }
      pDst += nDstPitch;
      pSrc += nSrcPitch;
    }
  }

template <decltype(sad16_c_op) sad, decltype(mask16_c_op) mask>
bool mask16_t(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int nLowThreshold, int nHighThreshold, int nMotionThreshold, int nSceneChange, int nSceneChangeValue, int nWidth, int nHeight, int bits_per_pixel)
{
    bool scenechange = nSceneChange >= 2 ? nSceneChange == 3 : sad(pDst, nDstPitch, pSrc, nSrcPitch, nWidth, nHeight) > (uint64_t)nMotionThreshold * nWidth * nHeight;

    if (scenechange) {
        Functions::memset_plane_16((uint8_t *)pDst, nDstPitch, nWidth, nHeight, static_cast<uint16_t>(nSceneChangeValue));
    } else {
        mask(pDst, nDstPitch, pSrc, nSrcPitch, nLowThreshold, nHighThreshold, nWidth, nHeight, bits_per_pixel);
    }
    return scenechange;
}

} // namespace

} } } } }

#endif // __Mt_motionmask16_C_H__
