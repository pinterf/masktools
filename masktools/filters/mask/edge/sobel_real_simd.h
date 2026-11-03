#ifndef MT_SOBEL_REAL_SIMD_H
#define MT_SOBEL_REAL_SIMD_H

#include "edgemask.h"
#include <cmath>
#include <cstring>
#include <type_traits>

#ifdef INTEL_INTRINSICS
#include <immintrin.h>
#endif
#ifdef NEON_INTRINSICS
#include <arm_neon.h>
#endif

namespace Filtering { namespace MaskTools { namespace Filters { namespace Mask { namespace Edge {
namespace {

#ifdef INTEL_INTRINSICS
struct SobelSSE {
  using V = __m128;
  static constexpr int lanes = 4;
  static V set(float x) { return _mm_set1_ps(x); }
  static V add(V a, V b) { return _mm_add_ps(a, b); }
  static V sub(V a, V b) { return _mm_sub_ps(a, b); }
  static V mul(V a, V b) { return _mm_mul_ps(a, b); }
  static V root(V a) { return _mm_sqrt_ps(a); }
  static V round(V a) { return _mm_cvtepi32_ps(_mm_cvtps_epi32(a)); }
  static V finish(V a, V lo, V hi, V maximum) {
    V r = _mm_or_ps(_mm_and_ps(_mm_cmpgt_ps(a, hi), maximum), _mm_andnot_ps(_mm_cmpgt_ps(a, hi), a));
    r = _mm_andnot_ps(_mm_cmple_ps(a, lo), r);
    return _mm_min_ps(maximum, _mm_max_ps(set(0), r));
  }
  static V load(const float *p) { return _mm_loadu_ps(p); }
  static V load(const uint16_t *p) {
    return _mm_cvtepi32_ps(_mm_unpacklo_epi16(_mm_loadl_epi64(reinterpret_cast<const __m128i*>(p)), _mm_setzero_si128()));
  }
  static V load(const uint8_t *p) {
    int32_t bytes;
    std::memcpy(&bytes, p, sizeof(bytes));
    auto words = _mm_unpacklo_epi8(_mm_cvtsi32_si128(bytes), _mm_setzero_si128());
    return _mm_cvtepi32_ps(_mm_unpacklo_epi16(words, _mm_setzero_si128()));
  }
  static void store(float *p, V a) { _mm_storeu_ps(p, a); }
  static void store(uint16_t *p, V a) {
    auto biased = _mm_sub_epi32(_mm_cvtps_epi32(a), _mm_set1_epi32(32768));
    auto words = _mm_xor_si128(_mm_packs_epi32(biased, biased), _mm_set1_epi16(-32768));
    _mm_storel_epi64(reinterpret_cast<__m128i*>(p), words);
  }
  static void store(uint8_t *p, V a) {
    auto words = _mm_packs_epi32(_mm_cvtps_epi32(a), _mm_setzero_si128());
    int32_t bytes = _mm_cvtsi128_si32(_mm_packus_epi16(words, words));
    std::memcpy(p, &bytes, sizeof(bytes));
  }
};

#ifdef MT_SOBEL_AVX2
struct SobelAVX {
  using V = __m256;
  static constexpr int lanes = 8;
  static V set(float x) { return _mm256_set1_ps(x); }
  static V add(V a, V b) { return _mm256_add_ps(a, b); }
  static V sub(V a, V b) { return _mm256_sub_ps(a, b); }
  static V mul(V a, V b) { return _mm256_mul_ps(a, b); }
  static V root(V a) { return _mm256_sqrt_ps(a); }
  static V round(V a) { return _mm256_cvtepi32_ps(_mm256_cvtps_epi32(a)); }
  static V finish(V a, V lo, V hi, V maximum) {
    V mask = _mm256_cmp_ps(a, hi, _CMP_GT_OQ);
    V r = _mm256_blendv_ps(a, maximum, mask);
    r = _mm256_andnot_ps(_mm256_cmp_ps(a, lo, _CMP_LE_OQ), r);
    return _mm256_min_ps(maximum, _mm256_max_ps(set(0), r));
  }
  static V load(const float *p) { return _mm256_loadu_ps(p); }
  static V load(const uint16_t *p) {
    return _mm256_cvtepi32_ps(_mm256_cvtepu16_epi32(_mm_loadu_si128(reinterpret_cast<const __m128i*>(p))));
  }
  static V load(const uint8_t *p) {
    return _mm256_cvtepi32_ps(_mm256_cvtepu8_epi32(_mm_loadl_epi64(reinterpret_cast<const __m128i*>(p))));
  }
  template<typename T> static void store(T *p, V a) {
    SobelSSE::store(p, _mm256_castps256_ps128(a));
    SobelSSE::store(p + 4, _mm256_extractf128_ps(a, 1));
  }
};
#endif
#endif

#ifdef NEON_INTRINSICS
struct SobelNEON {
  using V = float32x4_t;
  static constexpr int lanes = 4;
  static V set(float x) { return vdupq_n_f32(x); }
  static V add(V a, V b) { return vaddq_f32(a, b); }
  static V sub(V a, V b) { return vsubq_f32(a, b); }
  static V mul(V a, V b) { return vmulq_f32(a, b); }
  static V root(V a) { return vsqrtq_f32(a); }
  static V round(V a) { return vcvtq_f32_s32(vcvtnq_s32_f32(a)); }
  static V finish(V a, V lo, V hi, V maximum) {
    V r = vbslq_f32(vcgtq_f32(a, hi), maximum, a);
    r = vbslq_f32(vcleq_f32(a, lo), set(0), r);
    return vminq_f32(maximum, vmaxq_f32(set(0), r));
  }
  static V load(const float *p) { return vld1q_f32(p); }
  static V load(const uint16_t *p) { return vcvtq_f32_u32(vmovl_u16(vld1_u16(p))); }
  static V load(const uint8_t *p) {
    uint32_t bytes;
    std::memcpy(&bytes, p, sizeof(bytes));
    auto packed = vreinterpret_u8_u32(vdup_n_u32(bytes));
    return vcvtq_f32_u32(vmovl_u16(vget_low_u16(vmovl_u8(packed))));
  }
  static void store(float *p, V a) { vst1q_f32(p, a); }
  static void store(uint16_t *p, V a) { vst1_u16(p, vmovn_u32(vcvtq_u32_f32(a))); }
  static void store(uint8_t *p, V a) {
    auto words = vmovn_u32(vcvtq_u32_f32(a));
    auto bytes = vmovn_u16(vcombine_u16(words, words));
    uint32_t packed = vget_lane_u32(vreinterpret_u32_u8(bytes), 0);
    std::memcpy(p, &packed, sizeof(packed));
  }
};
#endif

template<typename T>
T sobel_real_pixel(const T *above, const T *row, const T *below, int x, int width, float lo, float hi, float maximum) {
  const int left = x > 0 ? x - 1 : 0, right = x + 1 < width ? x + 1 : x;
  const float gx = (float(above[right]) - above[left]) + (float(below[right]) - below[left]) +
    2.0f * (float(row[right]) - row[left]);
  const float gy = (float(below[left]) - above[left]) + (float(below[right]) - above[right]) +
    2.0f * (float(below[x]) - above[x]);
  float magnitude = std::sqrt(gx * gx + gy * gy);
  if constexpr (!std::is_same_v<T, float>) magnitude = float(std::lrint(magnitude));
  float result = magnitude <= lo ? 0 : magnitude > hi ? maximum : magnitude;
  return static_cast<T>(min<float>(maximum, max<float>(0, result)));
}

template<class Ops, typename T, typename Coeff, typename Threshold>
void sobel_real_simd(T *dst, ptrdiff_t dstPitch, const T *src, ptrdiff_t srcPitch,
  const Coeff *, Threshold low, Threshold high, int width, int height, int bits = 8) {
  constexpr bool floating = std::is_same_v<T, float>;
  const float maximum = floating ? 1.0f : float((1 << bits) - 1);
  const auto lo = Ops::set(float(low)), hi = Ops::set(float(high)), mx = Ops::set(maximum);
  const auto two = Ops::set(2.0f);
  dstPitch /= sizeof(T); srcPitch /= sizeof(T);
  for (int y = 0; y < height; ++y) {
    const T *row = src + y * srcPitch;
    const T *above = y > 0 ? row - srcPitch : row;
    const T *below = y + 1 < height ? row + srcPitch : row;
    T *d = dst + y * dstPitch;
    if (width <= 0) continue;
    d[0] = sobel_real_pixel(above, row, below, 0, width, float(low), float(high), maximum);
    int x = 1;
    // Keep every neighbor load inside the plane; scalar edges/tails retain the original window origin.
    for (; x + Ops::lanes < width; x += Ops::lanes) {
      auto tl = Ops::load(above + x - 1), tr = Ops::load(above + x + 1);
      auto bl = Ops::load(below + x - 1), br = Ops::load(below + x + 1);
      // Integer samples and derivatives (at most 4*65535) are exact in float32.
      auto gx = Ops::add(Ops::add(Ops::sub(tr, tl), Ops::sub(br, bl)),
        Ops::mul(two, Ops::sub(Ops::load(row + x + 1), Ops::load(row + x - 1))));
      auto gy = Ops::add(Ops::add(Ops::sub(bl, tl), Ops::sub(br, tr)),
        Ops::mul(two, Ops::sub(Ops::load(below + x), Ops::load(above + x))));
      auto magnitude = Ops::root(Ops::add(Ops::mul(gx, gx), Ops::mul(gy, gy)));
      // Intentionally native nearest-even rounding, without Expr's +0.5f bias.
      if constexpr (!floating) magnitude = Ops::round(magnitude);
      Ops::store(d + x, Ops::finish(magnitude, lo, hi, mx));
    }
    for (; x < width; ++x)
      d[x] = sobel_real_pixel(above, row, below, x, width, float(low), float(high), maximum);
  }
}

// Keep the public processor signatures unchanged; only native16 takes a run-time depth.
template<class Ops>
void sobel_real8_simd(uint8_t *d, ptrdiff_t dp, const uint8_t *s, ptrdiff_t sp, const int16_t *m,
  int lo, int hi, int w, int h) {
  sobel_real_simd<Ops>(d, dp, s, sp, m, lo, hi, w, h);
}
template<class Ops>
void sobel_real32_simd(float *d, ptrdiff_t dp, const float *s, ptrdiff_t sp, const float *m,
  float lo, float hi, int w, int h) {
  sobel_real_simd<Ops>(d, dp, s, sp, m, lo, hi, w, h);
}

} // namespace
} } } } }
#endif
