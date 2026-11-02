#include "merge.h"

#ifdef INTEL_INTRINSICS

#include "merge16_sse.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Merge {


Processor16 *merge16_sse2 = merge16_simd<CPU_SSE2, MASK444, merge16_t_c<MASK444>, merge16_t_c<MASK444>>;
Processor16 *merge16_luma_420_sse2 = merge16_simd<CPU_SSE2, MASK420, merge16_t_c<MASK420>, merge16_t_c<MASK420>>;
Processor16 *merge16_luma_420_mpeg2_sse2 = merge16_simd<CPU_SSE2, MASK420_MPEG2, merge16_t_c<MASK420_MPEG2>, merge16_t_allow_leftminus1_c<MASK420_MPEG2>>;
Processor16 *merge16_luma_420_topleft_sse2 = merge16_simd<CPU_SSE2, MASK420_TOPLEFT, merge16_t_c<MASK420_TOPLEFT>, merge16_t_allow_leftminus1_c<MASK420_TOPLEFT>>;
Processor16 *merge16_luma_422_sse2 = merge16_simd<CPU_SSE2, MASK422, merge16_t_c<MASK422>, merge16_t_c<MASK422>>;
Processor16 *merge16_luma_422_mpeg2_sse2 = merge16_simd<CPU_SSE2, MASK422_MPEG2, merge16_t_c<MASK422_MPEG2>, merge16_t_allow_leftminus1_c<MASK422_MPEG2>>;

} } } }

#endif // INTEL_INTRINSICS
