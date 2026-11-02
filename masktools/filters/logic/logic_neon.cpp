#include "logic.h"

#ifdef NEON_INTRINSICS

#include <arm_neon.h>
#include <cstring>

// mt_logic NEON: and, or, xor, andn (~dst & src, the x86 andnot order, as the C), min / max with the add / sub
// thresholds, 8, 10-16 bit and float. Bit-exact to C: the float min / max are compare + select as the C
// a < b ? a : b (fmin / fmax differ in the sign of zero and in NaN handling).

namespace Filtering { namespace MaskTools { namespace Filters { namespace Logic {

namespace {

/* lane types */

struct U8 {
  using T = uint8_t; using V = uint8x16_t; static constexpr int L = 16;
  static MT_FORCEINLINE V ld(const T *p) { return vld1q_u8(p); }
  static MT_FORCEINLINE void st(T *p, V v) { vst1q_u8(p, v); }
  static MT_FORCEINLINE V dup(T v) { return vdupq_n_u8(v); }
};
struct U16 {
  using T = uint16_t; using V = uint16x8_t; static constexpr int L = 8;
  static MT_FORCEINLINE V ld(const T *p) { return vld1q_u16(p); }
  static MT_FORCEINLINE void st(T *p, V v) { vst1q_u16(p, v); }
  static MT_FORCEINLINE V dup(T v) { return vdupq_n_u16(v); }
};
struct F32 {
  using T = float; using V = float32x4_t; static constexpr int L = 4;
  static MT_FORCEINLINE V ld(const T *p) { return vld1q_f32(p); }
  static MT_FORCEINLINE void st(T *p, V v) { vst1q_f32(p, v); }
  static MT_FORCEINLINE V dup(T v) { return vdupq_n_f32(v); }
};

/* threshold pre-ops: v(vector, threshold, max_pixel_value), s(scalar, threshold, max_pixel_value) */

struct Nop {
  template<class V> static MT_FORCEINLINE V v(V a, V, V) { return a; }
  template<class T> static MT_FORCEINLINE T s(T a, T, T) { return a; }
};
struct Add8 {
  static MT_FORCEINLINE uint8x16_t v(uint8x16_t a, uint8x16_t th, uint8x16_t) { return vqaddq_u8(a, th); }
  static MT_FORCEINLINE uint8_t s(uint8_t a, uint8_t th, uint8_t) { int r = a + th; return uint8_t(r > 255 ? 255 : r); }
};
struct Sub8 {
  static MT_FORCEINLINE uint8x16_t v(uint8x16_t a, uint8x16_t th, uint8x16_t) { return vqsubq_u8(a, th); }
  static MT_FORCEINLINE uint8_t s(uint8_t a, uint8_t th, uint8_t) { int r = a - th; return uint8_t(r < 0 ? 0 : r); }
};
struct Add16 {
  // the threshold is clamped to max_pixel_value; at 16 bit the min is a no-op
  static MT_FORCEINLINE uint16x8_t v(uint16x8_t a, uint16x8_t th, uint16x8_t max_pixel_value) { return vminq_u16(vqaddq_u16(a, th), max_pixel_value); }
  static MT_FORCEINLINE uint16_t s(uint16_t a, uint16_t th, uint16_t max_pixel_value) { int r = a + th; return uint16_t(r > max_pixel_value ? max_pixel_value : r); }
};
struct Sub16 {
  static MT_FORCEINLINE uint16x8_t v(uint16x8_t a, uint16x8_t th, uint16x8_t) { return vqsubq_u16(a, th); }
  static MT_FORCEINLINE uint16_t s(uint16_t a, uint16_t th, uint16_t) { int r = a - th; return uint16_t(r < 0 ? 0 : r); }
};
struct AddF {
  static MT_FORCEINLINE float32x4_t v(float32x4_t a, float32x4_t th, float32x4_t) { return vaddq_f32(a, th); }
  static MT_FORCEINLINE float s(float a, float th, float) { return a + th; }
};
struct SubF {
  static MT_FORCEINLINE float32x4_t v(float32x4_t a, float32x4_t th, float32x4_t) { return vsubq_f32(a, th); }
  static MT_FORCEINLINE float s(float a, float th, float) { return a - th; }
};

/* operators: v(dst, src, th1, th2, max_pixel_value), s(...) */

MT_FORCEINLINE uint8x16_t vand(uint8x16_t a, uint8x16_t b) { return vandq_u8(a, b); }
MT_FORCEINLINE uint16x8_t vand(uint16x8_t a, uint16x8_t b) { return vandq_u16(a, b); }
MT_FORCEINLINE float32x4_t vand(float32x4_t a, float32x4_t b) { return vreinterpretq_f32_u32(vandq_u32(vreinterpretq_u32_f32(a), vreinterpretq_u32_f32(b))); }
MT_FORCEINLINE uint8x16_t vor(uint8x16_t a, uint8x16_t b) { return vorrq_u8(a, b); }
MT_FORCEINLINE uint16x8_t vor(uint16x8_t a, uint16x8_t b) { return vorrq_u16(a, b); }
MT_FORCEINLINE float32x4_t vor(float32x4_t a, float32x4_t b) { return vreinterpretq_f32_u32(vorrq_u32(vreinterpretq_u32_f32(a), vreinterpretq_u32_f32(b))); }
MT_FORCEINLINE uint8x16_t vxor(uint8x16_t a, uint8x16_t b) { return veorq_u8(a, b); }
MT_FORCEINLINE uint16x8_t vxor(uint16x8_t a, uint16x8_t b) { return veorq_u16(a, b); }
MT_FORCEINLINE float32x4_t vxor(float32x4_t a, float32x4_t b) { return vreinterpretq_f32_u32(veorq_u32(vreinterpretq_u32_f32(a), vreinterpretq_u32_f32(b))); }
// ~a & b: vbic(b, a) = b & ~a
MT_FORCEINLINE uint8x16_t vandn(uint8x16_t a, uint8x16_t b) { return vbicq_u8(b, a); }
MT_FORCEINLINE uint16x8_t vandn(uint16x8_t a, uint16x8_t b) { return vbicq_u16(b, a); }
MT_FORCEINLINE float32x4_t vandn(float32x4_t a, float32x4_t b) { return vreinterpretq_f32_u32(vbicq_u32(vreinterpretq_u32_f32(b), vreinterpretq_u32_f32(a))); }
MT_FORCEINLINE uint8x16_t vmin(uint8x16_t a, uint8x16_t b) { return vminq_u8(a, b); }
MT_FORCEINLINE uint16x8_t vmin(uint16x8_t a, uint16x8_t b) { return vminq_u16(a, b); }
MT_FORCEINLINE float32x4_t vmin(float32x4_t a, float32x4_t b) { return vbslq_f32(vcltq_f32(a, b), a, b); }
MT_FORCEINLINE uint8x16_t vmax(uint8x16_t a, uint8x16_t b) { return vmaxq_u8(a, b); }
MT_FORCEINLINE uint16x8_t vmax(uint16x8_t a, uint16x8_t b) { return vmaxq_u16(a, b); }
MT_FORCEINLINE float32x4_t vmax(float32x4_t a, float32x4_t b) { return vbslq_f32(vcgtq_f32(a, b), a, b); }

template<class T> MT_FORCEINLINE uint32_t bits(T x) { uint32_t u = 0; memcpy(&u, &x, sizeof(T)); return u; }
template<class T> MT_FORCEINLINE T from_bits(uint32_t u) { T x; memcpy(&x, &u, sizeof(T)); return x; }

struct And {
  template<class V> static MT_FORCEINLINE V v(V a, V b, V, V, V) { return vand(a, b); }
  template<class T> static MT_FORCEINLINE T s(T a, T b, T, T, T) { return from_bits<T>(bits(a) & bits(b)); }
};
struct Or {
  template<class V> static MT_FORCEINLINE V v(V a, V b, V, V, V) { return vor(a, b); }
  template<class T> static MT_FORCEINLINE T s(T a, T b, T, T, T) { return from_bits<T>(bits(a) | bits(b)); }
};
struct Xor {
  template<class V> static MT_FORCEINLINE V v(V a, V b, V, V, V) { return vxor(a, b); }
  template<class T> static MT_FORCEINLINE T s(T a, T b, T, T, T) { return from_bits<T>(bits(a) ^ bits(b)); }
};
struct Andn {
  template<class V> static MT_FORCEINLINE V v(V a, V b, V, V, V) { return vandn(a, b); }
  template<class T> static MT_FORCEINLINE T s(T a, T b, T, T, T) { return from_bits<T>(~bits(a) & bits(b)); }
};
template<class OpA, class OpB>
struct Min {
  template<class V> static MT_FORCEINLINE V v(V a, V b, V th1, V th2, V mx) { return vmin(OpA::v(a, th1, mx), OpB::v(b, th2, mx)); }
  template<class T> static MT_FORCEINLINE T s(T a, T b, T th1, T th2, T mx) { T x = OpA::s(a, th1, mx), y = OpB::s(b, th2, mx); return x < y ? x : y; }
};
template<class OpA, class OpB>
struct Max {
  template<class V> static MT_FORCEINLINE V v(V a, V b, V th1, V th2, V mx) { return vmax(OpA::v(a, th1, mx), OpB::v(b, th2, mx)); }
  template<class T> static MT_FORCEINLINE T s(T a, T b, T th1, T th2, T mx) { T x = OpA::s(a, th1, mx), y = OpB::s(b, th2, mx); return x > y ? x : y; }
};

// pitches in bytes
template<class Lane, class Op>
MT_FORCEINLINE void logic_neon_t(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int nWidth, int nHeight,
                                 typename Lane::T th1, typename Lane::T th2, typename Lane::T max_pixel_value)
{
  using T = typename Lane::T;
  constexpr int L = Lane::L;
  const int wmod = nWidth / L * L;
  const auto vth1 = Lane::dup(th1), vth2 = Lane::dup(th2), vmax = Lane::dup(max_pixel_value);
  for (int y = 0; y < nHeight; y++) {
    T *d = reinterpret_cast<T *>(pDst);
    const T *s = reinterpret_cast<const T *>(pSrc);
    for (int x = 0; x < wmod; x += L)
      Lane::st(d + x, Op::v(Lane::ld(d + x), Lane::ld(s + x), vth1, vth2, vmax));
    for (int x = wmod; x < nWidth; x++)
      d[x] = Op::s(d[x], s[x], th1, th2, max_pixel_value);
    pDst += nDstPitch;
    pSrc += nSrcPitch;
  }
}

template<class Op>
void logic8_neon(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int nWidth, int nHeight, uint8_t th1, uint8_t th2)
{
  logic_neon_t<U8, Op>(pDst, nDstPitch, pSrc, nSrcPitch, nWidth, nHeight, th1, th2, uint8_t(255));
}

template<class Op>
void logic16_neon(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int nWidth, int nHeight, int bits_per_pixel, uint16_t th1, uint16_t th2)
{
  logic_neon_t<U16, Op>(pDst, nDstPitch, pSrc, nSrcPitch, nWidth, nHeight, th1, th2, uint16_t((1 << bits_per_pixel) - 1));
}

template<class Op>
void logic32_neon(float *pDst, ptrdiff_t nDstPitch, const float *pSrc, ptrdiff_t nSrcPitch, int nWidth, int nHeight, float th1, float th2)
{
  logic_neon_t<F32, Op>(reinterpret_cast<uint8_t *>(pDst), nDstPitch, reinterpret_cast<const uint8_t *>(pSrc), nSrcPitch, nWidth, nHeight, th1, th2, 1.0f);
}

} // namespace

/* 8 bit */

Processor *and_neon  = &logic8_neon<And>;
Processor *or_neon   = &logic8_neon<Or>;
Processor *andn_neon = &logic8_neon<Andn>;
Processor *xor_neon  = &logic8_neon<Xor>;

#define DEFINE_NEON_VERSIONS(mode, Mode) \
    Processor *mode##_neon         = &logic8_neon<Mode<Nop, Nop>>; \
    Processor *mode##sub_neon      = &logic8_neon<Mode<Nop, Sub8>>; \
    Processor *mode##add_neon      = &logic8_neon<Mode<Nop, Add8>>; \
    Processor *sub##mode##_neon    = &logic8_neon<Mode<Sub8, Nop>>; \
    Processor *sub##mode##sub_neon = &logic8_neon<Mode<Sub8, Sub8>>; \
    Processor *sub##mode##add_neon = &logic8_neon<Mode<Sub8, Add8>>; \
    Processor *add##mode##_neon    = &logic8_neon<Mode<Add8, Nop>>; \
    Processor *add##mode##sub_neon = &logic8_neon<Mode<Add8, Sub8>>; \
    Processor *add##mode##add_neon = &logic8_neon<Mode<Add8, Add8>>;

DEFINE_NEON_VERSIONS(min, Min)
DEFINE_NEON_VERSIONS(max, Max)
#undef DEFINE_NEON_VERSIONS

/* 10-16 bit */

Processor16 *and16_native_neon  = &logic16_neon<And>;
Processor16 *or16_native_neon   = &logic16_neon<Or>;
Processor16 *andn16_native_neon = &logic16_neon<Andn>;
Processor16 *xor16_native_neon  = &logic16_neon<Xor>;

#define DEFINE_NEON_VERSIONS(mode, Mode) \
    Processor16 *mode##_native_neon         = &logic16_neon<Mode<Nop, Nop>>; \
    Processor16 *mode##sub_native_neon      = &logic16_neon<Mode<Nop, Sub16>>; \
    Processor16 *mode##add_native_neon      = &logic16_neon<Mode<Nop, Add16>>; \
    Processor16 *sub##mode##_native_neon    = &logic16_neon<Mode<Sub16, Nop>>; \
    Processor16 *sub##mode##sub_native_neon = &logic16_neon<Mode<Sub16, Sub16>>; \
    Processor16 *sub##mode##add_native_neon = &logic16_neon<Mode<Sub16, Add16>>; \
    Processor16 *add##mode##_native_neon    = &logic16_neon<Mode<Add16, Nop>>; \
    Processor16 *add##mode##sub_native_neon = &logic16_neon<Mode<Add16, Sub16>>; \
    Processor16 *add##mode##add_native_neon = &logic16_neon<Mode<Add16, Add16>>;

DEFINE_NEON_VERSIONS(min, Min)
DEFINE_NEON_VERSIONS(max, Max)
#undef DEFINE_NEON_VERSIONS

/* float */

Processor32 *and_32_neon  = &logic32_neon<And>;
Processor32 *or_32_neon   = &logic32_neon<Or>;
Processor32 *andn_32_neon = &logic32_neon<Andn>;
Processor32 *xor_32_neon  = &logic32_neon<Xor>;

#define DEFINE_NEON_VERSIONS(mode, Mode) \
    Processor32 *mode##_32_neon         = &logic32_neon<Mode<Nop, Nop>>; \
    Processor32 *mode##sub_32_neon      = &logic32_neon<Mode<Nop, SubF>>; \
    Processor32 *mode##add_32_neon      = &logic32_neon<Mode<Nop, AddF>>; \
    Processor32 *sub##mode##_32_neon    = &logic32_neon<Mode<SubF, Nop>>; \
    Processor32 *sub##mode##sub_32_neon = &logic32_neon<Mode<SubF, SubF>>; \
    Processor32 *sub##mode##add_32_neon = &logic32_neon<Mode<SubF, AddF>>; \
    Processor32 *add##mode##_32_neon    = &logic32_neon<Mode<AddF, Nop>>; \
    Processor32 *add##mode##sub_32_neon = &logic32_neon<Mode<AddF, SubF>>; \
    Processor32 *add##mode##add_32_neon = &logic32_neon<Mode<AddF, AddF>>;

DEFINE_NEON_VERSIONS(min, Min)
DEFINE_NEON_VERSIONS(max, Max)
#undef DEFINE_NEON_VERSIONS

} } } }

#endif // NEON_INTRINSICS
