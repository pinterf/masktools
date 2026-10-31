#include "expand.h"
#include "../functions.h"

using namespace Filtering;

typedef uint8_t (local_maximum_f)(uint8_t a1, uint8_t a2, uint8_t a3, uint8_t a4, uint8_t a5, uint8_t a6, uint8_t a7, uint8_t a8, uint8_t a9);

static inline uint8_t maximum_square(uint8_t a1, uint8_t a2, uint8_t a3, uint8_t a4, uint8_t a5, uint8_t a6, uint8_t a7, uint8_t a8, uint8_t a9)
{
    uint8_t nMax = a1;
    if (a2 > nMax) nMax = a2;
    if (a3 > nMax) nMax = a3;
    if (a4 > nMax) nMax = a4;
    if (a5 > nMax) nMax = a5;
    if (a6 > nMax) nMax = a6;
    if (a7 > nMax) nMax = a7;
    if (a8 > nMax) nMax = a8;
    if (a9 > nMax) nMax = a9;
    return nMax;
}

static inline uint8_t maximum_horizontal(uint8_t a1, uint8_t a2, uint8_t a3, uint8_t a4, uint8_t a5, uint8_t a6, uint8_t a7, uint8_t a8, uint8_t a9)
{
    uint8_t nMax = a4;

    UNUSED(a1); UNUSED(a2); UNUSED(a3); UNUSED(a7); UNUSED(a8); UNUSED(a9);

    if (a5 > nMax) nMax = a5;
    if (a6 > nMax) nMax = a6;
    return nMax;
}

static inline uint8_t maximum_vertical(uint8_t a1, uint8_t a2, uint8_t a3, uint8_t a4, uint8_t a5, uint8_t a6, uint8_t a7, uint8_t a8, uint8_t a9)
{
    uint8_t nMax = a2;

    UNUSED(a1); UNUSED(a3); UNUSED(a4); UNUSED(a6); UNUSED(a7); UNUSED(a9);

    if (a5 > nMax) nMax = a5;
    if (a8 > nMax) nMax = a8;
    return nMax;
}

static inline uint8_t maximum_both(uint8_t a1, uint8_t a2, uint8_t a3, uint8_t a4, uint8_t a5, uint8_t a6, uint8_t a7, uint8_t a8, uint8_t a9)
{
    uint8_t nMax = a2;

    UNUSED(a1); UNUSED(a3); UNUSED(a7); UNUSED(a9);

    if (a4 > nMax) nMax = a4;
    if (a5 > nMax) nMax = a5;
    if (a6 > nMax) nMax = a6;
    if (a8 > nMax) nMax = a8;
    return nMax;
}

template<local_maximum_f Maximum>
static inline uint8_t maximumThresholded(uint8_t a1, uint8_t a2, uint8_t a3, uint8_t a4, uint8_t a5, uint8_t a6, uint8_t a7, uint8_t a8, uint8_t a9, int nMaxDeviation)
{
    int nMaximum = Maximum(a1, a2, a3, a4, a5, a6, a7, a8, a9);
    if (nMaximum - a5 > nMaxDeviation) {
        nMaximum = a5 + nMaxDeviation;
    }
    return static_cast<uint8_t>(nMaximum);
}

namespace Filtering { namespace MaskTools { namespace Filters { namespace Morphologic { namespace Expand {

class ExpandProcessor {
    int max;
    int max_deviation;
    uint8_t value;
public:
    ExpandProcessor(uint8_t nValue, int nMaxDeviation) : max(-1), max_deviation(nMaxDeviation), value(nValue) { }

    void add(uint8_t nValue) {
        if (nValue > max) {
            max = nValue;
        }
    }

    uint8_t finalize() const {
        return static_cast<uint8_t>(max < 0 ? value : (max - value > max_deviation ? value + max_deviation : max));
    }
};

Processor *expand_square_c       = &generic_c<maximumThresholded<::maximum_square> >;
Processor *expand_both_c         = &generic_c<maximumThresholded<::maximum_both> >;
Processor *expand_horizontal_c   = &generic_c<maximumThresholded<::maximum_horizontal> >;
Processor *expand_vertical_c     = &generic_c<maximumThresholded<::maximum_vertical> >;

Processor *expand_custom_c = &generic_custom_c<ExpandProcessor>;

} } } } }