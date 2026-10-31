#include "clamp.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Support  { namespace Clamp {

void clamp_c(uint8_t *pDst, ptrdiff_t dst_pitch, const uint8_t *pSrc1, ptrdiff_t src1_pitch, const uint8_t *pSrc2, ptrdiff_t src2_pitch, int width, int height, int overshoot, int undershoot)
{
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            pDst[x] = static_cast<uint8_t>(pDst[x] > pSrc1[x] + overshoot ? pSrc1[x] + overshoot : pDst[x]);
            pDst[x] = static_cast<uint8_t>(pDst[x] < pSrc2[x] - undershoot ? pSrc2[x] - undershoot : pDst[x]);
        }
        pDst += dst_pitch;
        pSrc1 += src1_pitch;
        pSrc2 += src2_pitch;
    }
}

void clamp32_c(uint8_t *pDst, ptrdiff_t dst_pitch, const uint8_t *pSrc1, ptrdiff_t src1_pitch, const uint8_t *pSrc2, ptrdiff_t src2_pitch, int width, int height, float overshoot, float undershoot)
{
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      reinterpret_cast<float *>(pDst)[x] = (reinterpret_cast<float *>(pDst)[x] > reinterpret_cast<const float *>(pSrc1)[x] + overshoot ? reinterpret_cast<const float *>(pSrc1)[x] + overshoot : reinterpret_cast<float *>(pDst)[x]);
      reinterpret_cast<float *>(pDst)[x] = (reinterpret_cast<float *>(pDst)[x] < reinterpret_cast<const float *>(pSrc2)[x] - undershoot ? reinterpret_cast<const float *>(pSrc2)[x] - undershoot : reinterpret_cast<float *>(pDst)[x]);
    }
    pDst += dst_pitch;
    pSrc1 += src1_pitch;
    pSrc2 += src2_pitch;
  }
}

} } } } }