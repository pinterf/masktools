#include "merge.h"
#include "merge16_c.h"

#ifdef NEON_INTRINSICS

#include "merge_c.h"
#include <arm_neon.h>
#include "merge32_mask_simd.h"

// mt_merge NEON: full-size blending and every supported luma-mask reduction, at all depths/placements.
// Bit-exact to C (rounded pair averages: vrhadd; float in C order, unfused).

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

enum class WeightedMode { M420, TopLeft420, M422 };

template<typename T>
MT_FORCEINLINE void load_mask_pairs(const T *p, typename Mask410NEON<T>::V &even, typename Mask410NEON<T>::V &odd)
{
  if constexpr (std::is_same<T, float>::value) {
    auto r = vld2q_f32(p);
    even = r.val[0]; odd = r.val[1];
  }
  else if constexpr (sizeof(T) == 2) {
    auto r = vld2_u16(p);
    even = vmovl_u16(r.val[0]); odd = vmovl_u16(r.val[1]);
  }
  else {
    auto row = vld1_u8(p);
    auto r = vuzp_u8(row, row);
    even = vmovl_u16(vget_low_u16(vmovl_u8(r.val[0])));
    odd = vmovl_u16(vget_low_u16(vmovl_u8(r.val[1])));
  }
}

template<typename T, WeightedMode mode>
MT_FORCEINLINE void weighted_columns_neon(const uint8_t *row, ptrdiff_t pitch, ptrdiff_t prev_pitch, int x,
  typename Mask410NEON<T>::V &even, typename Mask410NEON<T>::V &odd)
{
  using O = Mask410NEON<T>;
  load_mask_pairs(reinterpret_cast<const T*>(row) + x, even, odd);
  if constexpr (mode != WeightedMode::M422) {
    typename O::V below_even, below_odd;
    load_mask_pairs(reinterpret_cast<const T*>(row + pitch) + x, below_even, below_odd);
    if constexpr (mode == WeightedMode::TopLeft420) {
      typename O::V above_even, above_odd;
      load_mask_pairs(reinterpret_cast<const T*>(row + prev_pitch) + x, above_even, above_odd);
      // Above + 2*center + below, in the same order as the C float reducer.
      even = O::add(above_even, O::template mul<2>(even));
      odd = O::add(above_odd, O::template mul<2>(odd));
    }
    even = O::add(even, below_even);
    odd = O::add(odd, below_odd);
  }
}

template<typename T, WeightedMode mode>
MT_FORCEINLINE T weighted_mask_c(const uint8_t *row, ptrdiff_t pitch, ptrdiff_t prev_pitch, int x)
{
  using Sum = typename std::conditional<std::is_same<T, float>::value, float, uint32_t>::type;
  constexpr int shift = mode == WeightedMode::TopLeft420 ? 4 : mode == WeightedMode::M420 ? 3 : 2;
  auto column = [&](int i) -> Sum {
    const Sum mid = reinterpret_cast<const T*>(row)[i];
    if constexpr (mode == WeightedMode::TopLeft420)
      return (Sum(reinterpret_cast<const T*>(row + prev_pitch)[i]) + 2 * mid) +
        Sum(reinterpret_cast<const T*>(row + pitch)[i]);
    else if constexpr (mode == WeightedMode::M420)
      return mid + Sum(reinterpret_cast<const T*>(row + pitch)[i]);
    else return mid;
  };
  const Sum sum = (column(std::max(x * 2 - 1, 0)) + 2 * column(x * 2)) + column(x * 2 + 1);
  if constexpr (std::is_same<T, float>::value) return sum * (1.0f / (1 << shift));
  else return T((sum + (1u << (shift - 1))) >> shift);
}

template<typename T, WeightedMode mode>
void merge_weighted_neon(uint8_t *dst, ptrdiff_t dst_pitch, const uint8_t *src, ptrdiff_t src_pitch,
  const uint8_t *mask, ptrdiff_t mask_pitch, int width, int height, int bits)
{
  using O = Mask410NEON<T>;
  constexpr int block = O::bytes / sizeof(T);
  constexpr int shift = mode == WeightedMode::TopLeft420 ? 4 : mode == WeightedMode::M420 ? 3 : 2;
  const uint32_t max = (1u << bits) - 1;
  const MergeMagicDiv magic = sizeof(T) == 2 ? get_merge_magic_div(bits) : MergeMagicDiv{};
  for (int y = 0; y < height; ++y) {
    T *d = reinterpret_cast<T*>(dst);
    const T *s = reinterpret_cast<const T*>(src);
    const ptrdiff_t prev_pitch = y == 0 ? 0 : -mask_pitch;
    auto scalar = [&](int x) {
      const T m = weighted_mask_c<T, mode>(mask, mask_pitch, prev_pitch, x);
      if constexpr (std::is_same<T, float>::value) d[x] = (1.0f - m) * d[x] + m * s[x];
      else if constexpr (sizeof(T) == 2) d[x] = merge16_core_c(d[x], s[x], m, uint16_t(max), magic);
      else d[x] = T((uint32_t(d[x]) * (255 - m) + uint32_t(s[x]) * m + 127) / 255);
    };
    int x = 0;
    // The first output replicates column zero; all subsequent loads have a valid left halo.
    if (width > 0) scalar(x++);
    for (; x + block <= width; x += block) {
      alignas(16) T reduced[block];
      for (int i = 0; i < block; i += O::lanes) {
        typename O::V mid, right, unused, left;
        weighted_columns_neon<T, mode>(mask, mask_pitch, prev_pitch, (x + i) * 2, mid, right);
        weighted_columns_neon<T, mode>(mask, mask_pitch, prev_pitch, (x + i) * 2 - 2, unused, left);
        auto sum = O::add(O::add(left, O::template mul<2>(mid)), right);
        O::store(reduced + i, O::template normalize<shift>(sum));
      }
      if constexpr (std::is_same<T, float>::value) O::blend(d + x, s + x, reduced);
      else if constexpr (sizeof(T) == 2)
        vst1q_u16(d + x, merge16_core(vld1q_u16(d + x), vld1q_u16(s + x), vld1q_u16(reduced), max, magic));
      else vst1q_u8(d + x, merge8_core(vld1q_u8(d + x), vld1q_u8(s + x), vld1q_u8(reduced)));
    }
    for (; x < width; ++x) scalar(x);
    dst += dst_pitch; src += src_pitch;
    mask += mask_pitch * (mode == WeightedMode::M422 ? 1 : 2);
  }
}

template<typename T, WeightedMode mode>
void merge_weighted_neon_t(uint8_t *dst, ptrdiff_t dst_pitch, const uint8_t *src, ptrdiff_t src_pitch,
  const uint8_t *mask, ptrdiff_t mask_pitch, int width, int height)
{
  merge_weighted_neon<T, mode>(dst, dst_pitch, src, src_pitch, mask, mask_pitch, width, height, 8);
}

} // namespace

Processor *merge_neon = &merge8_neon_t<Mode::M444>;
Processor *merge_luma_420_neon = &merge8_neon_t<Mode::M420>;
Processor *merge_luma_420_mpeg2_neon = merge_weighted_neon_t<uint8_t, WeightedMode::M420>;
Processor *merge_luma_420_topleft_neon = merge_weighted_neon_t<uint8_t, WeightedMode::TopLeft420>;
Processor *merge_luma_422_mpeg2_neon = merge_weighted_neon_t<uint8_t, WeightedMode::M422>;
Processor *merge_luma_422_neon = &merge8_neon_t<Mode::M422>;
Processor *merge_luma_411_neon = &merge8_neon_t<Mode::M411>;
Processor *merge_luma_440_neon = &merge8_neon_t<Mode::M440>;
Processor *merge_luma_440_topleft_neon = &merge8_neon_t<Mode::TopLeft440>;
Processor *merge_luma_410_neon = merge_410_t<uint8_t, false, false, Mask410NEON, merge8_neon_t<Mode::M444>>;
Processor *merge_luma_410_mpeg2_neon = merge_410_t<uint8_t, true, false, Mask410NEON, merge8_neon_t<Mode::M444>>;
Processor *merge_luma_410_topleft_neon = merge_410_t<uint8_t, true, true, Mask410NEON, merge8_neon_t<Mode::M444>>;

Processor16 *merge16_neon = &merge16_neon_t<Mode::M444>;
Processor16 *merge16_luma_420_neon = &merge16_neon_t<Mode::M420>;
Processor16 *merge16_luma_420_mpeg2_neon = merge_weighted_neon<uint16_t, WeightedMode::M420>;
Processor16 *merge16_luma_420_topleft_neon = merge_weighted_neon<uint16_t, WeightedMode::TopLeft420>;
Processor16 *merge16_luma_422_mpeg2_neon = merge_weighted_neon<uint16_t, WeightedMode::M422>;
Processor16 *merge16_luma_422_neon = &merge16_neon_t<Mode::M422>;
Processor16 *merge16_luma_411_neon = &merge16_neon_t<Mode::M411>;
Processor16 *merge16_luma_440_neon = &merge16_neon_t<Mode::M440>;
Processor16 *merge16_luma_440_topleft_neon = &merge16_neon_t<Mode::TopLeft440>;
Processor16 *merge16_luma_410_neon = merge16_410_t<false, false, Mask410NEON, merge16_neon_t<Mode::M444>>;
Processor16 *merge16_luma_410_mpeg2_neon = merge16_410_t<true, false, Mask410NEON, merge16_neon_t<Mode::M444>>;
Processor16 *merge16_luma_410_topleft_neon = merge16_410_t<true, true, Mask410NEON, merge16_neon_t<Mode::M444>>;

Processor32 *merge32_neon = &merge32_neon_t<Mode::M444>;
Processor32 *merge32_luma_420_neon = &merge32_neon_t<Mode::M420>;
Processor32 *merge32_luma_411_neon = merge32_luma_411_simd<Mask410NEON>;
Processor32 *merge32_luma_420_mpeg2_neon = merge_weighted_neon_t<float, WeightedMode::M420>;
Processor32 *merge32_luma_420_topleft_neon = merge_weighted_neon_t<float, WeightedMode::TopLeft420>;
Processor32 *merge32_luma_422_mpeg2_neon = merge_weighted_neon_t<float, WeightedMode::M422>;
Processor32 *merge32_luma_422_neon = &merge32_neon_t<Mode::M422>;
Processor32 *merge32_luma_440_neon = &merge32_neon_t<Mode::M440>;
Processor32 *merge32_luma_440_topleft_neon = &merge32_neon_t<Mode::TopLeft440>;
Processor32 *merge32_luma_410_neon = merge_410_t<float, false, false, Mask410NEON, nullptr>;
Processor32 *merge32_luma_410_mpeg2_neon = merge_410_t<float, true, false, Mask410NEON, nullptr>;
Processor32 *merge32_luma_410_topleft_neon = merge_410_t<float, true, true, Mask410NEON, nullptr>;

} } } }

#endif // NEON_INTRINSICS
