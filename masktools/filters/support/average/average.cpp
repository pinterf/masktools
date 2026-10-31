#include "average.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Support  { namespace Average {

void average_c(uint8_t *pDst, ptrdiff_t dst_pitch, const uint8_t *pSrc, ptrdiff_t src_pitch, int width, int height)
{
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            pDst[x] = (int(pDst[x]) + pSrc[x] + 1) >> 1;
        }
        pDst += dst_pitch;
        pSrc += src_pitch;
    }
}

void average32_c(uint8_t *pDst, ptrdiff_t dst_pitch, const uint8_t *pSrc, ptrdiff_t src_pitch, int width, int height)
{
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      reinterpret_cast<float *>(pDst)[x] = (reinterpret_cast<float *>(pDst)[x] + reinterpret_cast<const float *>(pSrc)[x]) * 0.5f;
    }
    pDst += dst_pitch;
    pSrc += src_pitch;
  }
}

} } } } }