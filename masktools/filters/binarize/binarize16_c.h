#ifndef __Mt_binarize16_C_H__
#define __Mt_binarize16_C_H__

// binarize16: C ops, also for the right edge of the SIMD versions. Private header of the filter.

#include "binarize.h"
#include "../../common/common.h"

using namespace Filtering;

namespace {

typedef uint16_t (Operator)(uint16_t, uint16_t);

template<int bits_per_pixel>
inline uint16_t binarize_upper(uint16_t x, uint16_t t) { return x > t ? 0 : ((1 << bits_per_pixel)-1); }
template<int bits_per_pixel>
inline uint16_t binarize_lower(uint16_t x, uint16_t t) { return x > t ? ((1 << bits_per_pixel)-1) : 0; }

template<int bits_per_pixel>
inline uint16_t binarize_0_x(uint16_t x, uint16_t t) { return x > t ? 0 : x; }
template<int bits_per_pixel>
inline uint16_t binarize_t_x(uint16_t x, uint16_t t) { return x > t ? t : x; }
template<int bits_per_pixel>
inline uint16_t binarize_x_0(uint16_t x, uint16_t t) { return x > t ? x : 0; }
template<int bits_per_pixel>
inline uint16_t binarize_x_t(uint16_t x, uint16_t t) { return x > t ? x : t; }

template<int bits_per_pixel>
inline uint16_t binarize_t_0(uint16_t x, uint16_t t) { return x > t ? t : 0; }
template<int bits_per_pixel>
inline uint16_t binarize_0_t(uint16_t x, uint16_t t) { return x > t ? 0 : t; }

template<int bits_per_pixel>
inline uint16_t binarize_x_255(uint16_t x, uint16_t t) { return x > t ? x : ((1 << bits_per_pixel)-1); }
template<int bits_per_pixel>
inline uint16_t binarize_t_255(uint16_t x, uint16_t t) { return x > t ? t : ((1 << bits_per_pixel)-1); }
template<int bits_per_pixel>
inline uint16_t binarize_255_x(uint16_t x, uint16_t t) { return x > t ? ((1 << bits_per_pixel)-1) : x; }
template<int bits_per_pixel>
inline uint16_t binarize_255_t(uint16_t x, uint16_t t) { return x > t ? ((1 << bits_per_pixel)-1) : t; }

// specialize them
#define MAKE_TEMPLATES(bits_per_pixel) \
template uint16_t binarize_upper<bits_per_pixel>(uint16_t x, uint16_t t); \
template uint16_t binarize_lower<bits_per_pixel>(uint16_t x, uint16_t t); \
template uint16_t binarize_0_x<bits_per_pixel>(uint16_t x, uint16_t t); \
template uint16_t binarize_t_x<bits_per_pixel>(uint16_t x, uint16_t t); \
template uint16_t binarize_x_0<bits_per_pixel>(uint16_t x, uint16_t t); \
template uint16_t binarize_x_t<bits_per_pixel>(uint16_t x, uint16_t t); \
template uint16_t binarize_t_0<bits_per_pixel>(uint16_t x, uint16_t t); \
template uint16_t binarize_0_t<bits_per_pixel>(uint16_t x, uint16_t t); \
template uint16_t binarize_x_255<bits_per_pixel>(uint16_t x, uint16_t t); \
template uint16_t binarize_t_255<bits_per_pixel>(uint16_t x, uint16_t t); \
template uint16_t binarize_255_x<bits_per_pixel>(uint16_t x, uint16_t t); \
template uint16_t binarize_255_t<bits_per_pixel>(uint16_t x, uint16_t t);

MAKE_TEMPLATES(10)
MAKE_TEMPLATES(12)
MAKE_TEMPLATES(14)
MAKE_TEMPLATES(16)
#undef MAKE_TEMPLATES


template <Operator op>
void binarize_native_t(uint8_t *pDst, ptrdiff_t nDstPitch, uint16_t nThreshold, int nWidth, int nHeight, int)
{
    for ( int y = 0; y < nHeight; y++ )
    {
        auto pDst16 = reinterpret_cast<uint16_t*>(pDst);
        for ( int x = 0; x < nWidth; x+=1 )
            pDst16[x] = op(pDst16[x], nThreshold);
        pDst += nDstPitch;
    }
}

} // namespace

#endif // __Mt_binarize16_C_H__
