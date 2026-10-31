#include "inpand.h"
#include "../functions.h"

using namespace Filtering;

typedef uint8_t (local_minimum_f)(uint8_t a1, uint8_t a2, uint8_t a3, uint8_t a4, uint8_t a5, uint8_t a6, uint8_t a7, uint8_t a8, uint8_t a9);

static inline uint8_t minimum_square(uint8_t a1, uint8_t a2, uint8_t a3, uint8_t a4, uint8_t a5, uint8_t a6, uint8_t a7, uint8_t a8, uint8_t a9)
{
    uint8_t nMin = a1;
    if (a2 < nMin) nMin = a2;
    if (a3 < nMin) nMin = a3;
    if (a4 < nMin) nMin = a4;
    if (a5 < nMin) nMin = a5;
    if (a6 < nMin) nMin = a6;
    if (a7 < nMin) nMin = a7;
    if (a8 < nMin) nMin = a8;
    if (a9 < nMin) nMin = a9;
    return nMin;
}

static inline uint8_t minimum_horizontal(uint8_t a1, uint8_t a2, uint8_t a3, uint8_t a4, uint8_t a5, uint8_t a6, uint8_t a7, uint8_t a8, uint8_t a9)
{
    uint8_t nMin = a4;

    UNUSED(a1); UNUSED(a2); UNUSED(a3); UNUSED(a7); UNUSED(a8); UNUSED(a9);

    if (a5 < nMin) nMin = a5;
    if (a6 < nMin) nMin = a6;
    return nMin;
}

static inline uint8_t minimum_vertical(uint8_t a1, uint8_t a2, uint8_t a3, uint8_t a4, uint8_t a5, uint8_t a6, uint8_t a7, uint8_t a8, uint8_t a9)
{
    uint8_t nMin = a2;

    UNUSED(a1); UNUSED(a3); UNUSED(a4); UNUSED(a6); UNUSED(a7); UNUSED(a9);

    if (a5 < nMin) nMin = a5;
    if (a8 < nMin) nMin = a8;
    return nMin;
}

static inline uint8_t minimum_both(uint8_t a1, uint8_t a2, uint8_t a3, uint8_t a4, uint8_t a5, uint8_t a6, uint8_t a7, uint8_t a8, uint8_t a9)
{
    uint8_t nMin = a2;

    UNUSED(a1); UNUSED(a3); UNUSED(a7); UNUSED(a9);

    if (a4 < nMin) nMin = a4;
    if (a5 < nMin) nMin = a5;
    if (a6 < nMin) nMin = a6;
    if (a8 < nMin) nMin = a8;
    return nMin;
}

template<local_minimum_f Minimum>
static inline uint8_t minimumThresholded(uint8_t a1, uint8_t a2, uint8_t a3, uint8_t a4, uint8_t a5, uint8_t a6, uint8_t a7, uint8_t a8, uint8_t a9, int nMaxDeviation)
{
    int minimum = Minimum(a1, a2, a3, a4, a5, a6, a7, a8, a9);
    if (a5 - minimum > nMaxDeviation) {
        minimum = a5 - nMaxDeviation;
    }
    return static_cast<uint8_t>(minimum);
}

extern "C" MT_FORCEINLINE __m128i inpand_operator_sse2(__m128i a, __m128i b) {
    return _mm_min_epu8(a, b);
}

namespace Filtering { namespace MaskTools { namespace Filters { namespace Morphologic { namespace Inpand {

class NewValue {
    int min;
    int max_deviation;
    uint8_t value;
public:
    NewValue(uint8_t nValue, int nMaxDeviation) : min(256), max_deviation(nMaxDeviation), value(nValue) { }

    void add(uint8_t nValue) {
        if (nValue < min) {
            min = nValue;
        }
    }
    uint8_t finalize() const {
        return static_cast<uint8_t>(min > 255 ? value : (value - min > max_deviation ? value - max_deviation : min));
    }
};

Processor *inpand_square_c       = &generic_c<minimumThresholded<::minimum_square> >;
Processor *inpand_horizontal_c   = &generic_c<minimumThresholded<::minimum_horizontal> >;
Processor *inpand_vertical_c     = &generic_c<minimumThresholded<::minimum_vertical> >;
Processor *inpand_both_c         = &generic_c<minimumThresholded<::minimum_both> >;

#define DEFINE_SSE2_VERSIONS(name, mem_mode) \
    Processor *inpand_both_##name = &generic_sse2< \
    process_line_xxpand_both<Border::Left, inpand_operator_sse2, limit_down_sse2, mem_mode>, \
    process_line_xxpand_both<Border::None, inpand_operator_sse2, limit_down_sse2, mem_mode>, \
    process_line_xxpand_both<Border::Right, inpand_operator_sse2, limit_down_sse2, MemoryMode::SSE2_UNALIGNED> \
    >;

DEFINE_SSE2_VERSIONS(sse2, MemoryMode::SSE2_UNALIGNED)
DEFINE_SSE2_VERSIONS(asse2, MemoryMode::SSE2_ALIGNED)

Processor *inpand_vertical_sse2 = &xxpand_sse2_vertical<inpand_operator_sse2, limit_down_sse2, MemoryMode::SSE2_UNALIGNED>;
Processor *inpand_vertical_asse2 = &xxpand_sse2_vertical<inpand_operator_sse2, limit_down_sse2, MemoryMode::SSE2_ALIGNED>;

Processor *inpand_horizontal_sse2 = &xxpand_sse2_horizontal<inpand_operator_sse2, limit_down_sse2, MemoryMode::SSE2_UNALIGNED, inpand_c_horizontal_core>;
Processor *inpand_horizontal_asse2 = &xxpand_sse2_horizontal<inpand_operator_sse2, limit_down_sse2, MemoryMode::SSE2_ALIGNED, inpand_c_horizontal_core>;

Processor *inpand_square_sse2 = &xxpand_sse2_square<inpand_operator_sse2, limit_down_sse2, MemoryMode::SSE2_UNALIGNED, inpand_c_horizontal_core>;
Processor *inpand_square_asse2 = &xxpand_sse2_square<inpand_operator_sse2, limit_down_sse2, MemoryMode::SSE2_ALIGNED, inpand_c_horizontal_core>;


Processor *inpand_custom_c = &generic_custom_c<NewValue>;

} } } } }