#include "merge.h"

#ifdef NEON_INTRINSICS

#include <arm_neon.h>

// mt_merge NEON: 4:4:4 (or luma=false) and the luma=true mask downsampling 4:2:0, 4:2:2, 4:1:1 (mpeg1 placement)
// for 8, 10-16 bit; float 4:4:4, 4:2:0, 4:2:2. Bit-exact to the C versions (the rounding pair averages are
// vrhadd; float in the C order, unfused). mpeg2 / top_left placement: C.

namespace Filtering { namespace MaskTools { namespace Filters { namespace Merge {

namespace {

enum class Mode { M444, M420, M422, M411 };

/* 8 bit */

template<Mode mode>
MT_FORCEINLINE uint8x16_t mask8(const uint8_t *pMask, ptrdiff_t pitch, int x)
{
  if constexpr (mode == Mode::M444)
    return vld1q_u8(pMask + x);
  else if constexpr (mode == Mode::M422) {
    uint8x16x2_t r = vld2q_u8(pMask + x * 2);
    return vrhaddq_u8(r.val[0], r.val[1]);
  }
  else if constexpr (mode == Mode::M420) {
    uint8x16x2_t r1 = vld2q_u8(pMask + x * 2);
    uint8x16x2_t r2 = vld2q_u8(pMask + pitch + x * 2);
    return vrhaddq_u8(vrhaddq_u8(r1.val[0], r2.val[0]), vrhaddq_u8(r1.val[1], r2.val[1]));
  }
  else {
    uint8x16x4_t r = vld4q_u8(pMask + x * 4);
    return vrhaddq_u8(vrhaddq_u8(r.val[0], r.val[1]), vrhaddq_u8(r.val[2], r.val[3]));
  }
}

// ((256 - m) * d + m * s + 128) >> 8; m == 255: s (m == 0 gives d by the formula)
MT_FORCEINLINE uint8x16_t merge8_core(uint8x16_t d, uint8x16_t s, uint8x16_t m)
{
  const uint16x8_t v256 = vdupq_n_u16(256);
  const uint16x8_t v128 = vdupq_n_u16(128);
  uint16x8_t mlo = vmovl_u8(vget_low_u8(m)), mhi = vmovl_u8(vget_high_u8(m));
  uint16x8_t lo = vmlaq_u16(vmlaq_u16(v128, vsubq_u16(v256, mlo), vmovl_u8(vget_low_u8(d))), mlo, vmovl_u8(vget_low_u8(s)));
  uint16x8_t hi = vmlaq_u16(vmlaq_u16(v128, vsubq_u16(v256, mhi), vmovl_u8(vget_high_u8(d))), mhi, vmovl_u8(vget_high_u8(s)));
  uint8x16_t r = vcombine_u8(vshrn_n_u16(lo, 8), vshrn_n_u16(hi, 8));
  return vbslq_u8(vceqq_u8(m, vdupq_n_u8(255)), s, r);
}

template<Mode mode>
void merge8_neon_t(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch,
                   const uint8_t *pMask, ptrdiff_t nMaskPitch, int nWidth, int nHeight)
{
  constexpr int mstep = mode == Mode::M420 || mode == Mode::M422 ? 2 : mode == Mode::M411 ? 4 : 1;
  const int wmod = nWidth / 16 * 16;
  uint8_t *pDst0 = pDst;
  const uint8_t *pSrc0 = pSrc1, *pMask0 = pMask;
  for (int y = 0; y < nHeight; y++) {
    for (int x = 0; x < wmod; x += 16)
      vst1q_u8(pDst + x, merge8_core(vld1q_u8(pDst + x), vld1q_u8(pSrc1 + x), mask8<mode>(pMask, nMaskPitch, x)));
    pDst += nDstPitch;
    pSrc1 += nSrc1Pitch;
    pMask += mode == Mode::M420 ? nMaskPitch * 2 : nMaskPitch;
  }
  if (nWidth > wmod) {
    Processor *c = mode == Mode::M444 ? merge_c : mode == Mode::M420 ? merge_luma_420_c : mode == Mode::M422 ? merge_luma_422_c : merge_luma_411_c;
    c(pDst0 + wmod, nDstPitch, pSrc0 + wmod, nSrc1Pitch, pMask0 + wmod * mstep, nMaskPitch, nWidth - wmod, nHeight);
  }
}

/* 10-16 bit */

template<Mode mode>
MT_FORCEINLINE uint16x8_t mask16(const uint8_t *pMask, ptrdiff_t pitch, int x)
{
  const uint16_t *m = reinterpret_cast<const uint16_t *>(pMask);
  if constexpr (mode == Mode::M444)
    return vld1q_u16(m + x);
  else if constexpr (mode == Mode::M422) {
    uint16x8x2_t r = vld2q_u16(m + x * 2);
    return vrhaddq_u16(r.val[0], r.val[1]);
  }
  else if constexpr (mode == Mode::M420) {
    const uint16_t *m2 = reinterpret_cast<const uint16_t *>(pMask + pitch);
    uint16x8x2_t r1 = vld2q_u16(m + x * 2);
    uint16x8x2_t r2 = vld2q_u16(m2 + x * 2);
    return vrhaddq_u16(vrhaddq_u16(r1.val[0], r2.val[0]), vrhaddq_u16(r1.val[1], r2.val[1]));
  }
  else {
    uint16x8x4_t r = vld4q_u16(m + x * 4);
    return vrhaddq_u16(vrhaddq_u16(r.val[0], r.val[1]), vrhaddq_u16(r.val[2], r.val[3]));
  }
}

// d + (((s - d) * m) >> bpp), at 16 bit (m >> 1) and >> 15; arithmetic shift as in C; m == max: s
template<int bits_per_pixel>
MT_FORCEINLINE int32x4_t merge16_half(uint16x4_t d, uint16x4_t s, uint16x4_t m)
{
  int32x4_t d32 = vreinterpretq_s32_u32(vmovl_u16(d));
  int32x4_t diff = vsubq_s32(vreinterpretq_s32_u32(vmovl_u16(s)), d32);
  if constexpr (bits_per_pixel < 16)
    return vaddq_s32(d32, vshrq_n_s32(vmulq_s32(diff, vreinterpretq_s32_u32(vmovl_u16(m))), bits_per_pixel));
  else
    return vaddq_s32(d32, vshrq_n_s32(vmulq_s32(diff, vreinterpretq_s32_u32(vmovl_u16(vshr_n_u16(m, 1)))), 15));
}

template<int bits_per_pixel>
MT_FORCEINLINE uint16x8_t merge16_core(uint16x8_t d, uint16x8_t s, uint16x8_t m)
{
  int32x4_t lo = merge16_half<bits_per_pixel>(vget_low_u16(d), vget_low_u16(s), vget_low_u16(m));
  int32x4_t hi = merge16_half<bits_per_pixel>(vget_high_u16(d), vget_high_u16(s), vget_high_u16(m));
  uint16x8_t r = vcombine_u16(vmovn_u32(vreinterpretq_u32_s32(lo)), vmovn_u32(vreinterpretq_u32_s32(hi)));
  return vbslq_u16(vceqq_u16(m, vdupq_n_u16((uint16_t)((1 << bits_per_pixel) - 1))), s, r);
}

template<Mode mode, int bits_per_pixel>
void merge16_neon_t(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch,
                    const uint8_t *pMask, ptrdiff_t nMaskPitch, int nWidth, int nHeight, int nOrigHeight)
{
  constexpr int mstep = mode == Mode::M420 || mode == Mode::M422 ? 2 : mode == Mode::M411 ? 4 : 1;
  const int wmod = nWidth / 8 * 8;
  uint8_t *pDst0 = pDst;
  const uint8_t *pSrc0 = pSrc1, *pMask0 = pMask;
  for (int y = 0; y < nHeight; y++) {
    uint16_t *d16 = reinterpret_cast<uint16_t *>(pDst);
    const uint16_t *s16 = reinterpret_cast<const uint16_t *>(pSrc1);
    for (int x = 0; x < wmod; x += 8)
      vst1q_u16(d16 + x, merge16_core<bits_per_pixel>(vld1q_u16(d16 + x), vld1q_u16(s16 + x), mask16<mode>(pMask, nMaskPitch, x)));
    pDst += nDstPitch;
    pSrc1 += nSrc1Pitch;
    pMask += mode == Mode::M420 ? nMaskPitch * 2 : nMaskPitch;
  }
  if (nWidth > wmod) {
    Processor16 *c;
    if constexpr (mode == Mode::M444)
      c = bits_per_pixel == 10 ? merge16_10_c : bits_per_pixel == 12 ? merge16_12_c : bits_per_pixel == 14 ? merge16_14_c : merge16_16_c;
    else if constexpr (mode == Mode::M420)
      c = bits_per_pixel == 10 ? merge16_luma_420_10_c : bits_per_pixel == 12 ? merge16_luma_420_12_c : bits_per_pixel == 14 ? merge16_luma_420_14_c : merge16_luma_420_16_c;
    else if constexpr (mode == Mode::M422)
      c = bits_per_pixel == 10 ? merge16_luma_422_10_c : bits_per_pixel == 12 ? merge16_luma_422_12_c : bits_per_pixel == 14 ? merge16_luma_422_14_c : merge16_luma_422_16_c;
    else
      c = bits_per_pixel == 10 ? merge16_luma_411_10_c : bits_per_pixel == 12 ? merge16_luma_411_12_c : bits_per_pixel == 14 ? merge16_luma_411_14_c : merge16_luma_411_16_c;
    c(pDst0 + wmod * 2, nDstPitch, pSrc0 + wmod * 2, nSrc1Pitch, pMask0 + wmod * 2 * mstep, nMaskPitch, nWidth - wmod, nHeight, nOrigHeight);
  }
}

/* float */

template<Mode mode>
MT_FORCEINLINE float32x4_t mask32(const uint8_t *pMask, ptrdiff_t pitch, int x)
{
  const float *m = reinterpret_cast<const float *>(pMask);
  if constexpr (mode == Mode::M444)
    return vld1q_f32(m + x);
  else if constexpr (mode == Mode::M422) {
    float32x4x2_t r = vld2q_f32(m + x * 2);
    return vmulq_f32(vdupq_n_f32(0.5f), vaddq_f32(r.val[0], r.val[1]));
  }
  else { // 420: 0.25f * (((a + b) + c) + d), a, c: row 1, b, d: row 2, as in C
    const float *m2 = reinterpret_cast<const float *>(pMask + pitch);
    float32x4x2_t r1 = vld2q_f32(m + x * 2);
    float32x4x2_t r2 = vld2q_f32(m2 + x * 2);
    float32x4_t sum = vaddq_f32(vaddq_f32(vaddq_f32(r1.val[0], r2.val[0]), r1.val[1]), r2.val[1]);
    return vmulq_f32(vdupq_n_f32(0.25f), sum);
  }
}

// (1 - m) * d + m * s, unfused (C order)
template<Mode mode>
void merge32_neon_t(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch,
                    const uint8_t *pMask, ptrdiff_t nMaskPitch, int nWidth, int nHeight)
{
  constexpr int mstep = mode == Mode::M444 ? 1 : 2;
  const int wmod = nWidth / 4 * 4;
  const float32x4_t one = vdupq_n_f32(1.0f);
  uint8_t *pDst0 = pDst;
  const uint8_t *pSrc0 = pSrc1, *pMask0 = pMask;
  for (int y = 0; y < nHeight; y++) {
    float *d = reinterpret_cast<float *>(pDst);
    const float *s = reinterpret_cast<const float *>(pSrc1);
    for (int x = 0; x < wmod; x += 4) {
      float32x4_t m = mask32<mode>(pMask, nMaskPitch, x);
      vst1q_f32(d + x, vaddq_f32(vmulq_f32(vsubq_f32(one, m), vld1q_f32(d + x)), vmulq_f32(m, vld1q_f32(s + x))));
    }
    pDst += nDstPitch;
    pSrc1 += nSrc1Pitch;
    pMask += mode == Mode::M420 ? nMaskPitch * 2 : nMaskPitch;
  }
  if (nWidth > wmod) {
    Processor32 *c = mode == Mode::M444 ? merge32_c : mode == Mode::M420 ? merge32_luma_420_c : merge32_luma_422_c;
    c(pDst0 + wmod * 4, nDstPitch, pSrc0 + wmod * 4, nSrc1Pitch, pMask0 + wmod * 4 * mstep, nMaskPitch, nWidth - wmod, nHeight);
  }
}

} // namespace

Processor *merge_neon = &merge8_neon_t<Mode::M444>;
Processor *merge_luma_420_neon = &merge8_neon_t<Mode::M420>;
Processor *merge_luma_422_neon = &merge8_neon_t<Mode::M422>;
Processor *merge_luma_411_neon = &merge8_neon_t<Mode::M411>;

#define MAKE_NEON16(bits_per_pixel) \
Processor16 *merge16_##bits_per_pixel##_neon = &merge16_neon_t<Mode::M444, bits_per_pixel>; \
Processor16 *merge16_luma_420_##bits_per_pixel##_neon = &merge16_neon_t<Mode::M420, bits_per_pixel>; \
Processor16 *merge16_luma_422_##bits_per_pixel##_neon = &merge16_neon_t<Mode::M422, bits_per_pixel>; \
Processor16 *merge16_luma_411_##bits_per_pixel##_neon = &merge16_neon_t<Mode::M411, bits_per_pixel>;

MAKE_NEON16(10)
MAKE_NEON16(12)
MAKE_NEON16(14)
MAKE_NEON16(16)
#undef MAKE_NEON16

Processor32 *merge32_neon = &merge32_neon_t<Mode::M444>;
Processor32 *merge32_luma_420_neon = &merge32_neon_t<Mode::M420>;
Processor32 *merge32_luma_422_neon = &merge32_neon_t<Mode::M422>;

} } } }

#endif // NEON_INTRINSICS
