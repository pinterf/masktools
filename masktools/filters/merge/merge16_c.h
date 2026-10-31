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
    MASK444
};

template<int bits_per_pixel>
MT_FORCEINLINE static uint16_t merge16_core_c(uint16_t dst, uint16_t src, uint16_t mask) {
    if (mask == 0) {
        return dst;
    } else if (mask == ((1 << bits_per_pixel) - 1)) {
        return src;
    } else {
      if(bits_per_pixel < 16)
        return dst + (((src - dst) * mask) >> bits_per_pixel);
      else
        return dst + (((src - dst) * (mask >> 1)) >> (bits_per_pixel - 1)); // >> 1: avoid overflow
    }
}

/* Stacked */

MT_FORCEINLINE static uint16_t get_value_stacked_c(const uint8_t *pMsb, const uint8_t *pLsb, int x) {
    return (uint16_t(pMsb[x]) << 8) + pLsb[x];
}

MT_FORCEINLINE static uint16_t get_mask_420_stacked_c(const uint8_t *pMsb, const uint8_t *pLsb, ptrdiff_t pitch, int x) {
    x = x*2;
    return ((int((get_value_stacked_c(pMsb, pLsb, x)) + get_value_stacked_c(pMsb + pitch, pLsb + pitch, x) + 1) >> 1) +
        ((int(get_value_stacked_c(pMsb, pLsb, x+1)) + get_value_stacked_c(pMsb + pitch, pLsb + pitch, x+1) + 1) >> 1) + 1) >> 1;
}

MT_FORCEINLINE static uint16_t get_mask_420_mpeg2_stacked_c(const uint8_t *pMsb, const uint8_t *pLsb, ptrdiff_t pitch, int x, int &right) {
  x = x * 2;
  const int left = right;
  const int mid = (get_value_stacked_c(pMsb, pLsb, x)) + get_value_stacked_c(pMsb + pitch, pLsb + pitch, x);
  right = get_value_stacked_c(pMsb, pLsb, x + 1) + get_value_stacked_c(pMsb + pitch, pLsb + pitch, x + 1);
  return (uint16_t)((left + 2 * mid + right + 4) >> 3);
}

MT_FORCEINLINE static uint16_t get_mask_422_stacked_c(const uint8_t *pMsb, const uint8_t *pLsb, int x) {
  x = x * 2;
  return int((get_value_stacked_c(pMsb, pLsb, x)) + get_value_stacked_c(pMsb, pLsb, x + 1) + 1) >> 1;
}

MT_FORCEINLINE static uint16_t get_mask_422_mpeg2_stacked_c(const uint8_t *pMsb, const uint8_t *pLsb, int x, int &right) {
  x = x * 2;
  const int left = right;
  const int mid = (get_value_stacked_c(pMsb, pLsb, x));
  right = get_value_stacked_c(pMsb, pLsb, x + 1);
  return (uint16_t)((left + 2 * mid + right + 2) >> 2);
}


template<MaskMode mode, bool allow_leftminus1>
void internal_merge16_t_stacked_c(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch,
                 const uint8_t *pMask, ptrdiff_t nMaskPitch, int nWidth, int nHeight, int nOrigHeight)
{
    auto pDstLsb = pDst + nDstPitch * nOrigHeight / 2;
    auto pSrc1Lsb = pSrc1 + nSrc1Pitch * nOrigHeight / 2;
    auto pMaskLsb = pMask + nMaskPitch * nOrigHeight / (mode == MASK420 || mode == MASK420_MPEG2 ? 1 : 2); // though mpeg2 not supported for stacked

    for ( int y = 0; y < nHeight / 2; ++y )
    {
        int right;
        if (mode == MASK420_MPEG2)
          right = allow_leftminus1 ?
          get_value_stacked_c(pMask, pMaskLsb, -1) + get_value_stacked_c(pMask + nMaskPitch, pMaskLsb + nMaskPitch, -1) :
          get_value_stacked_c(pMask, pMaskLsb, 0) + get_value_stacked_c(pMask + nMaskPitch, pMaskLsb + nMaskPitch, 0);
        else if (mode == MASK422_MPEG2)
          right = allow_leftminus1 ?
          get_value_stacked_c(pMask, pMaskLsb, -1) :
          get_value_stacked_c(pMask, pMaskLsb, 0);

        for ( int x = 0; x < nWidth; ++x ) {
            uint16_t dst = get_value_stacked_c(pDst, pDstLsb, x);
            uint16_t src = get_value_stacked_c(pSrc1, pSrc1Lsb, x);
            uint16_t mask;

            if (mode == MASK420)
              mask = get_mask_420_stacked_c(pMask, pMaskLsb, nMaskPitch, x);
            else if (mode == MASK420_MPEG2)
              mask = get_mask_420_mpeg2_stacked_c(pMask, pMaskLsb, nMaskPitch, x, right);
            else if (mode == MASK422)
              mask = get_mask_422_stacked_c(pMask, pMaskLsb, x);
            else if (mode == MASK422_MPEG2)
              mask = get_mask_422_mpeg2_stacked_c(pMask, pMaskLsb, x, right);
            else
              mask = get_value_stacked_c(pMask, pMaskLsb, x);

            uint16_t output = merge16_core_c<16>(dst, src, mask);

            pDst[x] = output >> 8;
            pDstLsb[x] = output & 0xFF;
        }
        pDst += nDstPitch;
        pDstLsb += nDstPitch;
        pSrc1 += nSrc1Pitch;
        pSrc1Lsb += nSrc1Pitch;

        if (mode == MASK420 || mode == MASK420_MPEG2) {
            pMask += nMaskPitch * 2;
            pMaskLsb += nMaskPitch * 2;
        } else {
            pMask += nMaskPitch;
            pMaskLsb += nMaskPitch;
        }
    }
}

template<MaskMode mode>
void merge16_t_stacked_c(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch,
  const uint8_t *pMask, ptrdiff_t nMaskPitch, int nWidth, int nHeight, int nOrigHeight)
{
  internal_merge16_t_stacked_c<mode, false>(pDst, nDstPitch, pSrc1, nSrc1Pitch, pMask, nMaskPitch, nWidth, nHeight, nOrigHeight);
}

template<MaskMode mode>
void merge16_t_stacked_allow_leftminus1_c(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch,
  const uint8_t *pMask, ptrdiff_t nMaskPitch, int nWidth, int nHeight, int nOrigHeight)
{
  internal_merge16_t_stacked_c<mode, true>(pDst, nDstPitch, pSrc1, nSrc1Pitch, pMask, nMaskPitch, nWidth, nHeight, nOrigHeight);
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

MT_FORCEINLINE static uint16_t get_mask_422_mpeg2_c(const uint8_t *ptr, int x, int &right) {
  x = x * 2;

  const int left = right;
  const int mid = reinterpret_cast<const uint16_t*>(ptr)[x];
  right = reinterpret_cast<const uint16_t*>(ptr)[x + 1];
  return (uint16_t)((left + 2 * mid + right + 2) >> 2);
}

template<MaskMode mode, int bits_per_pixel, bool allow_leftminus1>
static void internal_merge16_t_c(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch,
                         const uint8_t *pMask, ptrdiff_t nMaskPitch, int nWidth, int nHeight, int /*nOrigHeight*/)
{
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
            else
              mask = reinterpret_cast<const uint16_t*>(pMask)[x];

            uint16_t output = merge16_core_c<bits_per_pixel>(dst, src, mask);

            reinterpret_cast<uint16_t*>(pDst)[x] = output;
        }
        pDst += nDstPitch;
        pSrc1 += nSrc1Pitch;

        if (mode == MASK420 || mode == MASK420_MPEG2 || mode == MASK420_TOPLEFT) {
            pMask += nMaskPitch * 2;
        } else {
            pMask += nMaskPitch; // 422, 444
        }
    }
}

template<MaskMode mode, int bits_per_pixel>
static void merge16_t_c(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch,
  const uint8_t *pMask, ptrdiff_t nMaskPitch, int nWidth, int nHeight, int nOrigHeight)
{
  internal_merge16_t_c<mode, bits_per_pixel, false>(pDst, nDstPitch, pSrc1, nSrc1Pitch, pMask, nMaskPitch, nWidth, nHeight, nOrigHeight);
}

template<MaskMode mode, int bits_per_pixel>
static void merge16_t_allow_leftminus1_c(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch,
  const uint8_t *pMask, ptrdiff_t nMaskPitch, int nWidth, int nHeight, int nOrigHeight)
{ // used for mpeg2 simd, for nonMod16 right side
  internal_merge16_t_c<mode, bits_per_pixel, true>(pDst, nDstPitch, pSrc1, nSrc1Pitch, pMask, nMaskPitch, nWidth, nHeight, nOrigHeight);
}

} // namespace

} } } }

#endif // __Mt_merge16_C_H__
