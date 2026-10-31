#include "binarize.h"
#include "binarize32_c.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Binarize {

Processor32 *binarize32_upper_c = &binarize32_t<binarize32_upper>;
Processor32 *binarize32_lower_c = &binarize32_t<binarize32_lower>;
Processor32 *binarize32_0_x_c   = &binarize32_t<binarize32_0_x>;
Processor32 *binarize32_t_x_c   = &binarize32_t<binarize32_t_x>;
Processor32 *binarize32_x_0_c   = &binarize32_t<binarize32_x_0>;
Processor32 *binarize32_x_t_c   = &binarize32_t<binarize32_x_t>;
Processor32 *binarize32_t_0_c   = &binarize32_t<binarize32_t_0>;
Processor32 *binarize32_0_t_c   = &binarize32_t<binarize32_0_t>;
Processor32 *binarize32_x_255_c = &binarize32_t<binarize32_x_255>;
Processor32 *binarize32_t_255_c = &binarize32_t<binarize32_t_255>;
Processor32 *binarize32_255_x_c = &binarize32_t<binarize32_255_x>;
Processor32 *binarize32_255_t_c = &binarize32_t<binarize32_255_t>;

} } } }