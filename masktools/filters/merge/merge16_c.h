#ifndef __Mt_merge16_C_H__
#define __Mt_merge16_C_H__

// merge16: C functions, also used by the SIMD versions. Private header of the filter.

#include "merge.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Merge {

namespace {


/* Common */

enum MaskMode {
    MASK420,
    MASK420_MPEG2,
    MASK420_TOPLEFT,
    MASK422,
    MASK422_MPEG2,
    MASK411,
    MASK440,
    MASK440_TOPLEFT,
    MASK444
};

struct MergeMagicDiv {
    uint32_t div;
    uint8_t shift;
};

// For max = 2^bpp - 1, these reciprocals make (n * div) >> (32 + shift) equal floor(n / max)
// throughout the rounded blend numerator range.
MT_FORCEINLINE static MergeMagicDiv get_merge_magic_div(int bits_per_pixel) {
    switch (bits_per_pixel) {
    case 10: return { 0x80200803, 9 };
    case 12: return { 0x80080081, 11 };
    case 14: return { 0x80020009, 13 };
    case 16: return { 0x80008001, 15 };
    default: return { 0, 0 };
    }
}

MT_FORCEINLINE static uint16_t merge16_magic_div(uint32_t value, MergeMagicDiv magic) {
    return static_cast<uint16_t>((static_cast<uint64_t>(value) * magic.div) >> (32 + magic.shift));
}

MT_FORCEINLINE static uint16_t merge16_core_c(uint16_t dst, uint16_t src, uint16_t mask, uint16_t max, MergeMagicDiv magic) {
    // Round the weighted sum, then normalize by max (rather than 2^bpp).
    const uint32_t numerator = uint32_t(dst) * (max - mask) + uint32_t(src) * mask + max / 2;
    return merge16_magic_div(numerator, magic);
}

/* Native */

MT_FORCEINLINE static uint16_t get_mask_420_c(const uint8_t *ptr, ptrdiff_t pitch, int x) {
    x = x*2;

    return (((reinterpret_cast<const uint16_t*>(ptr)[x] + reinterpret_cast<const uint16_t*>(ptr+pitch)[x] + 1) >> 1) + 
        ((reinterpret_cast<const uint16_t*>(ptr)[x+1] +  reinterpret_cast<const uint16_t*>(ptr+pitch)[x+1] + 1) >> 1) + 1) >> 1;
}

MT_FORCEINLINE static uint16_t get_mask_420_mpeg2_c(const uint8_t *ptr, ptrdiff_t pitch, int x, int &right) {
  x = x * 2;

  const int left = right;
  const int mid = reinterpret_cast<const uint16_t*>(ptr)[x] + reinterpret_cast<const uint16_t*>(ptr + pitch)[x];
  right = reinterpret_cast<const uint16_t*>(ptr)[x + 1] + reinterpret_cast<const uint16_t*>(ptr + pitch)[x + 1];
  return (uint16_t)((left + 2 * mid + right + 4) >> 3);
}

template<bool allow_topminus1>
MT_FORCEINLINE static uint16_t get_mask_420_topleft_c(const uint8_t* ptr, ptrdiff_t nPrevMaskPitch, ptrdiff_t pitch, int x, int& right) {
  x = x * 2;
  const int left = right;
  const int mid = reinterpret_cast<const uint16_t*>(ptr + nPrevMaskPitch)[x] + (reinterpret_cast<const uint16_t*>(ptr)[x] << 1) + reinterpret_cast<const uint16_t*>(ptr + pitch)[x];
  right = reinterpret_cast<const uint16_t*>(ptr + nPrevMaskPitch)[x + 1] + (reinterpret_cast<const uint16_t*>(ptr)[x + 1] << 1) + reinterpret_cast<const uint16_t*>(ptr + pitch)[x + 1];
  return (uint16_t)((left + 2 * mid + right + 8) >> 4);
}


MT_FORCEINLINE static uint16_t get_mask_422_c(const uint8_t *ptr, int x) {
  x = x * 2;

  return (reinterpret_cast<const uint16_t*>(ptr)[x] + reinterpret_cast<const uint16_t*>(ptr)[x + 1] + 1) >> 1;
}

// 411: the average of the 4 mask pixels of the row (pair averages, as the 8 bit version)
MT_FORCEINLINE static uint16_t get_mask_411_c(const uint8_t *ptr, int x) {
  const uint16_t *p = reinterpret_cast<const uint16_t*>(ptr) + x * 4;
  return (uint16_t)((((p[0] + p[1] + 1) >> 1) + ((p[2] + p[3] + 1) >> 1) + 1) >> 1);
}

MT_FORCEINLINE static uint16_t get_mask_422_mpeg2_c(const uint8_t *ptr, int x, int &right) {
  x = x * 2;

  const int left = right;
  const int mid = reinterpret_cast<const uint16_t*>(ptr)[x];
  right = reinterpret_cast<const uint16_t*>(ptr)[x + 1];
  return (uint16_t)((left + 2 * mid + right + 2) >> 2);
}

template<MaskMode mode, bool allow_leftminus1>
static void internal_merge16_t_c(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch,
                         const uint8_t *pMask, ptrdiff_t nMaskPitch, int nWidth, int nHeight, int bits_per_pixel)
{
    const uint16_t max = (uint16_t)((1 << bits_per_pixel) - 1);
    const MergeMagicDiv magic = get_merge_magic_div(bits_per_pixel);
    for ( int y = 0; y < nHeight; ++y )
    {
        constexpr auto left_or_same = allow_leftminus1 ? -1 : 0;
        const auto top_or_same = y == 0 ? 0 : -nMaskPitch; // for MASK420_TOPLEFT, later -nMaskPitch

        int right;
        if (mode == MASK420_TOPLEFT) {
          right = reinterpret_cast<const uint16_t*>(pMask + top_or_same)[left_or_same] + (reinterpret_cast<const uint16_t*>(pMask)[left_or_same] << 1) + reinterpret_cast<const uint16_t*>(pMask + nMaskPitch)[left_or_same];
        } else if (mode == MASK420_MPEG2)
          right = reinterpret_cast<const uint16_t*>(pMask)[left_or_same] + reinterpret_cast<const uint16_t*>(pMask + nMaskPitch)[left_or_same];
        else if (mode == MASK422_MPEG2)
          right = reinterpret_cast<const uint16_t*>(pMask)[left_or_same];

        for ( int x = 0; x < nWidth; ++x ) {
            uint16_t dst = reinterpret_cast<const uint16_t*>(pDst)[x];
            uint16_t src = reinterpret_cast<const uint16_t*>(pSrc1)[x];
            uint16_t mask;

            if (mode == MASK420)
              mask = get_mask_420_c(pMask, nMaskPitch, x);
            else if (mode == MASK420_MPEG2)
              mask = get_mask_420_mpeg2_c(pMask, nMaskPitch, x, right); // right: in-out parameter
            else if (mode == MASK420_TOPLEFT)
              mask = get_mask_420_topleft_c<true>(pMask, top_or_same, nMaskPitch, x, right); // right: in-out parameter
            else if (mode == MASK422)
              mask = get_mask_422_c(pMask, x);
            else if (mode == MASK422_MPEG2)
              mask = get_mask_422_mpeg2_c(pMask, x, right);
            else if (mode == MASK411)
              mask = get_mask_411_c(pMask, x);
            else if (mode == MASK440 || mode == MASK440_TOPLEFT) {
              const uint16_t *row = reinterpret_cast<const uint16_t*>(pMask);
              const uint16_t *below = reinterpret_cast<const uint16_t*>(pMask + nMaskPitch);
              const uint16_t *above = reinterpret_cast<const uint16_t*>(pMask + top_or_same);
              // Only the vertical axis is subsampled: centered pair or co-sited 1-2-1.
              mask = mode == MASK440_TOPLEFT ? (above[x] + 2 * row[x] + below[x] + 2) >> 2 :
                (row[x] + below[x] + 1) >> 1;
            }
            else
              mask = reinterpret_cast<const uint16_t*>(pMask)[x];

            uint16_t output = merge16_core_c(dst, src, mask, max, magic);

            reinterpret_cast<uint16_t*>(pDst)[x] = output;
        }
        pDst += nDstPitch;
        pSrc1 += nSrc1Pitch;

        if (mode == MASK420 || mode == MASK420_MPEG2 || mode == MASK420_TOPLEFT ||
            mode == MASK440 || mode == MASK440_TOPLEFT) {
            pMask += nMaskPitch * 2;
        } else {
            pMask += nMaskPitch; // 422, 444
        }
    }
}

template<MaskMode mode>
static void merge16_t_c(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch,
  const uint8_t *pMask, ptrdiff_t nMaskPitch, int nWidth, int nHeight, int bits_per_pixel)
{
  internal_merge16_t_c<mode, false>(pDst, nDstPitch, pSrc1, nSrc1Pitch, pMask, nMaskPitch, nWidth, nHeight, bits_per_pixel);
}

template<MaskMode mode>
static void merge16_t_allow_leftminus1_c(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch,
  const uint8_t *pMask, ptrdiff_t nMaskPitch, int nWidth, int nHeight, int bits_per_pixel)
{ // used for mpeg2 simd, for nonMod16 right side
  internal_merge16_t_c<mode, true>(pDst, nDstPitch, pSrc1, nSrc1Pitch, pMask, nMaskPitch, nWidth, nHeight, bits_per_pixel);
}

} // namespace

} } } }

#endif // __Mt_merge16_C_H__
