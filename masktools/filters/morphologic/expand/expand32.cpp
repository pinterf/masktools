#include "expand.h"
#include "../functions32.h"

using namespace Filtering;

typedef float (local_maximum_f)(float a1, float a2, float a3, float a4, float a5, float a6, float a7, float a8, float a9);

static inline float maximum_square(float a1, float a2, float a3, float a4, float a5, float a6, float a7, float a8, float a9)
{
    float nMax = a1;
    if ( a2 > nMax ) nMax = a2;
    if ( a3 > nMax ) nMax = a3;
    if ( a4 > nMax ) nMax = a4;
    if ( a5 > nMax ) nMax = a5;
    if ( a6 > nMax ) nMax = a6;
    if ( a7 > nMax ) nMax = a7;
    if ( a8 > nMax ) nMax = a8;
    if ( a9 > nMax ) nMax = a9;
    return nMax;
}

static inline float maximum_horizontal(float a1, float a2, float a3, float a4, float a5, float a6, float a7, float a8, float a9)
{
    float nMax = a4;

    UNUSED(a1); UNUSED(a2); UNUSED(a3); UNUSED(a7); UNUSED(a8); UNUSED(a9); 

    if ( a5 > nMax ) nMax = a5;
    if ( a6 > nMax ) nMax = a6;
    return nMax;
}

static inline float maximum_vertical(float a1, float a2, float a3, float a4, float a5, float a6, float a7, float a8, float a9)
{
    float nMax = a2;

    UNUSED(a1); UNUSED(a3); UNUSED(a4); UNUSED(a6); UNUSED(a7); UNUSED(a9); 

    if ( a5 > nMax ) nMax = a5;
    if ( a8 > nMax ) nMax = a8;
    return nMax;
}

static inline float maximum_both(float a1, float a2, float a3, float a4, float a5, float a6, float a7, float a8, float a9)
{
    float nMax = a2;

    UNUSED(a1); UNUSED(a3); UNUSED(a7); UNUSED(a9); 

    if ( a4 > nMax ) nMax = a4;
    if ( a5 > nMax ) nMax = a5;
    if ( a6 > nMax ) nMax = a6;
    if ( a8 > nMax ) nMax = a8;
    return nMax;
}

template<local_maximum_f Maximum>
static inline float maximumThresholded(float a1, float a2, float a3, float a4, float a5, float a6, float a7, float a8, float a9, float nMaxDeviation)
{
    float nMaximum = Maximum(a1, a2, a3, a4, a5, a6, a7, a8, a9);
    if ( nMaximum - a5 > nMaxDeviation ) nMaximum = a5 + nMaxDeviation;
    return nMaximum;
}

namespace Filtering { namespace MaskTools { namespace Filters { namespace Morphologic { namespace Expand {

    class NewValue32 {
        float nMax;
        float nMaxDeviation;
        float nValue;
    public:
        NewValue32(float nValue, float nMaxDeviation) : nMax(-1.0), nMaxDeviation(nMaxDeviation), nValue(nValue) { }
        void add(float _nValue) { if ( _nValue > nMax ) nMax = _nValue; }
        float finalize() const { return nMax < 0 ? nValue : (nMax - nValue > nMaxDeviation ? nValue + nMaxDeviation : nMax); }
    };


Processor32 *expand32_square_c = &MorphologicProcessor<float>::generic_32_c<
    process_line_morpho_32_c<Border::Left, maximumThresholded<::maximum_square>>,
    process_line_morpho_32_c<Border::None, maximumThresholded<::maximum_square>>,
    process_line_morpho_32_c<Border::Right, maximumThresholded<::maximum_square>>
    >;

Processor32 *expand32_horizontal_c = &MorphologicProcessor<float>::generic_32_c<
    process_line_morpho_32_c<Border::Left, maximumThresholded<::maximum_horizontal>>,
    process_line_morpho_32_c<Border::None, maximumThresholded<::maximum_horizontal>>,
    process_line_morpho_32_c<Border::Right, maximumThresholded<::maximum_horizontal>>
    >;

Processor32 *expand32_vertical_c = &MorphologicProcessor<float>::generic_32_c<
    process_line_morpho_32_c<Border::Left, maximumThresholded<::maximum_vertical>>,
    process_line_morpho_32_c<Border::None, maximumThresholded<::maximum_vertical>>,
    process_line_morpho_32_c<Border::Right, maximumThresholded<::maximum_vertical>>
    >;

Processor32 *expand32_both_c = &MorphologicProcessor<float>::generic_32_c<
    process_line_morpho_32_c<Border::Left, maximumThresholded<::maximum_both>>,
    process_line_morpho_32_c<Border::None, maximumThresholded<::maximum_both>>,
    process_line_morpho_32_c<Border::Right, maximumThresholded<::maximum_both>>
    >;


Processor32 *expand32_custom_c   = &generic_custom_32_c<NewValue32>;

} } } } }