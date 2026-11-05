#ifndef __Mt_merge_mask_H__
#define __Mt_merge_mask_H__

#include "merge.h"
#include <algorithm>
#include <type_traits>

namespace Filtering { namespace MaskTools { namespace Filters { namespace Merge {

namespace {

// Low-pass filters of the 4:1 subsampled axes, positioned by the chroma siting. Offsets are luma positions
// relative to the first luma sample of the chroma block:
//   centred  : triangle 1 3 5 7 7 5 3 1 / 32, offsets -2..5
//   co-sited : tent     1 2 3 4 3 2 1   / 16, offsets -3..3
// Evaluated as cascades of box sums, vertical first, additions only, in a fixed order: the float results do not
// depend on how the weights could be multiplied, and there is nothing to contract into FMA.
enum MaskFilter { MF_ID, MF_TENT7, MF_TRI8 };

template<MaskFilter F> constexpr int mask_filter_first = F == MF_TENT7 ? -3 : F == MF_TRI8 ? -2 : 0;
template<MaskFilter F> constexpr int mask_filter_taps = F == MF_TENT7 ? 7 : F == MF_TRI8 ? 8 : 1;
template<MaskFilter F> constexpr int mask_filter_log2 = F == MF_TENT7 ? 4 : F == MF_TRI8 ? 5 : 0;
template<MaskFilter F> constexpr int mask_filter_ratio = F == MF_ID ? 1 : 4;

template<MaskFilter F, typename Sum>
MT_FORCEINLINE Sum mask_filter_sum(const Sum *v)
{
  if constexpr (F == MF_ID) {
    return v[0];
  }
  else if constexpr (F == MF_TENT7) {
    const Sum s0 = v[0] + v[1] + v[2] + v[3];
    const Sum s1 = v[1] + v[2] + v[3] + v[4];
    const Sum s2 = v[2] + v[3] + v[4] + v[5];
    const Sum s3 = v[3] + v[4] + v[5] + v[6];
    return s0 + s1 + s2 + s3;
  }
  else { // the triangle is a tent over v[0..6] plus a tent over v[1..7]
    return mask_filter_sum<MF_TENT7, Sum>(v) + mask_filter_sum<MF_TENT7, Sum>(v + 1);
  }
}

// Chroma mask at x, y for 4:1:0 and 4:1:1. row: the first mask row of the chroma row y.
// width, height: size of the processed chroma window, the filters are clamped to its luma pixels.
template<typename T, MaskFilter H, MaskFilter V>
MT_FORCEINLINE T get_mask_filtered_c(const uint8_t *row, ptrdiff_t pitch, int x, int y, int width, int height)
{
  using Sum = typename std::conditional<std::is_same<T, float>::value, float, uint32_t>::type;
  constexpr int shift = mask_filter_log2<H> + mask_filter_log2<V>;
  constexpr int v_ratio = mask_filter_ratio<V>;
  constexpr int h_ratio = mask_filter_ratio<H>;
  const int last_row = height * v_ratio - 1;
  const int last_column = width * h_ratio - 1;

  const T *rows[8];
  for (int j = 0; j < mask_filter_taps<V>; ++j) {
    const int luma_y = std::min(std::max(y * v_ratio + mask_filter_first<V> + j, 0), last_row);
    rows[j] = reinterpret_cast<const T*>(row + (luma_y - y * v_ratio) * pitch);
  }
  // At 16 bit the largest sum is 65535 * 1024, safely within uint32_t.
  Sum columns[8];
  for (int k = 0; k < mask_filter_taps<H>; ++k) {
    const int luma_x = std::min(std::max(x * h_ratio + mask_filter_first<H> + k, 0), last_column);
    Sum values[8];
    for (int j = 0; j < mask_filter_taps<V>; ++j)
      values[j] = Sum(rows[j][luma_x]);
    columns[k] = mask_filter_sum<V, Sum>(values);
  }
  const Sum sum = mask_filter_sum<H, Sum>(columns);
  if constexpr (std::is_same<T, float>::value)
    return sum * (1.0f / (1 << shift));
  else
    return static_cast<T>((sum + (1u << (shift - 1))) >> shift);
}

} // namespace

} } } }

#endif // __Mt_merge_mask_H__
