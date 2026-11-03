#ifndef __Mt_merge_mask_H__
#define __Mt_merge_mask_H__

#include "merge.h"
#include <type_traits>

namespace Filtering { namespace MaskTools { namespace Filters { namespace Merge {

namespace {

template<typename T, bool left, bool top>
MT_FORCEINLINE T get_mask_410_c(const uint8_t *row, ptrdiff_t pitch, int x, int y)
{
  using Sum = typename std::conditional<std::is_same<T, float>::value, float, uint32_t>::type;
  constexpr int shift = (left ? 4 : 2) + (top ? 4 : 2);
  Sum sum = 0;
  // Centered: four equal taps. Co-sited: 1-2-3-4-3-2-1, centered on the first sample.
  // Positive offsets never exceed the current 4x4 block; only top/left need replication.
  for (int dy = top ? -3 : 0; dy <= 3; ++dy) {
    const int row_offset = std::max(y * 4 + dy, 0) - y * 4;
    const T *p = reinterpret_cast<const T*>(row + row_offset * pitch);
    Sum horizontal = 0;
    for (int dx = left ? -3 : 0; dx <= 3; ++dx) {
      const int weight = left ? 4 - std::abs(dx) : 1;
      horizontal += Sum(p[std::max(x * 4 + dx, 0)]) * weight;
    }
    sum += horizontal * (top ? 4 - std::abs(dy) : 1);
  }
  if constexpr (std::is_same<T, float>::value)
    return sum * (1.0f / (1 << shift));
  else
    // At 16 bit the largest sum is 65535 * 256, safely within uint32_t.
    return static_cast<T>((sum + (1u << (shift - 1))) >> shift);
}

} // namespace

} } } }

#endif // __Mt_merge_mask_H__
