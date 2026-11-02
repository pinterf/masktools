#include "merge.h"

#ifdef INTEL_INTRINSICS

#include "merge16_sse.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Merge {


Processor16 *merge16_luma_420_ssse3 = merge16_simd<CPU_SSSE3, MASK420, merge16_t_c<MASK420>, merge16_t_c<MASK420>>;
Processor16 *merge16_luma_420_mpeg2_ssse3 = merge16_simd<CPU_SSSE3, MASK420_MPEG2, merge16_t_c<MASK420_MPEG2>, merge16_t_allow_leftminus1_c<MASK420_MPEG2>>;
Processor16 *merge16_luma_422_ssse3 = merge16_simd<CPU_SSSE3, MASK422, merge16_t_c<MASK422>, merge16_t_c<MASK422>>;
Processor16 *merge16_luma_422_mpeg2_ssse3 = merge16_simd<CPU_SSSE3, MASK422_MPEG2, merge16_t_c<MASK422_MPEG2>, merge16_t_allow_leftminus1_c<MASK422_MPEG2>>;

} } } }

#endif // INTEL_INTRINSICS
