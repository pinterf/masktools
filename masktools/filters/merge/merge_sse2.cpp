#include "merge.h"

#ifdef INTEL_INTRINSICS

#include "merge_sse.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Merge {

   Processor *merge_sse2 = merge_sse2_t<MemoryMode::SSE2_UNALIGNED, CPU_SSE2>;
   Processor *merge_asse2 = merge_sse2_t<MemoryMode::SSE2_ALIGNED, CPU_SSE2>;
   Processor *merge_luma_420_sse2 = merge_luma_420_sse2_t<MemoryMode::SSE2_UNALIGNED, CPU_SSE2>;
   Processor *merge_luma_420_asse2 = merge_luma_420_sse2_t<MemoryMode::SSE2_ALIGNED, CPU_SSE2>;
   Processor *merge_luma_420_mpeg2_sse2 = merge_luma_420_mpeg2_sse2_t<MemoryMode::SSE2_UNALIGNED, CPU_SSE2>;
   Processor *merge_luma_420_mpeg2_asse2 = merge_luma_420_mpeg2_sse2_t<MemoryMode::SSE2_ALIGNED, CPU_SSE2>;
   Processor* merge_luma_420_topleft_sse2 = merge_luma_420_topleft_sse2_t<MemoryMode::SSE2_UNALIGNED, CPU_SSE2>;
   Processor* merge_luma_420_topleft_asse2 = merge_luma_420_topleft_sse2_t<MemoryMode::SSE2_ALIGNED, CPU_SSE2>;
   Processor *merge_luma_422_sse2 = merge_luma_422_sse2_t<MemoryMode::SSE2_UNALIGNED, CPU_SSE2>;
   Processor *merge_luma_422_asse2 = merge_luma_422_sse2_t<MemoryMode::SSE2_ALIGNED, CPU_SSE2>;
   Processor *merge_luma_422_mpeg2_sse2 = merge_luma_422_mpeg2_sse2_t<MemoryMode::SSE2_UNALIGNED, CPU_SSE2>;
   Processor *merge_luma_422_mpeg2_asse2 = merge_luma_422_mpeg2_sse2_t<MemoryMode::SSE2_ALIGNED, CPU_SSE2>;
   Processor *merge_luma_411_sse2 = merge_luma_411_sse2_t<MemoryMode::SSE2_UNALIGNED, CPU_SSE2>;
   Processor *merge_luma_411_asse2 = merge_luma_411_sse2_t<MemoryMode::SSE2_ALIGNED, CPU_SSE2>;

} } } }

#endif // INTEL_INTRINSICS
