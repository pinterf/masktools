#include "expand.h"

#ifdef NEON_INTRINSICS

#include "../functions_neon.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Morphologic { namespace Expand {

Processor *expand_square_neon = &morpho8_neon<ExpandSquare>;
Processor16 *expand_square_16_neon = &morpho16_neon<ExpandSquare>;
Processor *expand_both_neon = &morpho8_neon<ExpandBoth>;
Processor16 *expand_both_16_neon = &morpho16_neon<ExpandBoth>;
Processor *expand_horizontal_neon = &morpho8_neon<ExpandHorizontal>;
Processor16 *expand_horizontal_16_neon = &morpho16_neon<ExpandHorizontal>;
Processor *expand_vertical_neon = &morpho8_neon<ExpandVertical>;
Processor16 *expand_vertical_16_neon = &morpho16_neon<ExpandVertical>;

} } } } }

#endif // NEON_INTRINSICS
