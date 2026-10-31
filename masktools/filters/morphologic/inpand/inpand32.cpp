#include "inpand.h"
#include "../functions32.h"

using namespace Filtering;

typedef float (local_minimum_f)(float a1, float a2, float a3, float a4, float a5, float a6, float a7, float a8, float a9);

static inline float minimum_square(float a1, float a2, float a3, float a4, float a5, float a6, float a7, float a8, float a9)
{
    float nMin = a1;
    if ( a2 < nMin ) nMin = a2;
    if ( a3 < nMin ) nMin = a3;
    if ( a4 < nMin ) nMin = a4;
    if ( a5 < nMin ) nMin = a5;
    if ( a6 < nMin ) nMin = a6;
    if ( a7 < nMin ) nMin = a7;
    if ( a8 < nMin ) nMin = a8;
    if ( a9 < nMin ) nMin = a9;
    return nMin;
}

static inline float minimum_horizontal(float a1, float a2, float a3, float a4, float a5, float a6, float a7, float a8, float a9)
{
    float nMin = a4;

    UNUSED(a1); UNUSED(a2); UNUSED(a3); UNUSED(a7); UNUSED(a8); UNUSED(a9); 

    if ( a5 < nMin ) nMin = a5;
    if ( a6 < nMin ) nMin = a6;
    return nMin;
}

static inline float minimum_vertical(float a1, float a2, float a3, float a4, float a5, float a6, float a7, float a8, float a9)
{
    float nMin = a2;

    UNUSED(a1); UNUSED(a3); UNUSED(a4); UNUSED(a6); UNUSED(a7); UNUSED(a9); 

    if ( a5 < nMin ) nMin = a5;
    if ( a8 < nMin ) nMin = a8;
    return nMin;
}

static inline float minimum_both(float a1, float a2, float a3, float a4, float a5, float a6, float a7, float a8, float a9)
{
    float nMin = a2;

    UNUSED(a1); UNUSED(a3); UNUSED(a7); UNUSED(a9); 

    if ( a4 < nMin ) nMin = a4;
    if ( a5 < nMin ) nMin = a5;
    if ( a6 < nMin ) nMin = a6;
    if ( a8 < nMin ) nMin = a8;
    return nMin;
}

template<local_minimum_f Minimum>
static inline float minimumThresholded(float a1, float a2, float a3, float a4, float a5, float a6, float a7, float a8, float a9, float nMaxDeviation)
{
    float nMinimum = Minimum(a1, a2, a3, a4, a5, a6, a7, a8, a9);
    if ( a5 - nMinimum > nMaxDeviation ) nMinimum = a5 - nMaxDeviation;
    return nMinimum;
}

namespace Filtering { namespace MaskTools { namespace Filters { namespace Morphologic { namespace Inpand {

    class NewValue32 {
        float nMin;
        float nMaxDeviation;
        float nValue;
    public:
        NewValue32(float nValue, float nMaxDeviation) : nMin(1.0f), nMaxDeviation(nMaxDeviation), nValue(nValue) { }
        void add(float _nValue) { if ( _nValue < nMin ) nMin = _nValue; }
        float finalize() const { return nMin > 1.0f ? nValue : (nValue - nMin > nMaxDeviation ? nValue - nMaxDeviation : nMin); }
    };


Processor32 *inpand32_square_c = &MorphologicProcessor<float>::generic_32_c<
    process_line_morpho_32_c<Border::Left, minimumThresholded<::minimum_square>>,
    process_line_morpho_32_c<Border::None, minimumThresholded<::minimum_square>>,
    process_line_morpho_32_c<Border::Right, minimumThresholded<::minimum_square>>
    >;

Processor32 *inpand32_horizontal_c = &MorphologicProcessor<float>::generic_32_c<
    process_line_morpho_32_c<Border::Left, minimumThresholded<::minimum_horizontal>>,
    process_line_morpho_32_c<Border::None, minimumThresholded<::minimum_horizontal>>,
    process_line_morpho_32_c<Border::Right, minimumThresholded<::minimum_horizontal>>
    >;

Processor32 *inpand32_vertical_c = &MorphologicProcessor<float>::generic_32_c<
    process_line_morpho_32_c<Border::Left, minimumThresholded<::minimum_vertical>>,
    process_line_morpho_32_c<Border::None, minimumThresholded<::minimum_vertical>>,
    process_line_morpho_32_c<Border::Right, minimumThresholded<::minimum_vertical>>
    >;

Processor32 *inpand32_both_c = &MorphologicProcessor<float>::generic_32_c<
    process_line_morpho_32_c<Border::Left, minimumThresholded<::minimum_both>>,
    process_line_morpho_32_c<Border::None, minimumThresholded<::minimum_both>>,
    process_line_morpho_32_c<Border::Right, minimumThresholded<::minimum_both>>
    >;

Processor32 *inpand32_custom_c = &generic_custom_32_c<NewValue32>;

} } } } }