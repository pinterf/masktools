#include "merge.h"

#include "merge16_c.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Merge {


Processor16 *merge16_c = &merge16_t_c<MASK444>;
Processor16 *merge16_luma_420_c = &merge16_t_c<MASK420>;
Processor16 *merge16_luma_420_mpeg2_c = &merge16_t_c<MASK420_MPEG2>;
Processor16 *merge16_luma_420_topleft_c = &merge16_t_c<MASK420_TOPLEFT>;
Processor16 *merge16_luma_422_c = &merge16_t_c<MASK422>;
Processor16 *merge16_luma_422_mpeg2_c = &merge16_t_c<MASK422_MPEG2>;
Processor16 *merge16_luma_411_c = &merge16_t_c<MASK411>;
Processor16 *merge16_luma_411_mpeg2_c = &merge16_t_c<MASK411_MPEG2>;
Processor16 *merge16_luma_440_c = &merge16_t_c<MASK440>;
Processor16 *merge16_luma_440_topleft_c = &merge16_t_c<MASK440_TOPLEFT>;
Processor16 *merge16_luma_410_c = &merge16_t_c<MASK410>;
Processor16 *merge16_luma_410_mpeg2_c = &merge16_t_c<MASK410_MPEG2>;
Processor16 *merge16_luma_410_topleft_c = &merge16_t_c<MASK410_TOPLEFT>;

} } } }