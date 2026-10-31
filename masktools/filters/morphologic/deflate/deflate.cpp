#include "deflate.h"
#include "../functions.h"

using namespace Filtering;

static inline uint8_t meanMin(uint8_t a1, uint8_t a2, uint8_t a3, uint8_t a4, uint8_t a5, uint8_t a6, uint8_t a7, uint8_t a8, uint8_t a9)
{
    int nSum = 0;
    nSum += a1 + a2 + a3 + a4 + a6 + a7 + a8 + a9;
    nSum >>= 3;
    return static_cast<uint8_t>(nSum < a5 ? nSum : a5);
}

static inline uint8_t meanMinThresholded(uint8_t a1, uint8_t a2, uint8_t a3, uint8_t a4, uint8_t a5, uint8_t a6, uint8_t a7, uint8_t a8, uint8_t a9, int nMaxDeviation)
{
    int nMeanMin = meanMin(a1, a2, a3, a4, a5, a6, a7, a8, a9);
    if (a5 - nMeanMin > nMaxDeviation) {
        nMeanMin = a5 - nMaxDeviation;
    }
    return static_cast<uint8_t>(nMeanMin);
}

namespace Filtering { namespace MaskTools { namespace Filters { namespace Morphologic { namespace Deflate {

Processor *deflate_c = &generic_c<meanMinThresholded>;

} } } } }