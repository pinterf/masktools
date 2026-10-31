#include "average.h"

#ifdef NEON_INTRINSICS

#include <arm_neon.h>

// mt_average NEON: (dst + src + 1) >> 1 (vrhadd), float (dst + src) * 0.5.

namespace Filtering { namespace MaskTools { namespace Filters { namespace Support { namespace Average {

namespace {

void average8_neon_t(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int nWidth, int nHeight)
{
  const int wmod = nWidth / 16 * 16;
  for (int y = 0; y < nHeight; y++) {
    for (int x = 0; x < wmod; x += 16)
      vst1q_u8(pDst + x, vrhaddq_u8(vld1q_u8(pDst + x), vld1q_u8(pSrc + x)));
    for (int x = wmod; x < nWidth; x++)
      pDst[x] = (int(pDst[x]) + pSrc[x] + 1) >> 1;
    pDst += nDstPitch;
    pSrc += nSrcPitch;
  }
}

void average16_neon_t(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int nWidth, int nHeight, int nOrigHeight)
{
  UNUSED(nOrigHeight);
  const int wmod = nWidth / 8 * 8;
  for (int y = 0; y < nHeight; y++) {
    uint16_t *d = reinterpret_cast<uint16_t *>(pDst);
    const uint16_t *s = reinterpret_cast<const uint16_t *>(pSrc);
    for (int x = 0; x < wmod; x += 8)
      vst1q_u16(d + x, vrhaddq_u16(vld1q_u16(d + x), vld1q_u16(s + x)));
    for (int x = wmod; x < nWidth; x++)
      d[x] = (d[x] + s[x] + 1) >> 1;
    pDst += nDstPitch;
    pSrc += nSrcPitch;
  }
}

void average32_neon_t(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int nWidth, int nHeight)
{
  const float32x4_t half = vdupq_n_f32(0.5f);
  const int wmod = nWidth / 4 * 4;
  for (int y = 0; y < nHeight; y++) {
    float *d = reinterpret_cast<float *>(pDst);
    const float *s = reinterpret_cast<const float *>(pSrc);
    for (int x = 0; x < wmod; x += 4)
      vst1q_f32(d + x, vmulq_f32(vaddq_f32(vld1q_f32(d + x), vld1q_f32(s + x)), half));
    for (int x = wmod; x < nWidth; x++)
      d[x] = (d[x] + s[x]) * 0.5f;
    pDst += nDstPitch;
    pSrc += nSrcPitch;
  }
}

} // namespace

Processor *average_neon = &average8_neon_t;
Processor16 *average16_native_neon = &average16_neon_t;
Processor32 *average32_neon = &average32_neon_t;

} } } } }

#endif // NEON_INTRINSICS
