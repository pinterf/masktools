#include "merge_c.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Merge {

Processor *merge_c = merge_t_c;
Processor *merge_luma_420_c = merge_luma_420_t_c;
Processor *merge_luma_420_mpeg2_c = merge_luma_420_mpeg2_t_c;
Processor *merge_luma_420_topleft_c = merge_luma_420_topleft_t_c;
Processor *merge_luma_422_c = merge_luma_422_t_c;
Processor *merge_luma_422_mpeg2_c = merge_luma_422_mpeg2_t_c;
Processor *merge_luma_411_c = merge_luma_411_t_c;
Processor *merge_luma_440_c = merge_luma_440_t_c<false>;
Processor *merge_luma_440_topleft_c = merge_luma_440_t_c<true>;

} } } }
