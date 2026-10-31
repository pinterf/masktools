#include "makediff.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Support  { namespace MakeDiff {

void makediff_c(uint8_t *pDst, ptrdiff_t dst_pitch, const uint8_t *pSrc, ptrdiff_t src_pitch, int width, int height)
{
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            pDst[x] = clip<uint8_t, int>(int(pDst[x]) - pSrc[x] + 128, 0, 255);
        }
        pDst += dst_pitch;
        pSrc += src_pitch;
    }
}

void makediff32_c(uint8_t *pDst, ptrdiff_t dst_pitch, const uint8_t *pSrc, ptrdiff_t src_pitch, int width, int height, bool chroma)
{
#ifdef FLOAT_CHROMA_IS_HALF_CENTERED
  UNUSED(chroma);
  const float middle_value = 0.5f; // also for chroma
#else
  const float middle_value = chroma ? 0.0f : 0.5f;
#endif
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      reinterpret_cast<float *>(pDst)[x] = reinterpret_cast<float *>(pDst)[x] - reinterpret_cast<const float *>(pSrc)[x] + middle_value; // to work like "x y - 128 +"
    }
    pDst += dst_pitch;
    pSrc += src_pitch;
  }
}

} } } } }