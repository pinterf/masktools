#include "clamp.h"

#ifdef NEON_INTRINSICS

#include <arm_neon.h>

// mt_clamp NEON: dst = max(min(dst, up + overshoot), low - undershoot), 8, 10-16 bit (saturating, the
// thresholds are 0..max), float (the C compares and selects).

namespace Filtering { namespace MaskTools { namespace Filters { namespace Support { namespace Clamp {

namespace {

void clamp8_neon_t(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch, const uint8_t *pSrc2, ptrdiff_t nSrc2Pitch, int nWidth, int nHeight, int nOvershoot, int nUndershoot)
{
  const uint8x16_t over = vdupq_n_u8(uint8_t(nOvershoot)), under = vdupq_n_u8(uint8_t(nUndershoot));
  const int wmod = nWidth / 16 * 16;
  uint8_t *pDst0 = pDst;
  const uint8_t *pSrc10 = pSrc1, *pSrc20 = pSrc2;
  for (int y = 0; y < nHeight; y++) {
    for (int x = 0; x < wmod; x += 16) {
      uint8x16_t r = vminq_u8(vld1q_u8(pDst + x), vqaddq_u8(vld1q_u8(pSrc1 + x), over));
      vst1q_u8(pDst + x, vmaxq_u8(r, vqsubq_u8(vld1q_u8(pSrc2 + x), under)));
    }
    pDst += nDstPitch;
    pSrc1 += nSrc1Pitch;
    pSrc2 += nSrc2Pitch;
  }
  if (nWidth > wmod)
    clamp_c(pDst0 + wmod, nDstPitch, pSrc10 + wmod, nSrc1Pitch, pSrc20 + wmod, nSrc2Pitch, nWidth - wmod, nHeight, nOvershoot, nUndershoot);
}

template<int bits_per_pixel>
void clamp16_neon_t(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pUpLimit, ptrdiff_t nUpLimitPitch, const uint8_t *pLowLimit, ptrdiff_t nLowLimitPitch, int nWidth, int nHeight, int nOrigHeight, int nOvershoot, int nUndershoot)
{
  const uint16x8_t over = vdupq_n_u16(uint16_t(nOvershoot)), under = vdupq_n_u16(uint16_t(nUndershoot));
  const uint16x8_t mx = vdupq_n_u16(uint16_t((1 << bits_per_pixel) - 1));
  const int wmod = nWidth / 8 * 8;
  uint8_t *pDst0 = pDst;
  const uint8_t *pUp0 = pUpLimit, *pLow0 = pLowLimit;
  for (int y = 0; y < nHeight; y++) {
    uint16_t *d = reinterpret_cast<uint16_t *>(pDst);
    const uint16_t *up = reinterpret_cast<const uint16_t *>(pUpLimit);
    const uint16_t *low = reinterpret_cast<const uint16_t *>(pLowLimit);
    for (int x = 0; x < wmod; x += 8) {
      uint16x8_t r = vminq_u16(vld1q_u16(d + x), vqaddq_u16(vld1q_u16(up + x), over));
      r = vmaxq_u16(r, vqsubq_u16(vld1q_u16(low + x), under));
      if constexpr (bits_per_pixel < 16)
        r = vminq_u16(r, mx);
      vst1q_u16(d + x, r);
    }
    pDst += nDstPitch;
    pUpLimit += nUpLimitPitch;
    pLowLimit += nLowLimitPitch;
  }
  if (nWidth > wmod) {
    Processor16 *c = bits_per_pixel == 10 ? clamp16_native_10_c : bits_per_pixel == 12 ? clamp16_native_12_c :
                     bits_per_pixel == 14 ? clamp16_native_14_c : clamp16_native_16_c;
    c(pDst0 + wmod * 2, nDstPitch, pUp0 + wmod * 2, nUpLimitPitch, pLow0 + wmod * 2, nLowLimitPitch, nWidth - wmod, nHeight, nOrigHeight, nOvershoot, nUndershoot);
  }
}

// dst > up + over ? up + over : dst, then r < low - under ? low - under : r (the C)
void clamp32_neon_t(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pUpLimit, ptrdiff_t nUpLimitPitch, const uint8_t *pLowLimit, ptrdiff_t nLowLimitPitch, int nWidth, int nHeight, float nOvershoot, float nUndershoot)
{
  const float32x4_t over = vdupq_n_f32(nOvershoot), under = vdupq_n_f32(nUndershoot);
  const int wmod = nWidth / 4 * 4;
  uint8_t *pDst0 = pDst;
  const uint8_t *pUp0 = pUpLimit, *pLow0 = pLowLimit;
  for (int y = 0; y < nHeight; y++) {
    float *d = reinterpret_cast<float *>(pDst);
    const float *up = reinterpret_cast<const float *>(pUpLimit);
    const float *low = reinterpret_cast<const float *>(pLowLimit);
    for (int x = 0; x < wmod; x += 4) {
      float32x4_t v = vld1q_f32(d + x);
      float32x4_t upo = vaddq_f32(vld1q_f32(up + x), over);
      v = vbslq_f32(vcgtq_f32(v, upo), upo, v);
      float32x4_t lowu = vsubq_f32(vld1q_f32(low + x), under);
      vst1q_f32(d + x, vbslq_f32(vcltq_f32(v, lowu), lowu, v));
    }
    pDst += nDstPitch;
    pUpLimit += nUpLimitPitch;
    pLowLimit += nLowLimitPitch;
  }
  if (nWidth > wmod)
    clamp32_c(pDst0 + wmod * 4, nDstPitch, pUp0 + wmod * 4, nUpLimitPitch, pLow0 + wmod * 4, nLowLimitPitch, nWidth - wmod, nHeight, nOvershoot, nUndershoot);
}

} // namespace

Processor *clamp_neon = &clamp8_neon_t;
Processor16 *clamp16_native_10_neon = &clamp16_neon_t<10>;
Processor16 *clamp16_native_12_neon = &clamp16_neon_t<12>;
Processor16 *clamp16_native_14_neon = &clamp16_neon_t<14>;
Processor16 *clamp16_native_16_neon = &clamp16_neon_t<16>;
Processor32 *clamp32_neon = &clamp32_neon_t;

} } } } }

#endif // NEON_INTRINSICS
