#ifndef __Mt_merge_410_SIMD_H__
#define __Mt_merge_410_SIMD_H__

#include "merge_mask.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Merge {

namespace {

#ifdef INTEL_INTRINSICS
template<typename T>
struct Mask410SSE {
  using V = __m128i;
  static constexpr int lanes = 4;
  static constexpr int bytes = 16;
  static V zero() { return _mm_setzero_si128(); }
  static V add(V a, V b) { return _mm_add_epi32(a, b); }
  template<int weight> static V mul(V a) {
    if constexpr (weight == 1) return a;
    else if constexpr (weight == 2) return _mm_slli_epi32(a, 1);
    else if constexpr (weight == 3) return add(a, _mm_slli_epi32(a, 1));
    else return _mm_slli_epi32(a, 2);
  }
  template<int tap> static V column16(V a, V b) {
    a = _mm_shuffle_epi32(_mm_srli_epi64(a, tap * 16), _MM_SHUFFLE(2, 0, 2, 0));
    b = _mm_shuffle_epi32(_mm_srli_epi64(b, tap * 16), _MM_SHUFFLE(2, 0, 2, 0));
    return _mm_and_si128(_mm_unpacklo_epi64(a, b), _mm_set1_epi32(65535));
  }
  static void columns(const T *p, V (&c)[4]) {
    auto a = _mm_loadu_si128(reinterpret_cast<const V*>(p));
    if constexpr (sizeof(T) == 1) {
      auto mask = _mm_set1_epi32(255);
      c[0] = _mm_and_si128(a, mask);
      c[1] = _mm_and_si128(_mm_srli_epi32(a, 8), mask);
      c[2] = _mm_and_si128(_mm_srli_epi32(a, 16), mask);
      c[3] = _mm_srli_epi32(a, 24);
    }
    else {
      auto b = _mm_loadu_si128(reinterpret_cast<const V*>(p + 8));
      c[0] = column16<0>(a, b); c[1] = column16<1>(a, b);
      c[2] = column16<2>(a, b); c[3] = column16<3>(a, b);
    }
  }
  template<int shift> static V normalize(V a) {
    return _mm_srli_epi32(add(a, _mm_set1_epi32(1 << (shift - 1))), shift);
  }
  static void store(T *p, V a) {
    // Sign-extend the low word before signed packing, preserving the full uint16_t range.
    auto words = _mm_packs_epi32(_mm_srai_epi32(_mm_slli_epi32(a, 16), 16), zero());
    if constexpr (sizeof(T) == 1) {
      auto packed = _mm_packus_epi16(words, zero());
      const int value = _mm_cvtsi128_si32(packed);
      memcpy(p, &value, sizeof(value));
    }
    else
      _mm_storel_epi64(reinterpret_cast<V*>(p), words);
  }
};

template<>
struct Mask410SSE<float> {
  using V = __m128;
  static constexpr int lanes = 4;
  static constexpr int bytes = 16;
  static V zero() { return _mm_setzero_ps(); }
  static V add(V a, V b) { return _mm_add_ps(a, b); }
  template<int weight> static V mul(V a) { return _mm_mul_ps(a, _mm_set1_ps(float(weight))); }
  static void pairs(const float *p, V &even, V &odd) {
    auto a = _mm_loadu_ps(p), b = _mm_loadu_ps(p + 4);
    even = _mm_shuffle_ps(a, b, _MM_SHUFFLE(2, 0, 2, 0));
    odd = _mm_shuffle_ps(a, b, _MM_SHUFFLE(3, 1, 3, 1));
  }
  static void columns(const float *p, V (&c)[4]) {
    c[0] = _mm_loadu_ps(p); c[1] = _mm_loadu_ps(p + 4);
    c[2] = _mm_loadu_ps(p + 8); c[3] = _mm_loadu_ps(p + 12);
    _MM_TRANSPOSE4_PS(c[0], c[1], c[2], c[3]);
  }
  template<int shift> static V normalize(V a) { return _mm_mul_ps(a, _mm_set1_ps(1.0f / (1 << shift))); }
  static void store(float *p, V a) { _mm_storeu_ps(p, a); }
  static void blend(float *d, const float *s, const float *m) {
    auto mask = _mm_loadu_ps(m);
    store(d, add(_mm_mul_ps(_mm_sub_ps(_mm_set1_ps(1.0f), mask), _mm_loadu_ps(d)),
      _mm_mul_ps(mask, _mm_loadu_ps(s))));
  }
};

#ifdef MT_MERGE_410_AVX2
template<typename T>
struct Mask410AVX {
  using V = __m256i;
  static constexpr int lanes = 8;
  static constexpr int bytes = 32;
  static V zero() { return _mm256_setzero_si256(); }
  static V add(V a, V b) { return _mm256_add_epi32(a, b); }
  template<int weight> static V mul(V a) {
    if constexpr (weight == 1) return a;
    else if constexpr (weight == 2) return _mm256_slli_epi32(a, 1);
    else if constexpr (weight == 3) return add(a, _mm256_slli_epi32(a, 1));
    else return _mm256_slli_epi32(a, 2);
  }
  static void columns(const T *p, V (&c)[4]) {
    __m128i lo[4], hi[4];
    Mask410SSE<T>::columns(p, lo);
    Mask410SSE<T>::columns(p + 16, hi);
    for (int i = 0; i < 4; ++i)
      c[i] = _mm256_inserti128_si256(_mm256_castsi128_si256(lo[i]), hi[i], 1);
  }
  template<int shift> static V normalize(V a) {
    return _mm256_srli_epi32(add(a, _mm256_set1_epi32(1 << (shift - 1))), shift);
  }
  static void store(T *p, V a) {
    Mask410SSE<T>::store(p, _mm256_castsi256_si128(a));
    Mask410SSE<T>::store(p + 4, _mm256_extracti128_si256(a, 1));
  }
};

template<>
struct Mask410AVX<float> {
  using V = __m256;
  static constexpr int lanes = 8;
  static constexpr int bytes = 32;
  static V zero() { return _mm256_setzero_ps(); }
  static V add(V a, V b) { return _mm256_add_ps(a, b); }
  template<int weight> static V mul(V a) { return _mm256_mul_ps(a, _mm256_set1_ps(float(weight))); }
  static void pairs(const float *p, V &even, V &odd) {
    auto a = _mm256_loadu_ps(p), b = _mm256_loadu_ps(p + 8);
    const auto order = _mm256_setr_epi32(0, 1, 4, 5, 2, 3, 6, 7);
    even = _mm256_permutevar8x32_ps(_mm256_shuffle_ps(a, b, _MM_SHUFFLE(2, 0, 2, 0)), order);
    odd = _mm256_permutevar8x32_ps(_mm256_shuffle_ps(a, b, _MM_SHUFFLE(3, 1, 3, 1)), order);
  }
  static void columns(const float *p, V (&c)[4]) {
    __m128 lo[4], hi[4];
    Mask410SSE<float>::columns(p, lo);
    Mask410SSE<float>::columns(p + 16, hi);
    for (int i = 0; i < 4; ++i)
      c[i] = _mm256_insertf128_ps(_mm256_castps128_ps256(lo[i]), hi[i], 1);
  }
  template<int shift> static V normalize(V a) { return _mm256_mul_ps(a, _mm256_set1_ps(1.0f / (1 << shift))); }
  static void store(float *p, V a) { _mm256_storeu_ps(p, a); }
  static void blend(float *d, const float *s, const float *m) {
    auto mask = _mm256_loadu_ps(m);
    store(d, add(_mm256_mul_ps(_mm256_sub_ps(_mm256_set1_ps(1.0f), mask), _mm256_loadu_ps(d)),
      _mm256_mul_ps(mask, _mm256_loadu_ps(s))));
  }
};
#endif
#endif

#ifdef NEON_INTRINSICS
template<typename T>
struct Mask410NEON {
  using V = uint32x4_t;
  static constexpr int lanes = 4;
  static constexpr int bytes = 16;
  static V zero() { return vdupq_n_u32(0); }
  static V add(V a, V b) { return vaddq_u32(a, b); }
  template<int weight> static V mul(V a) { return vmulq_n_u32(a, weight); }
  static void columns(const T *p, V (&c)[4]) {
    if constexpr (sizeof(T) == 1) {
      auto pairs = vuzp_u8(vld1_u8(p), vld1_u8(p + 8));
      auto even = vuzp_u8(pairs.val[0], pairs.val[0]);
      auto odd = vuzp_u8(pairs.val[1], pairs.val[1]);
      c[0] = vmovl_u16(vget_low_u16(vmovl_u8(even.val[0])));
      c[1] = vmovl_u16(vget_low_u16(vmovl_u8(odd.val[0])));
      c[2] = vmovl_u16(vget_low_u16(vmovl_u8(even.val[1])));
      c[3] = vmovl_u16(vget_low_u16(vmovl_u8(odd.val[1])));
    }
    else {
      auto r = vld4_u16(p);
      for (int i = 0; i < 4; ++i) c[i] = vmovl_u16(r.val[i]);
    }
  }
  template<int shift> static V normalize(V a) { return vrshrq_n_u32(a, shift); }
  static void store(T *p, V a) {
    auto words = vmovn_u32(a);
    if constexpr (sizeof(T) == 1) {
      auto packed = vmovn_u16(vcombine_u16(words, words));
      const uint32_t value = vget_lane_u32(vreinterpret_u32_u8(packed), 0);
      memcpy(p, &value, sizeof(value));
    }
    else vst1_u16(p, words);
  }
};

template<>
struct Mask410NEON<float> {
  using V = float32x4_t;
  static constexpr int lanes = 4;
  static constexpr int bytes = 16;
  static V zero() { return vdupq_n_f32(0); }
  static V add(V a, V b) { return vaddq_f32(a, b); }
  template<int weight> static V mul(V a) { return vmulq_n_f32(a, float(weight)); }
  static void columns(const float *p, V (&c)[4]) {
    auto r = vld4q_f32(p);
    for (int i = 0; i < 4; ++i) c[i] = r.val[i];
  }
  template<int shift> static V normalize(V a) { return vmulq_n_f32(a, 1.0f / (1 << shift)); }
  static void store(float *p, V a) { vst1q_f32(p, a); }
  static void blend(float *d, const float *s, const float *m) {
    auto mask = vld1q_f32(m);
    store(d, add(vmulq_f32(vsubq_f32(vdupq_n_f32(1), mask), vld1q_f32(d)), vmulq_f32(mask, vld1q_f32(s))));
  }
};
#endif

template<typename T, bool left, bool top, template<typename> class Ops>
MT_FORCEINLINE typename Ops<T>::V get_mask_410_simd(const uint8_t *row, ptrdiff_t pitch, int x, int y)
{
  using O = Ops<T>;
  constexpr int shift = (left ? 4 : 2) + (top ? 4 : 2);
  auto sum = O::zero();
  for (int dy = top ? -3 : 0; dy <= 3; ++dy) {
    const int offset = std::max(y * 4 + dy, 0) - y * 4;
    const T *p = reinterpret_cast<const T*>(row + offset * pitch) + x * 4;
    typename O::V c[4];
    O::columns(p, c);
    auto h = O::zero();
    if constexpr (left) {
      typename O::V prev[4];
      O::columns(p - 4, prev);
      h = O::add(h, O::template mul<1>(prev[1]));
      h = O::add(h, O::template mul<2>(prev[2]));
      h = O::add(h, O::template mul<3>(prev[3]));
      h = O::add(h, O::template mul<4>(c[0]));
      h = O::add(h, O::template mul<3>(c[1]));
      h = O::add(h, O::template mul<2>(c[2]));
      h = O::add(h, O::template mul<1>(c[3]));
    }
    else
      for (int i = 0; i < 4; ++i) h = O::add(h, O::template mul<1>(c[i]));
    // Keep the C horizontal-then-vertical float operation order (including the initial zero).
    if constexpr (top) {
      switch (4 - std::abs(dy)) {
      case 1: sum = O::add(sum, O::template mul<1>(h)); break;
      case 2: sum = O::add(sum, O::template mul<2>(h)); break;
      case 3: sum = O::add(sum, O::template mul<3>(h)); break;
      case 4: sum = O::add(sum, O::template mul<4>(h)); break;
      }
    }
    else sum = O::add(sum, O::template mul<1>(h));
  }
  return O::template normalize<shift>(sum);
}

template<typename T, bool left, bool top, template<typename> class Ops, auto blend>
void merge_410_simd(uint8_t *dst, ptrdiff_t dst_pitch, const uint8_t *src, ptrdiff_t src_pitch,
  const uint8_t *mask, ptrdiff_t mask_pitch, int width, int height, int bits)
{
  using O = Ops<T>;
  constexpr int block = O::bytes / sizeof(T);
  const uint32_t max = std::is_same<T, float>::value ? 0 : (1u << bits) - 1;
  for (int y = 0; y < height; ++y) {
    T *d = reinterpret_cast<T*>(dst);
    const T *s = reinterpret_cast<const T*>(src);
    auto scalar = [&](int x) {
      const T m = get_mask_410_c<T, left, top>(mask, mask_pitch, x, y);
      if constexpr (std::is_same<T, float>::value) d[x] = (1.0f - m) * d[x] + m * s[x];
      else d[x] = T((uint32_t(d[x]) * (max - m) + uint32_t(s[x]) * m + max / 2) / max);
    };
    int x = 0;
    // Only the first co-sited pixel needs horizontal replication. Preserve the mask origin for tails.
    if constexpr (left) { if (width > 0) scalar(x++); }
    for (; x + block <= width; x += block) {
      alignas(32) T reduced[block];
      for (int i = 0; i < block; i += O::lanes)
        O::store(reduced + i, get_mask_410_simd<T, left, top, Ops>(mask, mask_pitch, x + i, y));
      if constexpr (std::is_same<T, float>::value) O::blend(d + x, s + x, reduced);
      else if constexpr (sizeof(T) == 2)
        blend(reinterpret_cast<uint8_t*>(d + x), 0, reinterpret_cast<const uint8_t*>(s + x), 0,
          reinterpret_cast<const uint8_t*>(reduced), 0, block, 1, bits);
      else
        blend(d + x, 0, s + x, 0, reduced, 0, block, 1);
    }
    for (; x < width; ++x) scalar(x);
    dst += dst_pitch; src += src_pitch; mask += mask_pitch * 4;
  }
}

template<typename T, bool left, bool top, template<typename> class Ops, auto blend>
void merge_410_t(uint8_t *dst, ptrdiff_t dst_pitch, const uint8_t *src, ptrdiff_t src_pitch,
  const uint8_t *mask, ptrdiff_t mask_pitch, int width, int height)
{
  merge_410_simd<T, left, top, Ops, blend>(dst, dst_pitch, src, src_pitch, mask, mask_pitch, width, height, 8);
}

template<bool left, bool top, template<typename> class Ops, auto blend>
void merge16_410_t(uint8_t *dst, ptrdiff_t dst_pitch, const uint8_t *src, ptrdiff_t src_pitch,
  const uint8_t *mask, ptrdiff_t mask_pitch, int width, int height, int bits)
{
  merge_410_simd<uint16_t, left, top, Ops, blend>(dst, dst_pitch, src, src_pitch, mask, mask_pitch, width, height, bits);
}

} // namespace

} } } }

#endif // __Mt_merge_410_SIMD_H__
