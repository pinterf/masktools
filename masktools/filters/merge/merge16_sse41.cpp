#include "merge.h"

#ifdef INTEL_INTRINSICS

#include "merge16_sse.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Merge {


Processor16 *merge16_sse4_1 = merge16_simd<CPU_SSE4_1, MASK444, merge16_t_c<MASK444>, merge16_t_c<MASK444>>;
Processor16 *merge16_luma_420_sse4_1 = merge16_simd<CPU_SSE4_1, MASK420, merge16_t_c<MASK420>, merge16_t_c<MASK420>>;
Processor16 *merge16_luma_420_mpeg2_sse4_1 = merge16_simd<CPU_SSE4_1, MASK420_MPEG2, merge16_t_c<MASK420_MPEG2>, merge16_t_allow_leftminus1_c<MASK420_MPEG2>>;
Processor16 *merge16_luma_420_topleft_sse4_1 = merge16_simd<CPU_SSE4_1, MASK420_TOPLEFT, merge16_t_c<MASK420_TOPLEFT>, merge16_t_allow_leftminus1_c<MASK420_TOPLEFT>>;
Processor16 *merge16_luma_422_sse4_1 = merge16_simd<CPU_SSE4_1, MASK422, merge16_t_c<MASK422>, merge16_t_c<MASK422>>;
Processor16 *merge16_luma_422_mpeg2_sse4_1 = merge16_simd<CPU_SSE4_1, MASK422_MPEG2, merge16_t_c<MASK422_MPEG2>, merge16_t_allow_leftminus1_c<MASK422_MPEG2>>;

} } } }

#endif // INTEL_INTRINSICS
