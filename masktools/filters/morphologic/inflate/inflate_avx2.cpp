#include "inflate.h"

#ifdef INTEL_INTRINSICS

#include "../functions_avx2.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Morphologic { namespace Inflate {

Processor *inflate_avx2 = &generic_avx2<
    process_line_xxflate_avx2<Border::Left, limit_up_avx2>,
    process_line_xxflate_avx2<Border::None, limit_up_avx2>,
    process_line_xxflate_avx2<Border::Right, limit_up_avx2>
>;

} } } } }

#endif // INTEL_INTRINSICS
