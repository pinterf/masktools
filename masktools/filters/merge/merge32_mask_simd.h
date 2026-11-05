#ifndef __Mt_merge32_mask_SIMD_H__
#define __Mt_merge32_mask_SIMD_H__

#include "merge_410_simd.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Merge {

namespace {

template<template<typename> class Ops>
void merge32_luma_420_topleft_simd(uint8_t *dst, ptrdiff_t dst_pitch, const uint8_t *src, ptrdiff_t src_pitch,
  const uint8_t *mask, ptrdiff_t mask_pitch, int width, int height)
{
  using OpsFloat = Ops<float>;
  for (int y = 0; y < height; ++y) {
    const float *mid = reinterpret_cast<const float*>(mask);
    const float *above = reinterpret_cast<const float*>(y == 0 ? mask : mask - mask_pitch);
    const float *below = reinterpret_cast<const float*>(mask + mask_pitch);
    auto columns = [&](int x, typename OpsFloat::V &even, typename OpsFloat::V &odd) {
      typename OpsFloat::V a, b, c, d;
      OpsFloat::pairs(above + x, a, b);
      OpsFloat::pairs(mid + x, even, odd);
      OpsFloat::pairs(below + x, c, d);
      even = OpsFloat::add(OpsFloat::add(a, OpsFloat::template mul<2>(even)), c);
      odd = OpsFloat::add(OpsFloat::add(b, OpsFloat::template mul<2>(odd)), d);
    };
    float *d = reinterpret_cast<float*>(dst);
    const float *s = reinterpret_cast<const float*>(src);
    auto scalar = [&](int x) {
      auto column = [&](int i) { return (above[i] + mid[i] * 2.0f) + below[i]; };
      const float m = ((column(std::max(x * 2 - 1, 0)) + 2.0f * column(x * 2)) + column(x * 2 + 1)) * 0.0625f;
      d[x] = (1.0f - m) * d[x] + m * s[x];
    };
    int x = 0;
    // Only the first output needs left replication; retain full row coordinates for the tail.
    if (width > 0) scalar(x++);
    for (; x + OpsFloat::lanes <= width; x += OpsFloat::lanes) {
      typename OpsFloat::V center, right, unused, left;
      columns(x * 2, center, right);
      columns(x * 2 - 2, unused, left);
      auto m = OpsFloat::template normalize<4>(OpsFloat::add(OpsFloat::add(left, OpsFloat::template mul<2>(center)), right));
      alignas(32) float values[OpsFloat::lanes];
      OpsFloat::store(values, m);
      OpsFloat::blend(d + x, s + x, values);
    }
    for (; x < width; ++x) scalar(x);
    dst += dst_pitch; src += src_pitch; mask += mask_pitch * 2;
  }
}

} // namespace

} } } }

#endif // __Mt_merge32_mask_SIMD_H__
