#include "merge.h"

#include "merge16_c.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Merge {

/* Stacked, always 16 bit */
Processor16 *merge16_c_stacked = &merge16_t_stacked_c<MASK444>;
Processor16 *merge16_luma_420_c_stacked = &merge16_t_stacked_c<MASK420>;
Processor16 *merge16_luma_420_mpeg2_c_stacked = &merge16_t_stacked_c<MASK420_MPEG2>;
Processor16 *merge16_luma_422_c_stacked = &merge16_t_stacked_c<MASK422>;
Processor16 *merge16_luma_422_mpeg2_c_stacked = &merge16_t_stacked_c<MASK422_MPEG2>;

#define MAKE_TEMPLATES(bits_per_pixel) \
 \
 \
 \
 \


MAKE_TEMPLATES(10)
MAKE_TEMPLATES(12)
MAKE_TEMPLATES(14)
MAKE_TEMPLATES(16)
#undef MAKE_TEMPLATES

#define MAKE_EXPORTS(bits_per_pixel) \
Processor16 *merge16_##bits_per_pixel##_c = &merge16_t_c<MASK444, bits_per_pixel>; \
Processor16 *merge16_luma_420_##bits_per_pixel##_c = &merge16_t_c<MASK420, bits_per_pixel>; \
Processor16 *merge16_luma_420_mpeg2_##bits_per_pixel##_c = &merge16_t_c<MASK420_MPEG2, bits_per_pixel>; \
Processor16 *merge16_luma_420_topleft_##bits_per_pixel##_c = &merge16_t_c<MASK420_TOPLEFT, bits_per_pixel>; \
Processor16 *merge16_luma_422_##bits_per_pixel##_c = &merge16_t_c<MASK422, bits_per_pixel>; \
Processor16 *merge16_luma_422_mpeg2_##bits_per_pixel##_c = &merge16_t_c<MASK422_MPEG2, bits_per_pixel>;

MAKE_EXPORTS(10)
MAKE_EXPORTS(12)
MAKE_EXPORTS(14)
MAKE_EXPORTS(16)
#undef MAKE_EXPORTS
} } } }