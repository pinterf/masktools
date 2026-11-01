#include "binarize.h"

#ifdef NEON_INTRINSICS

#include <arm_neon.h>

// mt_binarize NEON: x > t ? A : B, A and B one of 0, max (255, 2^bits - 1, 1.0), x, t; 8, 10-16 bit, float.

namespace Filtering { namespace MaskTools { namespace Filters { namespace Binarize {

namespace {

enum Val { Z, M, X, T };

struct U8 {
  using T_ = uint8_t; using V = uint8x16_t; static constexpr int L = 16;
  static MT_FORCEINLINE V ld(const T_ *p) { return vld1q_u8(p); }
  static MT_FORCEINLINE void st(T_ *p, V v) { vst1q_u8(p, v); }
  static MT_FORCEINLINE V dup(T_ v) { return vdupq_n_u8(v); }
  static MT_FORCEINLINE V sel(V x, V t, V a, V b) { return vbslq_u8(vcgtq_u8(x, t), a, b); }
};
struct U16 {
  using T_ = uint16_t; using V = uint16x8_t; static constexpr int L = 8;
  static MT_FORCEINLINE V ld(const T_ *p) { return vld1q_u16(p); }
  static MT_FORCEINLINE void st(T_ *p, V v) { vst1q_u16(p, v); }
  static MT_FORCEINLINE V dup(T_ v) { return vdupq_n_u16(v); }
  static MT_FORCEINLINE V sel(V x, V t, V a, V b) { return vbslq_u16(vcgtq_u16(x, t), a, b); }
};
struct F32 {
  using T_ = float; using V = float32x4_t; static constexpr int L = 4;
  static MT_FORCEINLINE V ld(const T_ *p) { return vld1q_f32(p); }
  static MT_FORCEINLINE void st(T_ *p, V v) { vst1q_f32(p, v); }
  static MT_FORCEINLINE V dup(T_ v) { return vdupq_n_f32(v); }
  static MT_FORCEINLINE V sel(V x, V t, V a, V b) { return vbslq_f32(vcgtq_f32(x, t), a, b); }
};

template<Val v, class T_>
MT_FORCEINLINE T_ pick(T_ x, T_ t, T_ max_pixel_value) { return v == Z ? T_(0) : v == M ? max_pixel_value : v == X ? x : t; }

template<class Lane, Val a, Val b>
MT_FORCEINLINE void binarize_neon_t(uint8_t *pDst, ptrdiff_t nDstPitch, typename Lane::T_ nThreshold, typename Lane::T_ max_pixel_value, int nWidth, int nHeight)
{
  using T_ = typename Lane::T_;
  using V = typename Lane::V;
  constexpr int L = Lane::L;
  const int wmod = nWidth / L * L;
  const V t = Lane::dup(nThreshold), v_max_pixel_value = Lane::dup(max_pixel_value), zero = Lane::dup(T_(0));
  for (int y = 0; y < nHeight; y++) {
    T_ *d = reinterpret_cast<T_ *>(pDst);
    for (int x = 0; x < wmod; x += L) {
      V v = Lane::ld(d + x);
      V va = a == Z ? zero : a == M ? v_max_pixel_value : a == X ? v : t;
      V vb = b == Z ? zero : b == M ? v_max_pixel_value : b == X ? v : t;
      Lane::st(d + x, Lane::sel(v, t, va, vb));
    }
    for (int x = wmod; x < nWidth; x++) {
      T_ v = d[x];
      d[x] = v > nThreshold ? pick<a>(v, nThreshold, max_pixel_value) : pick<b>(v, nThreshold, max_pixel_value);
    }
    pDst += nDstPitch;
  }
}

template<Val a, Val b>
void binarize8_neon(uint8_t *pDst, ptrdiff_t nDstPitch, uint8_t nThreshold, int nWidth, int nHeight)
{
  binarize_neon_t<U8, a, b>(pDst, nDstPitch, nThreshold, 255, nWidth, nHeight);
}

template<Val a, Val b>
void binarize16_neon(uint8_t *pDst, ptrdiff_t nDstPitch, uint16_t nThreshold, int nWidth, int nHeight, int bits_per_pixel)
{
  binarize_neon_t<U16, a, b>(pDst, nDstPitch, nThreshold, uint16_t((1 << bits_per_pixel) - 1), nWidth, nHeight);
}

template<Val a, Val b>
void binarize32_neon(uint8_t *pDst, ptrdiff_t nDstPitch, float nThreshold, int nWidth, int nHeight)
{
  binarize_neon_t<F32, a, b>(pDst, nDstPitch, nThreshold, 1.0f, nWidth, nHeight);
}

} // namespace

#define DEFINE_NEON_VERSIONS(name, a, b) \
Processor *binarize_##name##_neon = &binarize8_neon<a, b>; \
Processor16 *binarize_##name##_native_neon = &binarize16_neon<a, b>; \
Processor32 *binarize32_##name##_neon = &binarize32_neon<a, b>;

DEFINE_NEON_VERSIONS(upper, Z, M)
DEFINE_NEON_VERSIONS(lower, M, Z)
DEFINE_NEON_VERSIONS(0_x, Z, X)
DEFINE_NEON_VERSIONS(t_x, T, X)
DEFINE_NEON_VERSIONS(x_0, X, Z)
DEFINE_NEON_VERSIONS(x_t, X, T)
DEFINE_NEON_VERSIONS(t_0, T, Z)
DEFINE_NEON_VERSIONS(0_t, Z, T)
DEFINE_NEON_VERSIONS(x_255, X, M)
DEFINE_NEON_VERSIONS(t_255, T, M)
DEFINE_NEON_VERSIONS(255_x, M, X)
DEFINE_NEON_VERSIONS(255_t, M, T)
#undef DEFINE_NEON_VERSIONS

} } } }

#endif // NEON_INTRINSICS
