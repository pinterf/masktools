#include "edgemask.h"

#ifdef NEON_INTRINSICS

#include <arm_neon.h>

// mt_edge NEON: all modes (sobel, roberts, laplace, prewitt, half_prewitt, min/max (morpho), cartoon, custom
// convolution), 8, 10-16 bit and float; the C border handling (missing neighbors replaced by the center row /
// column). Bit-exact to C:
// - 8 bit in int16, 10-16 bit in int32 lanes; the convolution sum in int32, the division truncated as the C
//   integer division (in double: exact for |sum| < 2^31 and divisor <= 32767), so any divisor is supported;
// - float: the C evaluation order, unfused; abs, min, max as the C a < 0 ? -a : a, a < b ? a : b.

namespace Filtering { namespace MaskTools { namespace Filters { namespace Mask { namespace Edge {

namespace {

/* integer work vectors: int16x8_t (8 bit), int32x4_t (10-16 bit) */

MT_FORCEINLINE int16x8_t vadd(int16x8_t a, int16x8_t b) { return vaddq_s16(a, b); }
MT_FORCEINLINE int32x4_t vadd(int32x4_t a, int32x4_t b) { return vaddq_s32(a, b); }
MT_FORCEINLINE int16x8_t vsub(int16x8_t a, int16x8_t b) { return vsubq_s16(a, b); }
MT_FORCEINLINE int32x4_t vsub(int32x4_t a, int32x4_t b) { return vsubq_s32(a, b); }
MT_FORCEINLINE int16x8_t vabs(int16x8_t a) { return vabsq_s16(a); }
MT_FORCEINLINE int32x4_t vabs(int32x4_t a) { return vabsq_s32(a); }
MT_FORCEINLINE int16x8_t vmax(int16x8_t a, int16x8_t b) { return vmaxq_s16(a, b); }
MT_FORCEINLINE int32x4_t vmax(int32x4_t a, int32x4_t b) { return vmaxq_s32(a, b); }
MT_FORCEINLINE int16x8_t vmin(int16x8_t a, int16x8_t b) { return vminq_s16(a, b); }
MT_FORCEINLINE int32x4_t vmin(int32x4_t a, int32x4_t b) { return vminq_s32(a, b); }
template<int n> MT_FORCEINLINE int16x8_t vshl(int16x8_t a) { return vshlq_n_s16(a, n); }
template<int n> MT_FORCEINLINE int32x4_t vshl(int32x4_t a) { return vshlq_n_s32(a, n); }
template<int n> MT_FORCEINLINE int16x8_t vshr(int16x8_t a) { return vshrq_n_s16(a, n); }
template<int n> MT_FORCEINLINE int32x4_t vshr(int32x4_t a) { return vshrq_n_s32(a, n); }
MT_FORCEINLINE int16x8_t vdup(int v, int16x8_t) { return vdupq_n_s16(int16_t(v)); }
MT_FORCEINLINE int32x4_t vdup(int v, int32x4_t) { return vdupq_n_s32(v); }

// x <= lo ? 0 : x > hi ? max : x (the C threshold)
MT_FORCEINLINE int16x8_t vthreshold(int16x8_t x, int16x8_t lo, int16x8_t hi, int16x8_t mx) {
  int16x8_t r = vbslq_s16(vcgtq_s16(x, hi), mx, x);
  return vreinterpretq_s16_u16(vbicq_u16(vreinterpretq_u16_s16(r), vcleq_s16(x, lo)));
}
MT_FORCEINLINE int32x4_t vthreshold(int32x4_t x, int32x4_t lo, int32x4_t hi, int32x4_t mx) {
  int32x4_t r = vbslq_s32(vcgtq_s32(x, hi), mx, x);
  return vreinterpretq_s32_u32(vbicq_u32(vreinterpretq_u32_s32(r), vcleq_s32(x, lo)));
}
// val > 0 ? 0 : r
MT_FORCEINLINE int16x8_t vzero_if_positive(int16x8_t val, int16x8_t r) {
  return vreinterpretq_s16_u16(vbicq_u16(vreinterpretq_u16_s16(r), vcgtq_s16(val, vdupq_n_s16(0))));
}
MT_FORCEINLINE int32x4_t vzero_if_positive(int32x4_t val, int32x4_t r) {
  return vreinterpretq_s32_u32(vbicq_u32(vreinterpretq_u32_s32(r), vcgtq_s32(val, vdupq_n_s32(0))));
}

template<class W>
struct Params {
  W lo, hi, mx;
  int16_t m[9];
  double divisor;
};

// trunc(sum / divisor) for int32 lanes, as the C integer division
MT_FORCEINLINE int32x4_t vdiv_trunc(int32x4_t sum, double divisor) {
  const float64x2_t d = vdupq_n_f64(divisor);
  float64x2_t lo = vcvtq_f64_s64(vmovl_s32(vget_low_s32(sum)));
  float64x2_t hi = vcvtq_f64_s64(vmovl_high_s32(sum));
  int64x2_t qlo = vcvtq_s64_f64(vdivq_f64(lo, d)); // fcvtzs: toward zero
  int64x2_t qhi = vcvtq_s64_f64(vdivq_f64(hi, d));
  return vcombine_s32(vmovn_s64(qlo), vmovn_s64(qhi));
}

MT_FORCEINLINE int32x4_t conv_sum(int32x4_t a11, int32x4_t a21, int32x4_t a31, int32x4_t a12, int32x4_t a22, int32x4_t a32, int32x4_t a13, int32x4_t a23, int32x4_t a33, const int16_t *m) {
  int32x4_t s = vmulq_n_s32(a11, m[0]);
  s = vmlaq_n_s32(s, a21, m[1]);
  s = vmlaq_n_s32(s, a31, m[2]);
  s = vmlaq_n_s32(s, a12, m[3]);
  s = vmlaq_n_s32(s, a22, m[4]);
  s = vmlaq_n_s32(s, a32, m[5]);
  s = vmlaq_n_s32(s, a13, m[6]);
  s = vmlaq_n_s32(s, a23, m[7]);
  return vmlaq_n_s32(s, a33, m[8]);
}

/* operators on the work vectors; a<column><row> as in the C */

struct Sobel {
  template<class W> static MT_FORCEINLINE W op(W, W a21, W, W a12, W, W a32, W, W a23, W, const Params<W> &p) {
    return vthreshold(vshr<1>(vabs(vsub(vsub(vadd(a32, a23), a12), a21))), p.lo, p.hi, p.mx);
  }
};
struct Roberts {
  template<class W> static MT_FORCEINLINE W op(W, W, W, W, W a22, W a32, W, W a23, W, const Params<W> &p) {
    return vthreshold(vshr<1>(vabs(vsub(vsub(vshl<1>(a22), a32), a23))), p.lo, p.hi, p.mx);
  }
};
struct Laplace {
  template<class W> static MT_FORCEINLINE W op(W a11, W a21, W a31, W a12, W a22, W a32, W a13, W a23, W a33, const Params<W> &p) {
    W s = vadd(vadd(vadd(a32, a23), vadd(a11, a21)), vadd(vadd(a31, a12), vadd(a13, a33)));
    return vthreshold(vshr<3>(vabs(vsub(vshl<3>(a22), s))), p.lo, p.hi, p.mx);
  }
};
struct Morpho {
  template<class W> static MT_FORCEINLINE W op(W a11, W a21, W a31, W a12, W a22, W a32, W a13, W a23, W a33, const Params<W> &p) {
    W mx = vmax(vmax(vmax(a11, a21), vmax(a31, a12)), vmax(vmax(a22, a32), vmax(vmax(a13, a23), a33)));
    W mn = vmin(vmin(vmin(a11, a21), vmin(a31, a12)), vmin(vmin(a22, a32), vmin(vmin(a13, a23), a33)));
    return vthreshold(vsub(mx, mn), p.lo, p.hi, p.mx);
  }
};
struct Cartoon {
  template<class W> static MT_FORCEINLINE W op(W, W a21, W a31, W, W a22, W, W, W, W, const Params<W> &p) {
    W val = vsub(vsub(vshl<1>(a21), a22), a31);
    return vzero_if_positive(val, vthreshold(vsub(vdup(0, val), val), p.lo, p.hi, p.mx));
  }
};
struct Prewitt {
  template<class W> static MT_FORCEINLINE W op(W a11, W a21, W a31, W a12, W, W a32, W a13, W a23, W a33, const Params<W> &p) {
    W p90 = vsub(vadd(vadd(a11, a21), a31), vadd(vadd(a13, a23), a33));
    W p180 = vsub(vadd(vadd(a11, a12), a13), vadd(vadd(a31, a32), a33));
    W p45 = vsub(vadd(vadd(a12, a11), a21), vadd(vadd(a33, a32), a23));
    W p135 = vsub(vadd(vadd(a13, a12), a23), vadd(vadd(a31, a32), a21));
    W m = vmax(vmax(vabs(p90), vabs(p180)), vmax(vabs(p45), vabs(p135)));
    return vthreshold(m, p.lo, p.hi, p.mx);
  }
};
struct HalfPrewitt {
  template<class W> static MT_FORCEINLINE W op(W a11, W a21, W a31, W a12, W, W a32, W a13, W a23, W a33, const Params<W> &p) {
    W p90 = vsub(vadd(vadd(a11, vshl<1>(a21)), a31), vadd(vadd(a13, vshl<1>(a23)), a33));
    W p180 = vsub(vadd(vadd(a11, vshl<1>(a12)), a13), vadd(vadd(a31, vshl<1>(a32)), a33));
    return vthreshold(vmax(vabs(p90), vabs(p180)), p.lo, p.hi, p.mx);
  }
};
struct Convolution {
  // 10-16 bit: int32 lanes
  static MT_FORCEINLINE int32x4_t op(int32x4_t a11, int32x4_t a21, int32x4_t a31, int32x4_t a12, int32x4_t a22, int32x4_t a32, int32x4_t a13, int32x4_t a23, int32x4_t a33, const Params<int32x4_t> &p) {
    int32x4_t q = vdiv_trunc(conv_sum(a11, a21, a31, a12, a22, a32, a13, a23, a33, p.m), p.divisor);
    return vthreshold(vabsq_s32(q), p.lo, p.hi, p.mx);
  }
  // 8 bit: int16 lanes, widened to int32 for the sum
  static MT_FORCEINLINE int16x8_t op(int16x8_t a11, int16x8_t a21, int16x8_t a31, int16x8_t a12, int16x8_t a22, int16x8_t a32, int16x8_t a13, int16x8_t a23, int16x8_t a33, const Params<int16x8_t> &p) {
    Params<int32x4_t> p32;
    p32.lo = vmovl_s16(vget_low_s16(p.lo)); p32.hi = vmovl_s16(vget_low_s16(p.hi)); p32.mx = vmovl_s16(vget_low_s16(p.mx));
    for (int i = 0; i < 9; i++) p32.m[i] = p.m[i];
    p32.divisor = p.divisor;
#define LO(a) vmovl_s16(vget_low_s16(a))
#define HI(a) vmovl_high_s16(a)
    int32x4_t lo = op(LO(a11), LO(a21), LO(a31), LO(a12), LO(a22), LO(a32), LO(a13), LO(a23), LO(a33), p32);
    int32x4_t hi = op(HI(a11), HI(a21), HI(a31), HI(a12), HI(a22), HI(a32), HI(a13), HI(a23), HI(a33), p32);
#undef LO
#undef HI
    return vcombine_s16(vmovn_s32(lo), vmovn_s32(hi)); // thresholded: 0..255
  }
};

/* the 3x3 driver */

struct U8 {
  using T = uint8_t; using V = uint8x16_t; using W = int16x8_t; static constexpr int L = 16;
  static MT_FORCEINLINE V ld(const T *p) { return vld1q_u8(p); }
  static MT_FORCEINLINE void st(T *p, V v) { vst1q_u8(p, v); }
  static MT_FORCEINLINE V left_border(V c) { return vextq_u8(vdupq_laneq_u8(c, 0), c, 15); }
  static MT_FORCEINLINE V right_border(V c) { return vextq_u8(c, vdupq_laneq_u8(c, 15), 1); }
  static MT_FORCEINLINE W lo(V v) { return vreinterpretq_s16_u16(vmovl_u8(vget_low_u8(v))); }
  static MT_FORCEINLINE W hi(V v) { return vreinterpretq_s16_u16(vmovl_high_u8(v)); }
  static MT_FORCEINLINE V pack(W l, W h) { return vcombine_u8(vmovn_u16(vreinterpretq_u16_s16(l)), vmovn_u16(vreinterpretq_u16_s16(h))); }
};
struct U16 {
  using T = uint16_t; using V = uint16x8_t; using W = int32x4_t; static constexpr int L = 8;
  static MT_FORCEINLINE V ld(const T *p) { return vld1q_u16(p); }
  static MT_FORCEINLINE void st(T *p, V v) { vst1q_u16(p, v); }
  static MT_FORCEINLINE V left_border(V c) { return vextq_u16(vdupq_laneq_u16(c, 0), c, 7); }
  static MT_FORCEINLINE V right_border(V c) { return vextq_u16(c, vdupq_laneq_u16(c, 7), 1); }
  static MT_FORCEINLINE W lo(V v) { return vreinterpretq_s32_u32(vmovl_u16(vget_low_u16(v))); }
  static MT_FORCEINLINE W hi(V v) { return vreinterpretq_s32_u32(vmovl_high_u16(v)); }
  static MT_FORCEINLINE V pack(W l, W h) { return vcombine_u16(vmovn_u32(vreinterpretq_u32_s32(l)), vmovn_u32(vreinterpretq_u32_s32(h))); }
};

template<class Lane, class Op, bool leftBorder, bool rightBorder>
MT_FORCEINLINE void process_block(typename Lane::T *pDst, const typename Lane::T *pSrcp, const typename Lane::T *pSrc, const typename Lane::T *pSrcn, int x, const Params<typename Lane::W> &p)
{
  using V = typename Lane::V;
  V up = Lane::ld(pSrcp + x), mid = Lane::ld(pSrc + x), down = Lane::ld(pSrcn + x);
  V upl, midl, downl, upr, midr, downr;
  if constexpr (leftBorder) { upl = Lane::left_border(up); midl = Lane::left_border(mid); downl = Lane::left_border(down); }
  else { upl = Lane::ld(pSrcp + x - 1); midl = Lane::ld(pSrc + x - 1); downl = Lane::ld(pSrcn + x - 1); }
  if constexpr (rightBorder) { upr = Lane::right_border(up); midr = Lane::right_border(mid); downr = Lane::right_border(down); }
  else { upr = Lane::ld(pSrcp + x + 1); midr = Lane::ld(pSrc + x + 1); downr = Lane::ld(pSrcn + x + 1); }
  // a<column><row>: a11 up-left, a21 up, a31 up-right, a12 left, a22 center, ...
  auto rlo = Op::op(Lane::lo(upl), Lane::lo(up), Lane::lo(upr), Lane::lo(midl), Lane::lo(mid), Lane::lo(midr), Lane::lo(downl), Lane::lo(down), Lane::lo(downr), p);
  auto rhi = Op::op(Lane::hi(upl), Lane::hi(up), Lane::hi(upr), Lane::hi(midl), Lane::hi(mid), Lane::hi(midr), Lane::hi(downl), Lane::hi(down), Lane::hi(downr), p);
  Lane::st(pDst + x, Lane::pack(rlo, rhi));
}

// pitches in T units; nWidth >= L + 1 (constraint); pDst != pSrc
template<class Lane, class Op>
void generic_neon(typename Lane::T *pDst, ptrdiff_t nDstPitch, const typename Lane::T *pSrc, ptrdiff_t nSrcPitch, const Params<typename Lane::W> &p, int nWidth, int nHeight)
{
  using T = typename Lane::T;
  constexpr int L = Lane::L;
  for (int y = 0; y < nHeight; y++) {
    const T *pSrcp = pSrc + (y > 0 ? y - 1 : 0) * nSrcPitch;
    const T *pSrcc = pSrc + y * nSrcPitch;
    const T *pSrcn = pSrc + (y < nHeight - 1 ? y + 1 : y) * nSrcPitch;
    T *d = pDst + y * nDstPitch;
    process_block<Lane, Op, true, false>(d, pSrcp, pSrcc, pSrcn, 0, p);
    for (int x = L; x + L < nWidth; x += L)
      process_block<Lane, Op, false, false>(d, pSrcp, pSrcc, pSrcn, x, p);
    process_block<Lane, Op, false, true>(d, pSrcp, pSrcc, pSrcn, nWidth - L, p); // overlaps, recomputed from the source
  }
}

template<class W>
MT_FORCEINLINE Params<W> make_params(const int16_t matrix[10], int nLowThreshold, int nHighThreshold, int maxv)
{
  Params<W> p;
  p.lo = vdup(nLowThreshold, W());
  p.hi = vdup(nHighThreshold, W());
  p.mx = vdup(maxv, W());
  for (int i = 0; i < 9; i++) p.m[i] = matrix[i];
  p.divisor = matrix[9];
  return p;
}

// 8 bit: pitches in bytes
template<class Op>
void mask8_neon(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, const int16_t matrix[10], int nLowThreshold, int nHighThreshold, int nWidth, int nHeight)
{
  // thresholds are 0..255: x (0..2040) compares the same in int16
  generic_neon<U8, Op>(pDst, nDstPitch, pSrc, nSrcPitch, make_params<int16x8_t>(matrix, nLowThreshold, nHighThreshold, 255), nWidth, nHeight);
}

// 10-16 bit: pitches in bytes (as the C)
template<class Op, int bits_per_pixel>
void mask16_neon(uint16_t *pDst, ptrdiff_t nDstPitch, const uint16_t *pSrc, ptrdiff_t nSrcPitch, const int16_t matrix[10], int nLowThreshold, int nHighThreshold, int nWidth, int nHeight)
{
  generic_neon<U16, Op>(pDst, nDstPitch / 2, pSrc, nSrcPitch / 2, make_params<int32x4_t>(matrix, nLowThreshold, nHighThreshold, (1 << bits_per_pixel) - 1), nWidth, nHeight);
}

/* float: the C order */

MT_FORCEINLINE float32x4_t fabs_c(float32x4_t x) { return vbslq_f32(vcltq_f32(x, vdupq_n_f32(0.0f)), vnegq_f32(x), x); }
MT_FORCEINLINE float32x4_t fmax_c(float32x4_t a, float32x4_t b) { return vbslq_f32(vcgtq_f32(a, b), a, b); }
MT_FORCEINLINE float32x4_t fmin_c(float32x4_t a, float32x4_t b) { return vbslq_f32(vcltq_f32(a, b), a, b); }

struct ParamsF {
  float32x4_t lo, hi;
  float m[10];
};

// x <= lo ? 0 : x > hi ? 1 : x
MT_FORCEINLINE float32x4_t fthreshold(float32x4_t x, const ParamsF &p) {
  float32x4_t r = vbslq_f32(vcgtq_f32(x, p.hi), vdupq_n_f32(1.0f), x);
  return vreinterpretq_f32_u32(vbicq_u32(vreinterpretq_u32_f32(r), vcleq_f32(x, p.lo)));
}

#define FV float32x4_t
struct SobelF {
  static MT_FORCEINLINE FV op(FV, FV a21, FV, FV a12, FV, FV a32, FV, FV a23, FV, const ParamsF &p) {
    return fthreshold(vdivq_f32(fabs_c(vsubq_f32(vsubq_f32(vaddq_f32(a32, a23), a12), a21)), vdupq_n_f32(2.0f)), p);
  }
};
struct RobertsF {
  static MT_FORCEINLINE FV op(FV, FV, FV, FV, FV a22, FV a32, FV, FV a23, FV, const ParamsF &p) {
    FV v = vsubq_f32(vsubq_f32(vmulq_f32(a22, vdupq_n_f32(2.0f)), a32), a23);
    return fthreshold(vdivq_f32(fabs_c(v), vdupq_n_f32(2.0f)), p);
  }
};
struct LaplaceF {
  static MT_FORCEINLINE FV op(FV a11, FV a21, FV a31, FV a12, FV a22, FV a32, FV a13, FV a23, FV a33, const ParamsF &p) {
    FV v = vmulq_f32(a22, vdupq_n_f32(8.0f));
    v = vsubq_f32(v, a32); v = vsubq_f32(v, a23); v = vsubq_f32(v, a11); v = vsubq_f32(v, a21);
    v = vsubq_f32(v, a31); v = vsubq_f32(v, a12); v = vsubq_f32(v, a13); v = vsubq_f32(v, a33);
    return fthreshold(vdivq_f32(fabs_c(v), vdupq_n_f32(8.0f)), p);
  }
};
struct MorphoF {
  static MT_FORCEINLINE FV op(FV a11, FV a21, FV a31, FV a12, FV a22, FV a32, FV a13, FV a23, FV a33, const ParamsF &p) {
    FV mn = a11, mx = a11;
    mn = fmin_c(mn, a21); mx = fmax_c(mx, a21);
    mn = fmin_c(mn, a31); mx = fmax_c(mx, a31);
    mn = fmin_c(mn, a12); mx = fmax_c(mx, a12);
    mn = fmin_c(mn, a22); mx = fmax_c(mx, a22);
    mn = fmin_c(mn, a32); mx = fmax_c(mx, a32);
    mn = fmin_c(mn, a13); mx = fmax_c(mx, a13);
    mn = fmin_c(mn, a23); mx = fmax_c(mx, a23);
    mn = fmin_c(mn, a33); mx = fmax_c(mx, a33);
    return fthreshold(vsubq_f32(mx, mn), p);
  }
};
struct CartoonF {
  static MT_FORCEINLINE FV op(FV, FV a21, FV a31, FV, FV a22, FV, FV, FV, FV, const ParamsF &p) {
    FV val = vsubq_f32(vsubq_f32(vmulq_f32(a21, vdupq_n_f32(2.0f)), a22), a31);
    FV r = fthreshold(vnegq_f32(val), p);
    return vreinterpretq_f32_u32(vbicq_u32(vreinterpretq_u32_f32(r), vcgtq_f32(val, vdupq_n_f32(0.0f))));
  }
};
struct PrewittF {
  static MT_FORCEINLINE FV op(FV a11, FV a21, FV a31, FV a12, FV, FV a32, FV a13, FV a23, FV a33, const ParamsF &p) {
    FV p90 = vsubq_f32(vsubq_f32(vsubq_f32(vaddq_f32(vaddq_f32(a11, a21), a31), a13), a23), a33);
    FV p180 = vsubq_f32(vsubq_f32(vsubq_f32(vaddq_f32(vaddq_f32(a11, a12), a13), a31), a32), a33);
    FV p45 = vsubq_f32(vsubq_f32(vsubq_f32(vaddq_f32(vaddq_f32(a12, a11), a21), a33), a32), a23);
    FV p135 = vsubq_f32(vsubq_f32(vsubq_f32(vaddq_f32(vaddq_f32(a13, a12), a23), a31), a32), a21);
    FV max1 = fmax_c(fabs_c(p90), fabs_c(p180));
    FV max2 = fmax_c(fabs_c(p45), fabs_c(p135));
    return fthreshold(fmax_c(max1, max2), p);
  }
};
struct HalfPrewittF {
  static MT_FORCEINLINE FV op(FV a11, FV a21, FV a31, FV a12, FV, FV a32, FV a13, FV a23, FV a33, const ParamsF &p) {
    const FV two = vdupq_n_f32(2.0f);
    FV p90 = vsubq_f32(vsubq_f32(vsubq_f32(vaddq_f32(vaddq_f32(a11, vmulq_f32(two, a21)), a31), a13), vmulq_f32(two, a23)), a33);
    FV p180 = vsubq_f32(vsubq_f32(vsubq_f32(vaddq_f32(vaddq_f32(a11, vmulq_f32(two, a12)), a13), a31), vmulq_f32(two, a32)), a33);
    return fthreshold(fmax_c(fabs_c(p90), fabs_c(p180)), p);
  }
};
struct ConvolutionF {
  static MT_FORCEINLINE FV op(FV a11, FV a21, FV a31, FV a12, FV a22, FV a32, FV a13, FV a23, FV a33, const ParamsF &p) {
    FV s = vmulq_n_f32(a11, p.m[0]);
    s = vaddq_f32(s, vmulq_n_f32(a21, p.m[1]));
    s = vaddq_f32(s, vmulq_n_f32(a31, p.m[2]));
    s = vaddq_f32(s, vmulq_n_f32(a12, p.m[3]));
    s = vaddq_f32(s, vmulq_n_f32(a22, p.m[4]));
    s = vaddq_f32(s, vmulq_n_f32(a32, p.m[5]));
    s = vaddq_f32(s, vmulq_n_f32(a13, p.m[6]));
    s = vaddq_f32(s, vmulq_n_f32(a23, p.m[7]));
    s = vaddq_f32(s, vmulq_n_f32(a33, p.m[8]));
    return fthreshold(fabs_c(vdivq_f32(s, vdupq_n_f32(p.m[9]))), p);
  }
};
#undef FV

template<class Op, bool leftBorder, bool rightBorder>
MT_FORCEINLINE void process_block_f(float *pDst, const float *pSrcp, const float *pSrc, const float *pSrcn, int x, const ParamsF &p)
{
  float32x4_t up = vld1q_f32(pSrcp + x), mid = vld1q_f32(pSrc + x), down = vld1q_f32(pSrcn + x);
  float32x4_t upl, midl, downl, upr, midr, downr;
  if constexpr (leftBorder) {
    upl = vextq_f32(vdupq_laneq_f32(up, 0), up, 3); midl = vextq_f32(vdupq_laneq_f32(mid, 0), mid, 3); downl = vextq_f32(vdupq_laneq_f32(down, 0), down, 3);
  }
  else { upl = vld1q_f32(pSrcp + x - 1); midl = vld1q_f32(pSrc + x - 1); downl = vld1q_f32(pSrcn + x - 1); }
  if constexpr (rightBorder) {
    upr = vextq_f32(up, vdupq_laneq_f32(up, 3), 1); midr = vextq_f32(mid, vdupq_laneq_f32(mid, 3), 1); downr = vextq_f32(down, vdupq_laneq_f32(down, 3), 1);
  }
  else { upr = vld1q_f32(pSrcp + x + 1); midr = vld1q_f32(pSrc + x + 1); downr = vld1q_f32(pSrcn + x + 1); }
  vst1q_f32(pDst + x, Op::op(upl, up, upr, midl, mid, midr, downl, down, downr, p));
}

// pitches in bytes (as the C); nWidth >= 5
template<class Op>
void mask32_neon(float *pDst, ptrdiff_t nDstPitch, const float *pSrc, ptrdiff_t nSrcPitch, const float matrix[10], float nLowThreshold, float nHighThreshold, int nWidth, int nHeight)
{
  constexpr int L = 4;
  ParamsF p;
  p.lo = vdupq_n_f32(nLowThreshold);
  p.hi = vdupq_n_f32(nHighThreshold);
  for (int i = 0; i < 10; i++) p.m[i] = matrix[i];
  nDstPitch /= sizeof(float);
  nSrcPitch /= sizeof(float);
  for (int y = 0; y < nHeight; y++) {
    const float *pSrcp = pSrc + (y > 0 ? y - 1 : 0) * nSrcPitch;
    const float *pSrcc = pSrc + y * nSrcPitch;
    const float *pSrcn = pSrc + (y < nHeight - 1 ? y + 1 : y) * nSrcPitch;
    float *d = pDst + y * nDstPitch;
    process_block_f<Op, true, false>(d, pSrcp, pSrcc, pSrcn, 0, p);
    for (int x = L; x + L < nWidth; x += L)
      process_block_f<Op, false, false>(d, pSrcp, pSrcc, pSrcn, x, p);
    process_block_f<Op, false, true>(d, pSrcp, pSrcc, pSrcn, nWidth - L, p);
  }
}

} // namespace

#define DEFINE_NEON_VERSIONS(name, Op) \
Processor *name##_neon = &mask8_neon<Op>; \
Processor16 *name##_10_neon = &mask16_neon<Op, 10>; \
Processor16 *name##_12_neon = &mask16_neon<Op, 12>; \
Processor16 *name##_14_neon = &mask16_neon<Op, 14>; \
Processor16 *name##_16_neon = &mask16_neon<Op, 16>; \
Processor32 *name##_32_neon = &mask32_neon<Op##F>;

DEFINE_NEON_VERSIONS(sobel, Sobel)
DEFINE_NEON_VERSIONS(roberts, Roberts)
DEFINE_NEON_VERSIONS(laplace, Laplace)
DEFINE_NEON_VERSIONS(prewitt, Prewitt)
DEFINE_NEON_VERSIONS(half_prewitt, HalfPrewitt)
DEFINE_NEON_VERSIONS(convolution, Convolution)
DEFINE_NEON_VERSIONS(morpho, Morpho)
DEFINE_NEON_VERSIONS(cartoon, Cartoon)
#undef DEFINE_NEON_VERSIONS

} } } } }

#endif // NEON_INTRINSICS
