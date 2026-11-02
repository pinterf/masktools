#ifndef __Mt_functions_neon_H__
#define __Mt_functions_neon_H__

// Morphologic filters NEON (aarch64): one 3x3 pass for every mode, the C border handling (missing neighbors
// replaced by the center row / column), bit-exact to the C versions. Included only from *_neon.cpp files.

#include "../../../common/utils/utils.h"
#include "../../common/common.h"
#include <arm_neon.h>
#include <algorithm>

namespace Filtering { namespace MaskTools { namespace Filters { namespace Morphologic {
// Anonymous namespace: this header is included from files built with different per file compiler flags.
namespace {

/* lane type helpers */

MT_FORCEINLINE uint8x16_t ld(const uint8_t *p) { return vld1q_u8(p); }
MT_FORCEINLINE uint16x8_t ld(const uint16_t *p) { return vld1q_u16(p); }
MT_FORCEINLINE void st(uint8_t *p, uint8x16_t v) { vst1q_u8(p, v); }
MT_FORCEINLINE void st(uint16_t *p, uint16x8_t v) { vst1q_u16(p, v); }
MT_FORCEINLINE uint8x16_t vmax(uint8x16_t a, uint8x16_t b) { return vmaxq_u8(a, b); }
MT_FORCEINLINE uint16x8_t vmax(uint16x8_t a, uint16x8_t b) { return vmaxq_u16(a, b); }
MT_FORCEINLINE uint8x16_t vmin(uint8x16_t a, uint8x16_t b) { return vminq_u8(a, b); }
MT_FORCEINLINE uint16x8_t vmin(uint16x8_t a, uint16x8_t b) { return vminq_u16(a, b); }
MT_FORCEINLINE uint8x16_t vqadd(uint8x16_t a, uint8x16_t b) { return vqaddq_u8(a, b); }
MT_FORCEINLINE uint16x8_t vqadd(uint16x8_t a, uint16x8_t b) { return vqaddq_u16(a, b); }
MT_FORCEINLINE uint8x16_t vqsub(uint8x16_t a, uint8x16_t b) { return vqsubq_u8(a, b); }
MT_FORCEINLINE uint16x8_t vqsub(uint16x8_t a, uint16x8_t b) { return vqsubq_u16(a, b); }
MT_FORCEINLINE uint8x16_t vdup(uint8_t v, uint8x16_t) { return vdupq_n_u8(v); }
MT_FORCEINLINE uint16x8_t vdup(uint16_t v, uint16x8_t) { return vdupq_n_u16(v); }
// [c0, c0, c1, ...]: the left neighbors at the left border
MT_FORCEINLINE uint8x16_t left_border(uint8x16_t c) { return vextq_u8(vdupq_laneq_u8(c, 0), c, 15); }
MT_FORCEINLINE uint16x8_t left_border(uint16x8_t c) { return vextq_u16(vdupq_laneq_u16(c, 0), c, 7); }
// [c1, ..., cN-1, cN-1]: the right neighbors at the right border
MT_FORCEINLINE uint8x16_t right_border(uint8x16_t c) { return vextq_u8(c, vdupq_laneq_u8(c, 15), 1); }
MT_FORCEINLINE uint16x8_t right_border(uint16x8_t c) { return vextq_u16(c, vdupq_laneq_u16(c, 7), 1); }

// (sum of the 8 neighbors) >> 3
MT_FORCEINLINE uint8x16_t mean8(uint8x16_t a1, uint8x16_t a2, uint8x16_t a3, uint8x16_t a4, uint8x16_t a6, uint8x16_t a7, uint8x16_t a8, uint8x16_t a9)
{
  uint16x8_t lo = vaddq_u16(vaddq_u16(vaddl_u8(vget_low_u8(a1), vget_low_u8(a2)), vaddl_u8(vget_low_u8(a3), vget_low_u8(a4))),
                            vaddq_u16(vaddl_u8(vget_low_u8(a6), vget_low_u8(a7)), vaddl_u8(vget_low_u8(a8), vget_low_u8(a9))));
  uint16x8_t hi = vaddq_u16(vaddq_u16(vaddl_high_u8(a1, a2), vaddl_high_u8(a3, a4)),
                            vaddq_u16(vaddl_high_u8(a6, a7), vaddl_high_u8(a8, a9)));
  return vcombine_u8(vshrn_n_u16(lo, 3), vshrn_n_u16(hi, 3));
}

MT_FORCEINLINE uint16x8_t mean8(uint16x8_t a1, uint16x8_t a2, uint16x8_t a3, uint16x8_t a4, uint16x8_t a6, uint16x8_t a7, uint16x8_t a8, uint16x8_t a9)
{
  uint32x4_t lo = vaddq_u32(vaddq_u32(vaddl_u16(vget_low_u16(a1), vget_low_u16(a2)), vaddl_u16(vget_low_u16(a3), vget_low_u16(a4))),
                            vaddq_u32(vaddl_u16(vget_low_u16(a6), vget_low_u16(a7)), vaddl_u16(vget_low_u16(a8), vget_low_u16(a9))));
  uint32x4_t hi = vaddq_u32(vaddq_u32(vaddl_high_u16(a1, a2), vaddl_high_u16(a3, a4)),
                            vaddq_u32(vaddl_high_u16(a6, a7), vaddl_high_u16(a8, a9)));
  return vcombine_u16(vshrn_n_u32(lo, 3), vshrn_n_u32(hi, 3));
}

/* operators: a1..a9 the 3x3 neighborhood row by row, a5 the center; the C thresholding:
   up:   if (r - a5 > dev) r = a5 + dev   (r >= a5)
   down: if (a5 - r > dev) r = a5 - dev   (r <= a5) */

struct ExpandSquare {
  template<class V> static MT_FORCEINLINE V op(V a1, V a2, V a3, V a4, V a5, V a6, V a7, V a8, V a9, V dev) {
    V r = vmax(vmax(vmax(a1, a2), vmax(a3, a4)), vmax(vmax(a5, a6), vmax(vmax(a7, a8), a9)));
    return vmin(r, vqadd(a5, dev));
  }
};
struct ExpandBoth {
  template<class V> static MT_FORCEINLINE V op(V, V a2, V, V a4, V a5, V a6, V, V a8, V, V dev) {
    V r = vmax(vmax(vmax(a2, a4), vmax(a6, a8)), a5);
    return vmin(r, vqadd(a5, dev));
  }
};
struct ExpandHorizontal {
  template<class V> static MT_FORCEINLINE V op(V, V, V, V a4, V a5, V a6, V, V, V, V dev) {
    return vmin(vmax(vmax(a4, a6), a5), vqadd(a5, dev));
  }
};
struct ExpandVertical {
  template<class V> static MT_FORCEINLINE V op(V, V a2, V, V, V a5, V, V, V a8, V, V dev) {
    return vmin(vmax(vmax(a2, a8), a5), vqadd(a5, dev));
  }
};
struct InpandSquare {
  template<class V> static MT_FORCEINLINE V op(V a1, V a2, V a3, V a4, V a5, V a6, V a7, V a8, V a9, V dev) {
    V r = vmin(vmin(vmin(a1, a2), vmin(a3, a4)), vmin(vmin(a5, a6), vmin(vmin(a7, a8), a9)));
    return vmax(r, vqsub(a5, dev));
  }
};
struct InpandBoth {
  template<class V> static MT_FORCEINLINE V op(V, V a2, V, V a4, V a5, V a6, V, V a8, V, V dev) {
    V r = vmin(vmin(vmin(a2, a4), vmin(a6, a8)), a5);
    return vmax(r, vqsub(a5, dev));
  }
};
struct InpandHorizontal {
  template<class V> static MT_FORCEINLINE V op(V, V, V, V a4, V a5, V a6, V, V, V, V dev) {
    return vmax(vmin(vmin(a4, a6), a5), vqsub(a5, dev));
  }
};
struct InpandVertical {
  template<class V> static MT_FORCEINLINE V op(V, V a2, V, V, V a5, V, V, V a8, V, V dev) {
    return vmax(vmin(vmin(a2, a8), a5), vqsub(a5, dev));
  }
};
// inflate: max(mean of the 8 neighbors, center), deflate: min
struct InflateOp {
  template<class V> static MT_FORCEINLINE V op(V a1, V a2, V a3, V a4, V a5, V a6, V a7, V a8, V a9, V dev) {
    return vmin(vmax(mean8(a1, a2, a3, a4, a6, a7, a8, a9), a5), vqadd(a5, dev));
  }
};
struct DeflateOp {
  template<class V> static MT_FORCEINLINE V op(V a1, V a2, V a3, V a4, V a5, V a6, V a7, V a8, V a9, V dev) {
    return vmax(vmin(mean8(a1, a2, a3, a4, a6, a7, a8, a9), a5), vqsub(a5, dev));
  }
};

template<class Op, bool leftBorder, bool rightBorder, class T, class V>
MT_FORCEINLINE void process_block(T *pDst, const T *pSrcp, const T *pSrc, const T *pSrcn, int x, V dev)
{
  V up = ld(pSrcp + x), mid = ld(pSrc + x), down = ld(pSrcn + x);
  V upl, midl, downl, upr, midr, downr;
  if constexpr (leftBorder) { upl = left_border(up); midl = left_border(mid); downl = left_border(down); }
  else { upl = ld(pSrcp + x - 1); midl = ld(pSrc + x - 1); downl = ld(pSrcn + x - 1); }
  if constexpr (rightBorder) { upr = right_border(up); midr = right_border(mid); downr = right_border(down); }
  else { upr = ld(pSrcp + x + 1); midr = ld(pSrc + x + 1); downr = ld(pSrcn + x + 1); }
  st(pDst + x, Op::op(upl, up, upr, midl, mid, midr, downl, down, downr, dev));
}

// pitches in T units; nWidth >= 16 / sizeof(T) + 1 (constraint); pDst != pSrc
template<class Op, class T>
void generic_neon(T *pDst, ptrdiff_t nDstPitch, const T *pSrc, ptrdiff_t nSrcPitch, int nMaxDeviation, int nWidth, int nHeight)
{
  using V = decltype(ld(pSrc));
  constexpr int L = 16 / sizeof(T);
  const V dev = vdup(static_cast<T>(std::min(std::max(nMaxDeviation, 0), int(T(~T(0))))), V());
  for (int y = 0; y < nHeight; y++) {
    const T *pSrcp = pSrc + (y > 0 ? y - 1 : 0) * nSrcPitch;
    const T *pSrcc = pSrc + y * nSrcPitch;
    const T *pSrcn = pSrc + (y < nHeight - 1 ? y + 1 : y) * nSrcPitch;
    T *d = pDst + y * nDstPitch;
    process_block<Op, true, false>(d, pSrcp, pSrcc, pSrcn, 0, dev);
    for (int x = L; x + L < nWidth; x += L)
      process_block<Op, false, false>(d, pSrcp, pSrcc, pSrcn, x, dev);
    process_block<Op, false, true>(d, pSrcp, pSrcc, pSrcn, nWidth - L, dev); // overlaps, recomputed from the source
  }
}

template<class Op>
void morpho8_neon(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int nMaxDeviation, const int *pCoordinates, int nCoordinates, int nWidth, int nHeight)
{
  UNUSED(pCoordinates); UNUSED(nCoordinates);
  generic_neon<Op, uint8_t>(pDst, nDstPitch, pSrc, nSrcPitch, nMaxDeviation, nWidth, nHeight);
}

template<class Op>
void morpho16_neon(uint16_t *pDst, ptrdiff_t nDstPitch, const uint16_t *pSrc, ptrdiff_t nSrcPitch, int nMaxDeviation, const int *pCoordinates, int nCoordinates, int nWidth, int nHeight)
{
  UNUSED(pCoordinates); UNUSED(nCoordinates);
  generic_neon<Op, uint16_t>(pDst, nDstPitch, pSrc, nSrcPitch, nMaxDeviation, nWidth, nHeight);
}

} // namespace

} } } } // namespace Morphologic, Filters, MaskTools, Filtering

#endif // __Mt_functions_neon_H__
