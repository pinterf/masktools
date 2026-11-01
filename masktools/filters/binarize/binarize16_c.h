#ifndef __Mt_binarize16_C_H__
#define __Mt_binarize16_C_H__

// binarize16: C ops, also for the right edge of the SIMD versions. Private header of the filter.

#include "binarize.h"
#include "../../common/common.h"

using namespace Filtering;

namespace {

// max_pixel_value: (1 << bits_per_pixel) - 1
typedef uint16_t (Operator)(uint16_t x, uint16_t t, uint16_t max_pixel_value);

inline uint16_t binarize_upper(uint16_t x, uint16_t t, uint16_t max_pixel_value) { return x > t ? 0 : max_pixel_value; }
inline uint16_t binarize_lower(uint16_t x, uint16_t t, uint16_t max_pixel_value) { return x > t ? max_pixel_value : 0; }
inline uint16_t binarize_0_x(uint16_t x, uint16_t t, uint16_t) { return x > t ? 0 : x; }
inline uint16_t binarize_t_x(uint16_t x, uint16_t t, uint16_t) { return x > t ? t : x; }
inline uint16_t binarize_x_0(uint16_t x, uint16_t t, uint16_t) { return x > t ? x : 0; }
inline uint16_t binarize_x_t(uint16_t x, uint16_t t, uint16_t) { return x > t ? x : t; }
inline uint16_t binarize_t_0(uint16_t x, uint16_t t, uint16_t) { return x > t ? t : 0; }
inline uint16_t binarize_0_t(uint16_t x, uint16_t t, uint16_t) { return x > t ? 0 : t; }
inline uint16_t binarize_x_255(uint16_t x, uint16_t t, uint16_t max_pixel_value) { return x > t ? x : max_pixel_value; }
inline uint16_t binarize_t_255(uint16_t x, uint16_t t, uint16_t max_pixel_value) { return x > t ? t : max_pixel_value; }
inline uint16_t binarize_255_x(uint16_t x, uint16_t t, uint16_t max_pixel_value) { return x > t ? max_pixel_value : x; }
inline uint16_t binarize_255_t(uint16_t x, uint16_t t, uint16_t max_pixel_value) { return x > t ? max_pixel_value : t; }

template <Operator op>
void binarize_native_t(uint8_t *pDst, ptrdiff_t nDstPitch, uint16_t nThreshold, int nWidth, int nHeight, int bits_per_pixel)
{
    const uint16_t max_pixel_value = uint16_t((1 << bits_per_pixel) - 1);
    for ( int y = 0; y < nHeight; y++ )
    {
        auto pDst16 = reinterpret_cast<uint16_t*>(pDst);
        for ( int x = 0; x < nWidth; x+=1 )
            pDst16[x] = op(pDst16[x], nThreshold, max_pixel_value);
        pDst += nDstPitch;
    }
}

} // namespace

#endif // __Mt_binarize16_C_H__
