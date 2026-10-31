#include "binarize.h"
#include "binarize_c.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Binarize {

Processor *binarize_upper_c = &binarize_t<binarize_upper>;
Processor *binarize_lower_c = &binarize_t<binarize_lower>;
Processor *binarize_0_x_c   = &binarize_t<binarize_0_x>;
Processor *binarize_t_x_c   = &binarize_t<binarize_t_x>;
Processor *binarize_x_0_c   = &binarize_t<binarize_x_0>;
Processor *binarize_x_t_c   = &binarize_t<binarize_x_t>;
Processor *binarize_t_0_c   = &binarize_t<binarize_t_0>;
Processor *binarize_0_t_c   = &binarize_t<binarize_0_t>;
Processor *binarize_x_255_c = &binarize_t<binarize_x_255>;
Processor *binarize_t_255_c = &binarize_t<binarize_t_255>;
Processor *binarize_255_x_c = &binarize_t<binarize_255_x>;
Processor *binarize_255_t_c = &binarize_t<binarize_255_t>;

} } } }