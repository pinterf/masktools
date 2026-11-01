#include "merge.h"

#ifdef INTEL_INTRINSICS

#include "merge_sse.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Merge {

   Processor *merge_sse2 = merge_sse2_t<CPU_SSE2>;
   Processor *merge_luma_420_sse2 = merge_luma_420_sse2_t<CPU_SSE2>;
   Processor *merge_luma_420_mpeg2_sse2 = merge_luma_420_mpeg2_sse2_t<CPU_SSE2>;
   Processor* merge_luma_420_topleft_sse2 = merge_luma_420_topleft_sse2_t<CPU_SSE2>;
   Processor *merge_luma_422_sse2 = merge_luma_422_sse2_t<CPU_SSE2>;
   Processor *merge_luma_422_mpeg2_sse2 = merge_luma_422_mpeg2_sse2_t<CPU_SSE2>;
   Processor *merge_luma_411_sse2 = merge_luma_411_sse2_t<CPU_SSE2>;

} } } }

#endif // INTEL_INTRINSICS
