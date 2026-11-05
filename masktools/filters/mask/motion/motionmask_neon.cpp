#include "motionmask.h"

#ifdef NEON_INTRINSICS

#include <arm_neon.h>
#include "motionmask_c.h"
#include "motionmask16_c.h"
#include "motionmask32_c.h"

// mt_motion NEON: the sum of absolute differences (scene change detection) and the thresholded absolute
// difference, 8, 10-16 bit; float: the mask in NEON, the SAD in C (its float summation order decides the scene
// change).

namespace Filtering { namespace MaskTools { namespace Filters { namespace Mask { namespace Motion {

namespace {

// Two accumulators, 32 bytes per step (as calculate_sad_8_or_16_neon in AviSynth+). A 16 bit lane takes at most
// 2 * 255 per step, so 64 steps cannot overflow it before the flush to 32 bit.
uint64_t sad8_neon_op(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int nWidth, int nHeight)
{
  const int mod32 = nWidth / 32 * 32;
  const int mod16 = nWidth / 16 * 16;
  uint64_t nSad = 0;
  for (int y = 0; y < nHeight; y++) {
    uint32x4_t acc32a = vdupq_n_u32(0);
    uint32x4_t acc32b = vdupq_n_u32(0);
    int x = 0;
    while (x < mod32) {
      uint16x8_t acc16a = vdupq_n_u16(0);
      uint16x8_t acc16b = vdupq_n_u16(0);
      const int block_end = mod32 - x > 64 * 32 ? x + 64 * 32 : mod32;
      for (; x < block_end; x += 32) {
        acc16a = vpadalq_u8(acc16a, vabdq_u8(vld1q_u8(pDst + x), vld1q_u8(pSrc + x)));
        acc16b = vpadalq_u8(acc16b, vabdq_u8(vld1q_u8(pDst + x + 16), vld1q_u8(pSrc + x + 16)));
      }
      acc32a = vpadalq_u16(acc32a, acc16a);
      acc32b = vpadalq_u16(acc32b, acc16b);
    }
    if (x < mod16) {
      acc32a = vpadalq_u16(acc32a, vpaddlq_u8(vabdq_u8(vld1q_u8(pDst + x), vld1q_u8(pSrc + x))));
      x += 16;
    }
    uint64_t rowsum = (uint64_t)vaddlvq_u32(acc32a) + vaddlvq_u32(acc32b);
    for (; x < nWidth; x++)
      rowsum += abs(pDst[x] - pSrc[x]);
    nSad += rowsum;
    pDst += nDstPitch;
    pSrc += nSrcPitch;
  }
  return nSad;
}

// x <= lo ? 0 : x > hi ? 255 : x; the thresholds are 0..255
void mask8_neon_op(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int nLowThreshold, int nHighThreshold, int nWidth, int nHeight)
{
  const uint8x16_t lo = vdupq_n_u8(uint8_t(nLowThreshold)), hi = vdupq_n_u8(uint8_t(nHighThreshold));
  const int wmod = nWidth / 16 * 16;
  for (int y = 0; y < nHeight; y++) {
    for (int x = 0; x < wmod; x += 16) {
      uint8x16_t a = vabdq_u8(vld1q_u8(pDst + x), vld1q_u8(pSrc + x));
      uint8x16_t r = vorrq_u8(a, vcgtq_u8(a, hi)); // > hi: 255
      vst1q_u8(pDst + x, vbicq_u8(r, vcleq_u8(a, lo)));
    }
    for (int x = wmod; x < nWidth; x++)
      pDst[x] = threshold<uint8_t, int>(abs<int>(pDst[x] - pSrc[x]), nLowThreshold, nHighThreshold);
    pDst += nDstPitch;
    pSrc += nSrcPitch;
  }
}

// Two accumulators, 16 pixels per step (as calculate_sad_8_or_16_neon in AviSynth+). Directly in 32 bit lanes.
uint64_t sad16_neon_op(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int nWidth, int nHeight)
{
  const int mod16 = nWidth / 16 * 16;
  const int mod8 = nWidth / 8 * 8;
  uint64_t nSad = 0;
  for (int y = 0; y < nHeight; y++) {
    const uint16_t *d = reinterpret_cast<const uint16_t *>(pDst);
    const uint16_t *s = reinterpret_cast<const uint16_t *>(pSrc);
    uint32x4_t acc_lo = vdupq_n_u32(0);
    uint32x4_t acc_hi = vdupq_n_u32(0);
    int x = 0;
    for (; x < mod16; x += 16) {
      const uint16x8_t a0 = vld1q_u16(d + x), b0 = vld1q_u16(s + x);
      const uint16x8_t a1 = vld1q_u16(d + x + 8), b1 = vld1q_u16(s + x + 8);
      acc_lo = vabal_u16(acc_lo, vget_low_u16(a0), vget_low_u16(b0));
      acc_hi = vabal_high_u16(acc_hi, a0, b0);
      acc_lo = vabal_u16(acc_lo, vget_low_u16(a1), vget_low_u16(b1));
      acc_hi = vabal_high_u16(acc_hi, a1, b1);
    }
    if (x < mod8) {
      const uint16x8_t a0 = vld1q_u16(d + x), b0 = vld1q_u16(s + x);
      acc_lo = vabal_u16(acc_lo, vget_low_u16(a0), vget_low_u16(b0));
      acc_hi = vabal_high_u16(acc_hi, a0, b0);
      x += 8;
    }
    uint64_t rowsum = (uint64_t)vaddlvq_u32(acc_lo) + vaddlvq_u32(acc_hi);
    for (; x < nWidth; x++)
      rowsum += abs(d[x] - s[x]);
    nSad += rowsum;
    pDst += nDstPitch;
    pSrc += nSrcPitch;
  }
  return nSad;
}

void mask16_neon_op(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int nLowThreshold, int nHighThreshold, int nWidth, int nHeight, int bits_per_pixel)
{
  const int pixelMax = (1 << bits_per_pixel) - 1;
  const uint16x8_t lo = vdupq_n_u16(uint16_t(nLowThreshold)), hi = vdupq_n_u16(uint16_t(nHighThreshold));
  const uint16x8_t mx = vdupq_n_u16(uint16_t(pixelMax));
  const int wmod = nWidth / 8 * 8;
  for (int y = 0; y < nHeight; y++) {
    uint16_t *d = reinterpret_cast<uint16_t *>(pDst);
    const uint16_t *s = reinterpret_cast<const uint16_t *>(pSrc);
    for (int x = 0; x < wmod; x += 8) {
      uint16x8_t a = vabdq_u16(vld1q_u16(d + x), vld1q_u16(s + x));
      uint16x8_t r = vbslq_u16(vcgtq_u16(a, hi), mx, a);
      vst1q_u16(d + x, vbicq_u16(r, vcleq_u16(a, lo)));
    }
    for (int x = wmod; x < nWidth; x++)
      d[x] = threshold16(abs<int>(d[x] - s[x]), nLowThreshold, nHighThreshold, pixelMax);
    pDst += nDstPitch;
    pSrc += nSrcPitch;
  }
}

// threshold<float>(abs(d - s)): x <= lo ? 0 : x > hi ? 1 : x, abs as the C x < 0 ? -x : x
void mask32_neon_op(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, float nLowThreshold, float nHighThreshold, int nWidth, int nHeight)
{
  const float32x4_t lo = vdupq_n_f32(nLowThreshold), hi = vdupq_n_f32(nHighThreshold);
  const float32x4_t one = vdupq_n_f32(1.0f), zero = vdupq_n_f32(0.0f);
  const int wmod = nWidth / 4 * 4;
  for (int y = 0; y < nHeight; y++) {
    float *d = reinterpret_cast<float *>(pDst);
    const float *s = reinterpret_cast<const float *>(pSrc);
    for (int x = 0; x < wmod; x += 4) {
      float32x4_t a = vsubq_f32(vld1q_f32(d + x), vld1q_f32(s + x));
      a = vbslq_f32(vcltq_f32(a, zero), vnegq_f32(a), a);
      float32x4_t r = vbslq_f32(vcgtq_f32(a, hi), one, a);
      vst1q_f32(d + x, vreinterpretq_f32_u32(vbicq_u32(vreinterpretq_u32_f32(r), vcleq_f32(a, lo))));
    }
    for (int x = wmod; x < nWidth; x++)
      d[x] = threshold<float, float>(abs<float>(d[x] - s[x]), nLowThreshold, nHighThreshold);
    pDst += nDstPitch;
    pSrc += nSrcPitch;
  }
}

} // namespace

Processor *mask_neon = &mask_t<sad8_neon_op, mask8_neon_op>;
Processor16 *mask16_neon = &mask16_t<sad16_neon_op, mask16_neon_op>;
Processor32 *mask32_neon = &mask32_t<sad32_c_op, mask32_neon_op>;

} } } } }

#endif // NEON_INTRINSICS
