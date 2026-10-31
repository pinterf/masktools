#include "inpand.h"

#ifdef NEON_INTRINSICS

#include "../functions_neon.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Morphologic { namespace Inpand {

Processor *inpand_square_neon = &morpho8_neon<InpandSquare>;
Processor16 *inpand_square_16_neon = &morpho16_neon<InpandSquare>;
Processor *inpand_both_neon = &morpho8_neon<InpandBoth>;
Processor16 *inpand_both_16_neon = &morpho16_neon<InpandBoth>;
Processor *inpand_horizontal_neon = &morpho8_neon<InpandHorizontal>;
Processor16 *inpand_horizontal_16_neon = &morpho16_neon<InpandHorizontal>;
Processor *inpand_vertical_neon = &morpho8_neon<InpandVertical>;
Processor16 *inpand_vertical_16_neon = &morpho16_neon<InpandVertical>;

} } } } }

#endif // NEON_INTRINSICS
