#include "deflate.h"
#include "../functions16.h"

using namespace Filtering;

static MT_FORCEINLINE uint16_t meanMin(uint16_t a1, uint16_t a2, uint16_t a3, uint16_t a4, uint16_t a5, uint16_t a6, uint16_t a7, uint16_t a8, uint16_t a9)
{
    int nSum = 0;
    nSum += a1 + a2 + a3 + a4 + a6 + a7 + a8 + a9;
    nSum >>= 3;
    return static_cast<uint16_t>(nSum < a5 ? nSum : a5);
}

static MT_FORCEINLINE uint16_t meanMinThresholded(uint16_t a1, uint16_t a2, uint16_t a3, uint16_t a4, uint16_t a5, uint16_t a6, uint16_t a7, uint16_t a8, uint16_t a9, int nMaxDeviation)
{
    int nMeanMin = meanMin(a1, a2, a3, a4, a5, a6, a7, a8, a9);
    if ( a5 - nMeanMin > nMaxDeviation ) nMeanMin = a5 - nMaxDeviation;
    return static_cast<uint16_t>(nMeanMin);
}

namespace Filtering { namespace MaskTools { namespace Filters { namespace Morphologic { namespace Deflate {

StackedProcessor *deflate_stacked_c = &MorphologicProcessor<uint8_t>::generic_16_c<
    process_line_morpho_stacked_c<Border::Left, meanMinThresholded>,
    process_line_morpho_stacked_c<Border::None, meanMinThresholded>,
    process_line_morpho_stacked_c<Border::Right, meanMinThresholded>
>;

Processor16 *deflate_16_c = &MorphologicProcessor<uint16_t>::generic_16_c<
    process_line_morpho_16_c<Border::Left, meanMinThresholded>,
    process_line_morpho_16_c<Border::None, meanMinThresholded>,
    process_line_morpho_16_c<Border::Right, meanMinThresholded>
>;

} } } } }