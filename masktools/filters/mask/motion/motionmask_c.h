#ifndef __Mt_motionmask_C_H__
#define __Mt_motionmask_C_H__

// motionmask: C functions, also used by the SIMD versions. Private header of the filter.

#include "motionmask.h"
#include "../../../../common/functions/functions.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Mask { namespace Motion {

namespace {

  static uint64_t sad_c_op(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int nWidth, int nHeight)
  {
    uint64_t nSad = 0;
    for (int y = 0; y < nHeight; y++)
    {
      unsigned int nSad32 = 0;
      for (int x = 0; x < nWidth; x++)
        nSad32 += abs(pDst[x] - pSrc[x]);
      nSad += nSad32; // avoid overflow for big frames
      pDst += nDstPitch;
      pSrc += nSrcPitch;
    }
    return nSad;
  }

  static void mask_c_op(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int nLowThreshold, int nHighThreshold, int nWidth, int nHeight)
  {
    for (int y = 0; y < nHeight; y++)
    {
      for (int x = 0; x < nWidth; x++)
        pDst[x] = threshold<uint8_t, int>(abs<int>(pDst[x] - pSrc[x]), nLowThreshold, nHighThreshold);
      pDst += nDstPitch;
      pSrc += nSrcPitch;
    }
  }

// Very wide planes in column chunks: the tiers sum a row in 32 bit, some of them signed. 255 * 2^23 < 2^31.
template <decltype(sad_c_op) sad>
uint64_t sad_chunked(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int nWidth, int nHeight)
{
    const int nChunk = 1 << 23;
    uint64_t nSad = 0;
    for (int x = 0; x < nWidth; x += min<int>(nChunk, nWidth - x))
        nSad += sad(pDst + (ptrdiff_t)x, nDstPitch, pSrc + (ptrdiff_t)x, nSrcPitch, min<int>(nChunk, nWidth - x), nHeight);
    return nSad;
}

template <decltype(sad_c_op) sad, decltype(mask_c_op) mask>
bool mask_t(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int nLowThreshold, int nHighThreshold, int nMotionThreshold, int nSceneChange, int nSceneChangeValue, int nWidth, int nHeight)
{
    bool scenechange = nSceneChange >= 2 ? nSceneChange == 3 : sad_chunked<sad>(pDst, nDstPitch, pSrc, nSrcPitch, nWidth, nHeight) > (uint64_t)nMotionThreshold * nWidth * nHeight;

    if (scenechange) {
        Functions::memset_plane(pDst, nDstPitch, nWidth, nHeight, static_cast<uint8_t>(nSceneChangeValue));
    } else {
        mask(pDst, nDstPitch, pSrc, nSrcPitch, nLowThreshold, nHighThreshold, nWidth, nHeight);
    }
    return scenechange;
}

} // namespace

} } } } }

#endif // __Mt_motionmask_C_H__
