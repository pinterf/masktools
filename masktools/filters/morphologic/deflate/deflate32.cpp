#include "deflate.h"
#include "../functions32.h"
#include "../../../common/simd.h"

using namespace Filtering;

static MT_FORCEINLINE float meanMin(float a1, float a2, float a3, float a4, float a5, float a6, float a7, float a8, float a9)
{
    float nSum = (a1 + a2 + a3 + a4 + a6 + a7 + a8 + a9) / 8.0f;
    return nSum < a5 ? nSum : a5;
}

static MT_FORCEINLINE float meanMinThresholded(float a1, float a2, float a3, float a4, float a5, float a6, float a7, float a8, float a9, float nMaxDeviation)
{
    float nMeanMin = meanMin(a1, a2, a3, a4, a5, a6, a7, a8, a9);
    if ( a5 - nMeanMin > nMaxDeviation ) nMeanMin = a5 - nMaxDeviation;
    return nMeanMin;
}

namespace Filtering { namespace MaskTools { namespace Filters { namespace Morphologic { namespace Deflate {

Processor32 *deflate32_c = &MorphologicProcessor<float>::generic_32_c<
    process_line_morpho_32_c<Border::Left, meanMinThresholded>,
    process_line_morpho_32_c<Border::None, meanMinThresholded>,
    process_line_morpho_32_c<Border::Right, meanMinThresholded>
>;

} } } } }