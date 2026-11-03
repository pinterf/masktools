#include "merge.h"
#include "merge16_c.h"

#ifdef NEON_INTRINSICS

#include "merge_c.h"
#include <arm_neon.h>
#include "merge_410_simd.h"

// mt_merge NEON: 4:4:4 (or luma=false) and the luma=true mask downsampling 4:2:0, 4:2:2, 4:1:1 (mpeg1 placement)
// for 8, 10-16 bit; float 4:4:4, 4:2:0, 4:2:2. Also 4:4:0 and 4:1:0 at all depths/placements.
// Bit-exact to C (rounded pair averages: vrhadd; float in C order, unfused).
// mpeg2 / top_left placement for 420/422: C.

namespace Filtering { namespace MaskTools { namespace Filters { namespace Merge {

namespace {

enum class Mode { M444, M420, M422, M411, M440, TopLeft440 };

/* 8 bit */

template<Mode mode>
MT_FORCEINLINE uint8x16_t mask8(const uint8_t *pMask, ptrdiff_t pitch, int x, ptrdiff_t prev_pitch)
{
  if constexpr (mode == Mode::M444)
    return vld1q_u8(pMask + x);
  else if constexpr (mode == Mode::M440 || mode == Mode::TopLeft440) {
    auto mid = vld1q_u8(pMask + x);
    auto below = vld1q_u8(pMask + pitch + x);
    if constexpr (mode == Mode::M440)
      return vrhaddq_u8(mid, below);
    else {
      auto above = vld1q_u8(pMask + prev_pitch + x);
      auto lo = vaddq_u16(vaddl_u8(vget_low_u8(above), vget_low_u8(below)), vshll_n_u8(vget_low_u8(mid), 1));
      auto hi = vaddq_u16(vaddl_u8(vget_high_u8(above), vget_high_u8(below)), vshll_n_u8(vget_high_u8(mid), 1));
      return vcombine_u8(vrshrn_n_u16(lo, 2), vrshrn_n_u16(hi, 2));
    }
  }
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

MT_FORCEINLINE uint8x16_t merge8_core(uint8x16_t d, uint8x16_t s, uint8x16_t m)
{
  const uint16x8_t v255 = vdupq_n_u16(255);
  const uint16x8_t v127 = vdupq_n_u16(127);
  uint16x8_t mlo = vmovl_u8(vget_low_u8(m)), mhi = vmovl_u8(vget_high_u8(m));
  uint16x8_t lo = vaddq_u16(vaddq_u16(vmulq_u16(vmovl_u8(vget_low_u8(d)), vsubq_u16(v255, mlo)), vmulq_u16(vmovl_u8(vget_low_u8(s)), mlo)), v127);
  uint16x8_t hi = vaddq_u16(vaddq_u16(vmulq_u16(vmovl_u8(vget_high_u8(d)), vsubq_u16(v255, mhi)), vmulq_u16(vmovl_u8(vget_high_u8(s)), mhi)), v127);
  // 0x8081 = ceil(2^23 / 255); the right shift yields exact division for this numerator range.
  uint32x4_t lo_product = vmull_n_u16(vget_low_u16(lo), 0x8081);
  uint32x4_t hi_product = vmull_n_u16(vget_high_u16(lo), 0x8081);
  uint32x4_t lo_product_hi = vmull_n_u16(vget_low_u16(hi), 0x8081);
  uint32x4_t hi_product_hi = vmull_n_u16(vget_high_u16(hi), 0x8081);
  int32x4_t shift = vdupq_n_s32(-23);
  uint16x8_t result_lo = vcombine_u16(vmovn_u32(vshlq_u32(lo_product, shift)), vmovn_u32(vshlq_u32(hi_product, shift)));
  uint16x8_t result_hi = vcombine_u16(vmovn_u32(vshlq_u32(lo_product_hi, shift)), vmovn_u32(vshlq_u32(hi_product_hi, shift)));
  return vcombine_u8(vmovn_u16(result_lo), vmovn_u16(result_hi));
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
      vst1q_u8(pDst + x, merge8_core(vld1q_u8(pDst + x), vld1q_u8(pSrc1 + x), mask8<mode>(pMask, nMaskPitch, x, y == 0 ? 0 : -nMaskPitch)));
    pDst += nDstPitch;
    pSrc1 += nSrc1Pitch;
    pMask += (mode == Mode::M420 || mode == Mode::M440 || mode == Mode::TopLeft440) ? nMaskPitch * 2 : nMaskPitch;
  }
  if (nWidth > wmod) {
    Processor *c = mode == Mode::M444 ? merge_t_c : mode == Mode::M420 ? merge_luma_420_t_c :
      mode == Mode::M440 ? merge_luma_440_t_c<false> : mode == Mode::TopLeft440 ? merge_luma_440_t_c<true> :
      mode == Mode::M422 ? merge_luma_422_t_c : merge_luma_411_t_c;
    c(pDst0 + wmod, nDstPitch, pSrc0 + wmod, nSrc1Pitch, pMask0 + wmod * mstep, nMaskPitch, nWidth - wmod, nHeight);
  }
}

/* 10-16 bit */

template<Mode mode>
MT_FORCEINLINE uint16x8_t mask16(const uint8_t *pMask, ptrdiff_t pitch, int x, ptrdiff_t prev_pitch)
{
  const uint16_t *m = reinterpret_cast<const uint16_t *>(pMask);
  if constexpr (mode == Mode::M444)
    return vld1q_u16(m + x);
  else if constexpr (mode == Mode::M440 || mode == Mode::TopLeft440) {
    auto mid = vld1q_u16(m + x);
    auto below = vld1q_u16(reinterpret_cast<const uint16_t*>(pMask + pitch) + x);
    if constexpr (mode == Mode::M440)
      return vrhaddq_u16(mid, below);
    else {
      auto above = vld1q_u16(reinterpret_cast<const uint16_t*>(pMask + prev_pitch) + x);
      auto lo = vaddq_u32(vaddl_u16(vget_low_u16(above), vget_low_u16(below)), vshll_n_u16(vget_low_u16(mid), 1));
      auto hi = vaddq_u32(vaddl_u16(vget_high_u16(above), vget_high_u16(below)), vshll_n_u16(vget_high_u16(mid), 1));
      return vcombine_u16(vrshrn_n_u32(lo, 2), vrshrn_n_u32(hi, 2));
    }
  }
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

MT_FORCEINLINE uint32x4_t merge16_half(uint16x4_t d, uint16x4_t s, uint16x4_t m, uint32_t max, MergeMagicDiv magic)
{
  uint32x4_t d32 = vmovl_u16(d);
  uint32x4_t s32 = vmovl_u16(s);
  uint32x4_t m32 = vmovl_u16(m);
  uint32x4_t inverse = vsubq_u32(vdupq_n_u32(max), m32);
  uint32x4_t numerator = vaddq_u32(vaddq_u32(vmulq_u32(d32, inverse), vmulq_u32(s32, m32)), vdupq_n_u32(max / 2));
  // The unsigned reciprocal product, shifted by 32 + magic.shift, gives floor(numerator / max).
  int64x2_t shift = vdupq_n_s64(-(int64_t)(32 + magic.shift));
  uint64x2_t product_lo = vmull_n_u32(vget_low_u32(numerator), magic.div);
  uint64x2_t product_hi = vmull_n_u32(vget_high_u32(numerator), magic.div);
  uint32x2_t quotient_lo = vmovn_u64(vshlq_u64(product_lo, shift));
  uint32x2_t quotient_hi = vmovn_u64(vshlq_u64(product_hi, shift));
  return vcombine_u32(quotient_lo, quotient_hi);
}

MT_FORCEINLINE uint16x8_t merge16_core(uint16x8_t d, uint16x8_t s, uint16x8_t m, uint32_t max, MergeMagicDiv magic)
{
  uint32x4_t lo = merge16_half(vget_low_u16(d), vget_low_u16(s), vget_low_u16(m), max, magic);
  uint32x4_t hi = merge16_half(vget_high_u16(d), vget_high_u16(s), vget_high_u16(m), max, magic);
  return vcombine_u16(vmovn_u32(lo), vmovn_u32(hi));
}

template<Mode mode>
void merge16_neon_t(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch,
                    const uint8_t *pMask, ptrdiff_t nMaskPitch, int nWidth, int nHeight, int bits_per_pixel)
{
  constexpr int mstep = mode == Mode::M420 || mode == Mode::M422 ? 2 : mode == Mode::M411 ? 4 : 1;
  const uint32_t max = (1u << bits_per_pixel) - 1;
  const MergeMagicDiv magic = get_merge_magic_div(bits_per_pixel);
  const int wmod = nWidth / 8 * 8;
  uint8_t *pDst0 = pDst;
  const uint8_t *pSrc0 = pSrc1, *pMask0 = pMask;
  for (int y = 0; y < nHeight; y++) {
    uint16_t *d16 = reinterpret_cast<uint16_t *>(pDst);
    const uint16_t *s16 = reinterpret_cast<const uint16_t *>(pSrc1);
    for (int x = 0; x < wmod; x += 8)
      vst1q_u16(d16 + x, merge16_core(vld1q_u16(d16 + x), vld1q_u16(s16 + x), mask16<mode>(pMask, nMaskPitch, x, y == 0 ? 0 : -nMaskPitch), max, magic));
    pDst += nDstPitch;
    pSrc1 += nSrc1Pitch;
    pMask += (mode == Mode::M420 || mode == Mode::M440 || mode == Mode::TopLeft440) ? nMaskPitch * 2 : nMaskPitch;
  }
  if (nWidth > wmod) {
    Processor16 *c = mode == Mode::M444 ? merge16_c : mode == Mode::M420 ? merge16_luma_420_c :
      mode == Mode::M440 ? merge16_luma_440_c : mode == Mode::TopLeft440 ? merge16_luma_440_topleft_c :
      mode == Mode::M422 ? merge16_luma_422_c : merge16_luma_411_c;
    c(pDst0 + wmod * 2, nDstPitch, pSrc0 + wmod * 2, nSrc1Pitch, pMask0 + wmod * 2 * mstep, nMaskPitch, nWidth - wmod, nHeight, bits_per_pixel);
  }
}

/* float */

template<Mode mode>
MT_FORCEINLINE float32x4_t mask32(const uint8_t *pMask, ptrdiff_t pitch, int x, ptrdiff_t prev_pitch)
{
  const float *m = reinterpret_cast<const float *>(pMask);
  if constexpr (mode == Mode::M444)
    return vld1q_f32(m + x);
  else if constexpr (mode == Mode::M440 || mode == Mode::TopLeft440) {
    auto mid = vld1q_f32(m + x);
    auto below = vld1q_f32(reinterpret_cast<const float*>(pMask + pitch) + x);
    if constexpr (mode == Mode::M440)
      return vmulq_f32(vaddq_f32(mid, below), vdupq_n_f32(0.5f));
    else {
      auto above = vld1q_f32(reinterpret_cast<const float*>(pMask + prev_pitch) + x);
      return vmulq_f32(vaddq_f32(vaddq_f32(above, vmulq_f32(mid, vdupq_n_f32(2.0f))), below), vdupq_n_f32(0.25f));
    }
  }
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
  constexpr int mstep = mode == Mode::M444 || mode == Mode::M440 || mode == Mode::TopLeft440 ? 1 : 2;
  const int wmod = nWidth / 4 * 4;
  const float32x4_t one = vdupq_n_f32(1.0f);
  uint8_t *pDst0 = pDst;
  const uint8_t *pSrc0 = pSrc1, *pMask0 = pMask;
  for (int y = 0; y < nHeight; y++) {
    float *d = reinterpret_cast<float *>(pDst);
    const float *s = reinterpret_cast<const float *>(pSrc1);
    for (int x = 0; x < wmod; x += 4) {
      float32x4_t m = mask32<mode>(pMask, nMaskPitch, x, y == 0 ? 0 : -nMaskPitch);
      vst1q_f32(d + x, vaddq_f32(vmulq_f32(vsubq_f32(one, m), vld1q_f32(d + x)), vmulq_f32(m, vld1q_f32(s + x))));
    }
    pDst += nDstPitch;
    pSrc1 += nSrc1Pitch;
    pMask += (mode == Mode::M420 || mode == Mode::M440 || mode == Mode::TopLeft440) ? nMaskPitch * 2 : nMaskPitch;
  }
  if (nWidth > wmod) {
    Processor32 *c = mode == Mode::M444 ? merge32_c : mode == Mode::M420 ? merge32_luma_420_c :
      mode == Mode::M440 ? merge32_luma_440_c : mode == Mode::TopLeft440 ? merge32_luma_440_topleft_c : merge32_luma_422_c;
    c(pDst0 + wmod * 4, nDstPitch, pSrc0 + wmod * 4, nSrc1Pitch, pMask0 + wmod * 4 * mstep, nMaskPitch, nWidth - wmod, nHeight);
  }
}

} // namespace

Processor *merge_neon = &merge8_neon_t<Mode::M444>;
Processor *merge_luma_420_neon = &merge8_neon_t<Mode::M420>;
Processor *merge_luma_422_neon = &merge8_neon_t<Mode::M422>;
Processor *merge_luma_411_neon = &merge8_neon_t<Mode::M411>;
Processor *merge_luma_440_neon = &merge8_neon_t<Mode::M440>;
Processor *merge_luma_440_topleft_neon = &merge8_neon_t<Mode::TopLeft440>;
Processor *merge_luma_410_neon = merge_410_t<uint8_t, false, false, Mask410NEON, merge8_neon_t<Mode::M444>>;
Processor *merge_luma_410_mpeg2_neon = merge_410_t<uint8_t, true, false, Mask410NEON, merge8_neon_t<Mode::M444>>;
Processor *merge_luma_410_topleft_neon = merge_410_t<uint8_t, true, true, Mask410NEON, merge8_neon_t<Mode::M444>>;

Processor16 *merge16_neon = &merge16_neon_t<Mode::M444>;
Processor16 *merge16_luma_420_neon = &merge16_neon_t<Mode::M420>;
Processor16 *merge16_luma_422_neon = &merge16_neon_t<Mode::M422>;
Processor16 *merge16_luma_411_neon = &merge16_neon_t<Mode::M411>;
Processor16 *merge16_luma_440_neon = &merge16_neon_t<Mode::M440>;
Processor16 *merge16_luma_440_topleft_neon = &merge16_neon_t<Mode::TopLeft440>;
Processor16 *merge16_luma_410_neon = merge16_410_t<false, false, Mask410NEON, merge16_neon_t<Mode::M444>>;
Processor16 *merge16_luma_410_mpeg2_neon = merge16_410_t<true, false, Mask410NEON, merge16_neon_t<Mode::M444>>;
Processor16 *merge16_luma_410_topleft_neon = merge16_410_t<true, true, Mask410NEON, merge16_neon_t<Mode::M444>>;

Processor32 *merge32_neon = &merge32_neon_t<Mode::M444>;
Processor32 *merge32_luma_420_neon = &merge32_neon_t<Mode::M420>;
Processor32 *merge32_luma_422_neon = &merge32_neon_t<Mode::M422>;
Processor32 *merge32_luma_440_neon = &merge32_neon_t<Mode::M440>;
Processor32 *merge32_luma_440_topleft_neon = &merge32_neon_t<Mode::TopLeft440>;
Processor32 *merge32_luma_410_neon = merge_410_t<float, false, false, Mask410NEON, nullptr>;
Processor32 *merge32_luma_410_mpeg2_neon = merge_410_t<float, true, false, Mask410NEON, nullptr>;
Processor32 *merge32_luma_410_topleft_neon = merge_410_t<float, true, true, Mask410NEON, nullptr>;

} } } }

#endif // NEON_INTRINSICS
