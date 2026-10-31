#include "inflate.h"
#include "../functions32.h"
#include "../../../common/simd.h"

using namespace Filtering;

static inline float meanMax(float a1, float a2, float a3, float a4, float a5, float a6, float a7, float a8, float a9)
{
    float nSum = (a1 + a2 + a3 + a4 + a6 + a7 + a8 + a9) / 8.0f;
    return nSum > a5 ? nSum : a5;
}

static inline float meanMaxThresholded(float a1, float a2, float a3, float a4, float a5, float a6, float a7, float a8, float a9, float nMaxDeviation)
{
    float nMeanMax = meanMax(a1, a2, a3, a4, a5, a6, a7, a8, a9);
    if ( nMeanMax - a5 > nMaxDeviation ) nMeanMax = a5 + nMaxDeviation;
    return nMeanMax;
}

namespace Filtering { namespace MaskTools { namespace Filters { namespace Morphologic { namespace Inflate {

Processor32 *inflate32_c = &MorphologicProcessor<float>::generic_32_c<
    process_line_morpho_32_c<Border::Left, meanMaxThresholded>,
    process_line_morpho_32_c<Border::None, meanMaxThresholded>,
    process_line_morpho_32_c<Border::Right, meanMaxThresholded>
>;

} } } } }