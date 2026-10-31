#include "inflate.h"
#include "../functions16.h"

using namespace Filtering;

static inline uint16_t meanMax(uint16_t a1, uint16_t a2, uint16_t a3, uint16_t a4, uint16_t a5, uint16_t a6, uint16_t a7, uint16_t a8, uint16_t a9)
{
    int nSum = 0;
    nSum += a1 + a2 + a3 + a4 + a6 + a7 + a8 + a9;
    nSum >>= 3;
    return static_cast<uint16_t>(nSum > a5 ? nSum : a5);
}

static inline uint16_t meanMaxThresholded(uint16_t a1, uint16_t a2, uint16_t a3, uint16_t a4, uint16_t a5, uint16_t a6, uint16_t a7, uint16_t a8, uint16_t a9, int nMaxDeviation)
{
    int nMeanMax = meanMax(a1, a2, a3, a4, a5, a6, a7, a8, a9);
    if ( nMeanMax - a5 > nMaxDeviation ) nMeanMax = a5 + nMaxDeviation;
    return static_cast<uint16_t>(nMeanMax);
}

namespace Filtering { namespace MaskTools { namespace Filters { namespace Morphologic { namespace Inflate {


Processor16 *inflate_16_c = &MorphologicProcessor<uint16_t>::generic_16_c<
    process_line_morpho_16_c<Border::Left, meanMaxThresholded>,
    process_line_morpho_16_c<Border::None, meanMaxThresholded>,
    process_line_morpho_16_c<Border::Right, meanMaxThresholded>
>;

} } } } }