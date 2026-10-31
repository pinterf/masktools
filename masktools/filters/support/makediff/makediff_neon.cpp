#include "makediff.h"

#ifdef NEON_INTRINSICS

#include <arm_neon.h>

namespace Filtering { namespace MaskTools { namespace Filters { namespace Support { namespace MakeDiff {

namespace {

// dst = clip(dst - src + 128): (dst - 128) - (src - 128) with signed saturation, + 128
void makediff_neon_t(uint8_t *pDst, ptrdiff_t dst_pitch, const uint8_t *pSrc, ptrdiff_t src_pitch, int width, int height)
{
  const int wmod = width / 16 * 16;
  const uint8x16_t v128 = vdupq_n_u8(0x80);
  uint8_t *pDst0 = pDst;
  const uint8_t *pSrc0 = pSrc;
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < wmod; x += 16) {
      int8x16_t d = vreinterpretq_s8_u8(veorq_u8(vld1q_u8(pDst + x), v128));
      int8x16_t s = vreinterpretq_s8_u8(veorq_u8(vld1q_u8(pSrc + x), v128));
      vst1q_u8(pDst + x, veorq_u8(vreinterpretq_u8_s8(vqsubq_s8(d, s)), v128));
    }
    pDst += dst_pitch;
    pSrc += src_pitch;
  }
  if (width > wmod)
    makediff_c(pDst0 + wmod, dst_pitch, pSrc0 + wmod, src_pitch, width - wmod, height);
}

template<int bits_per_pixel>
void makediff16_neon_t(uint8_t *pDst, ptrdiff_t dst_pitch, const uint8_t *pSrc, ptrdiff_t src_pitch, int width, int height, int origheight)
{
  const int wmod = width / 8 * 8;
  const uint16x8_t half = vdupq_n_u16((uint16_t)(1 << (bits_per_pixel - 1)));
  const uint16x8_t maxv = vdupq_n_u16((uint16_t)((1 << bits_per_pixel) - 1));
  uint8_t *pDst0 = pDst;
  const uint8_t *pSrc0 = pSrc;
  for (int y = 0; y < height; y++) {
    uint16_t *d16 = reinterpret_cast<uint16_t *>(pDst);
    const uint16_t *s16 = reinterpret_cast<const uint16_t *>(pSrc);
    for (int x = 0; x < wmod; x += 8) {
      uint16x8_t d = vld1q_u16(d16 + x);
      uint16x8_t s = vld1q_u16(s16 + x);
      uint16x8_t r;
      if constexpr (bits_per_pixel < 16) {
        // dst + half fits in 16 bits; the saturating subtraction clips at 0, min at max
        r = vminq_u16(vqsubq_u16(vaddq_u16(d, half), s), maxv);
      }
      else {
        int16x8_t ds = vreinterpretq_s16_u16(veorq_u16(d, half));
        int16x8_t ss = vreinterpretq_s16_u16(veorq_u16(s, half));
        r = veorq_u16(vreinterpretq_u16_s16(vqsubq_s16(ds, ss)), half);
      }
      vst1q_u16(d16 + x, r);
    }
    pDst += dst_pitch;
    pSrc += src_pitch;
  }
  if (width > wmod) {
    Processor16 *c = bits_per_pixel == 10 ? makediff16_native_10_c : bits_per_pixel == 12 ? makediff16_native_12_c :
                     bits_per_pixel == 14 ? makediff16_native_14_c : makediff16_native_16_c;
    c(pDst0 + wmod * 2, dst_pitch, pSrc0 + wmod * 2, src_pitch, width - wmod, height, origheight);
  }
}

// dst = (dst - src) + middle, the C order (no fused multiply-add involved)
void makediff32_neon_t(uint8_t *pDst, ptrdiff_t dst_pitch, const uint8_t *pSrc, ptrdiff_t src_pitch, int width, int height, bool chroma)
{
#ifdef FLOAT_CHROMA_IS_HALF_CENTERED
  UNUSED(chroma);
  const float middle_value = 0.5f;
#else
  const float middle_value = chroma ? 0.0f : 0.5f;
#endif
  const int wmod = width / 4 * 4;
  const float32x4_t middle = vdupq_n_f32(middle_value);
  uint8_t *pDst0 = pDst;
  const uint8_t *pSrc0 = pSrc;
  for (int y = 0; y < height; y++) {
    float *d = reinterpret_cast<float *>(pDst);
    const float *s = reinterpret_cast<const float *>(pSrc);
    for (int x = 0; x < wmod; x += 4)
      vst1q_f32(d + x, vaddq_f32(vsubq_f32(vld1q_f32(d + x), vld1q_f32(s + x)), middle));
    pDst += dst_pitch;
    pSrc += src_pitch;
  }
  if (width > wmod)
    makediff32_c(pDst0 + wmod * 4, dst_pitch, pSrc0 + wmod * 4, src_pitch, width - wmod, height, chroma);
}

} // namespace

Processor *makediff_neon = &makediff_neon_t;
Processor16 *makediff16_native_10_neon = &makediff16_neon_t<10>;
Processor16 *makediff16_native_12_neon = &makediff16_neon_t<12>;
Processor16 *makediff16_native_14_neon = &makediff16_neon_t<14>;
Processor16 *makediff16_native_16_neon = &makediff16_neon_t<16>;
Processor32 *makediff32_neon = &makediff32_neon_t;

} } } } }

#endif // NEON_INTRINSICS
