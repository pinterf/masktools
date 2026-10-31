#include "invert.h"

#ifdef NEON_INTRINSICS

#include <arm_neon.h>

// mt_invert NEON: max - x (8 bit: ~x), 1.0 - x for float.

namespace Filtering { namespace MaskTools { namespace Filters { namespace Invert {

namespace {

void invert8_neon_t(uint8_t *pDst, ptrdiff_t nDstPitch, int nWidth, int nHeight)
{
  const int wmod = nWidth / 16 * 16;
  for (int y = 0; y < nHeight; y++) {
    for (int x = 0; x < wmod; x += 16)
      vst1q_u8(pDst + x, vmvnq_u8(vld1q_u8(pDst + x)));
    for (int x = wmod; x < nWidth; x++)
      pDst[x] = 255 - pDst[x];
    pDst += nDstPitch;
  }
}

template<int bits_per_pixel>
void invert16_neon_t(uint8_t *pDst, ptrdiff_t nDstPitch, int nWidth, int nHeight)
{
  const uint16_t mx = (1 << bits_per_pixel) - 1;
  const uint16x8_t vmx = vdupq_n_u16(mx);
  const int wmod = nWidth / 8 * 8;
  for (int y = 0; y < nHeight; y++) {
    uint16_t *d = reinterpret_cast<uint16_t *>(pDst);
    for (int x = 0; x < wmod; x += 8)
      vst1q_u16(d + x, vsubq_u16(vmx, vld1q_u16(d + x)));
    for (int x = wmod; x < nWidth; x++)
      d[x] = mx - d[x];
    pDst += nDstPitch;
  }
}

void invert32_neon_t(uint8_t *pDst, ptrdiff_t nDstPitch, int nWidth, int nHeight)
{
  const float32x4_t one = vdupq_n_f32(1.0f);
  const int wmod = nWidth / 4 * 4;
  for (int y = 0; y < nHeight; y++) {
    float *d = reinterpret_cast<float *>(pDst);
    for (int x = 0; x < wmod; x += 4)
      vst1q_f32(d + x, vsubq_f32(one, vld1q_f32(d + x)));
    for (int x = wmod; x < nWidth; x++)
      d[x] = 1.0f - d[x];
    pDst += nDstPitch;
  }
}

} // namespace

Processor *invert_neon = &invert8_neon_t;
Processor *invert10_neon = &invert16_neon_t<10>;
Processor *invert12_neon = &invert16_neon_t<12>;
Processor *invert14_neon = &invert16_neon_t<14>;
Processor *invert16_neon = &invert16_neon_t<16>;
Processor *invert32_neon = &invert32_neon_t;

} } } }

#endif // NEON_INTRINSICS
