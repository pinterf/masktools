#include "merge.h"

#ifdef INTEL_INTRINSICS

#include "merge16_sse.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Merge {


#define MAKE_EXPORTS(bits_per_pixel) \
Processor16 *merge16_##bits_per_pixel##_sse2 = merge16_t_simd<CPU_SSE2, MASK444, merge16_t_c<MASK444, bits_per_pixel>, merge16_t_c<MASK444, bits_per_pixel>, bits_per_pixel>; \
Processor16 *merge16_luma_420_##bits_per_pixel##_sse2 = merge16_t_simd<CPU_SSE2, MASK420, merge16_t_c<MASK420, bits_per_pixel>, merge16_t_c<MASK420, bits_per_pixel>, bits_per_pixel>; \
Processor16 *merge16_luma_420_mpeg2_##bits_per_pixel##_sse2 = merge16_t_simd<CPU_SSE2, MASK420_MPEG2, merge16_t_c<MASK420_MPEG2, bits_per_pixel>, merge16_t_allow_leftminus1_c<MASK420_MPEG2, bits_per_pixel>, bits_per_pixel>; \
Processor16 *merge16_luma_420_topleft_##bits_per_pixel##_sse2 = merge16_t_simd<CPU_SSE2, MASK420_TOPLEFT, merge16_t_c<MASK420_TOPLEFT, bits_per_pixel>, merge16_t_allow_leftminus1_c<MASK420_TOPLEFT, bits_per_pixel>, bits_per_pixel>; \
Processor16 *merge16_luma_422_##bits_per_pixel##_sse2 = merge16_t_simd<CPU_SSE2, MASK422, merge16_t_c<MASK422, bits_per_pixel>, merge16_t_c<MASK422, bits_per_pixel>, bits_per_pixel>; \
Processor16 *merge16_luma_422_mpeg2_##bits_per_pixel##_sse2 = merge16_t_simd<CPU_SSE2, MASK422_MPEG2, merge16_t_c<MASK422_MPEG2, bits_per_pixel>, merge16_t_allow_leftminus1_c<MASK422_MPEG2, bits_per_pixel>, bits_per_pixel>;

MAKE_EXPORTS(10)
MAKE_EXPORTS(12)
MAKE_EXPORTS(14)
MAKE_EXPORTS(16)
#undef MAKE_EXPORTS

} } } }

#endif // INTEL_INTRINSICS
