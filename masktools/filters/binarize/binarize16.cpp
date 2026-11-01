#include "binarize.h"
#include "binarize16_c.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Binarize {

Processor16 *binarize_upper_native_c = &binarize_native_t<binarize_upper>;
Processor16 *binarize_lower_native_c = &binarize_native_t<binarize_lower>;
Processor16 *binarize_0_x_native_c   = &binarize_native_t<binarize_0_x>;
Processor16 *binarize_t_x_native_c   = &binarize_native_t<binarize_t_x>;
Processor16 *binarize_x_0_native_c   = &binarize_native_t<binarize_x_0>;
Processor16 *binarize_x_t_native_c   = &binarize_native_t<binarize_x_t>;
Processor16 *binarize_t_0_native_c   = &binarize_native_t<binarize_t_0>;
Processor16 *binarize_0_t_native_c   = &binarize_native_t<binarize_0_t>;
Processor16 *binarize_x_255_native_c = &binarize_native_t<binarize_x_255>;
Processor16 *binarize_t_255_native_c = &binarize_native_t<binarize_t_255>;
Processor16 *binarize_255_x_native_c = &binarize_native_t<binarize_255_x>;
Processor16 *binarize_255_t_native_c = &binarize_native_t<binarize_255_t>;

} } } }
