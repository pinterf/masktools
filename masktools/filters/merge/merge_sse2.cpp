#include "merge.h"

#ifdef INTEL_INTRINSICS

#include "merge_sse.h"
#include "merge_410_simd.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Merge {

   Processor *merge_sse2 = merge_sse2_t;
   Processor *merge_luma_420_sse2 = merge_luma_420_sse2_t;
   Processor *merge_luma_420_mpeg2_sse2 = merge_luma_420_mpeg2_sse2_t;
   Processor* merge_luma_420_topleft_sse2 = merge_luma_420_topleft_sse2_t;
   Processor *merge_luma_422_sse2 = merge_luma_422_sse2_t;
   Processor *merge_luma_422_mpeg2_sse2 = merge_luma_422_mpeg2_sse2_t;
   Processor *merge_luma_411_sse2 = merge_411_t<uint8_t, false, Mask410SSE, merge_sse2_t>;
   Processor *merge_luma_411_mpeg2_sse2 = merge_411_t<uint8_t, true, Mask410SSE, merge_sse2_t>;
   Processor *merge_luma_440_sse2 = merge_luma_440_sse2_t<false>;
   Processor *merge_luma_440_topleft_sse2 = merge_luma_440_sse2_t<true>;
   Processor *merge_luma_410_sse2 = merge_410_t<uint8_t, false, false, Mask410SSE, merge_sse2_t>;
   Processor *merge_luma_410_mpeg2_sse2 = merge_410_t<uint8_t, true, false, Mask410SSE, merge_sse2_t>;
   Processor *merge_luma_410_topleft_sse2 = merge_410_t<uint8_t, true, true, Mask410SSE, merge_sse2_t>;

} } } }

#endif // INTEL_INTRINSICS
