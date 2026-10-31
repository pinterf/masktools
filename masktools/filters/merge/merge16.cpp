#include "merge.h"
#include "../../common/simd.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Merge {

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

template <CpuFlags flags>
MT_FORCEINLINE static __m128i get_single_mask_value_420(const __m128i &row1_lo, const __m128i &row1_hi, const __m128i &row2_lo, const __m128i &row2_hi) {
    auto avg_lo = _mm_avg_epu16(row1_lo, row2_lo);
    auto avg_hi = _mm_avg_epu16(row1_hi, row2_hi);

    auto avg_lo_sh = _mm_srli_si128(avg_lo, 2);
    auto avg_hi_sh = _mm_srli_si128(avg_hi, 2);

    avg_lo = _mm_avg_epu16(avg_lo, avg_lo_sh);
    avg_hi = _mm_avg_epu16(avg_hi, avg_hi_sh);

    if (flags >= CPU_SSSE3) {
        avg_lo = _mm_shuffle_epi8(avg_lo, _mm_set_epi8(0, 0, 0, 0, 0, 0, 0, 0, 13, 12, 9, 8, 5, 4, 1, 0));
        avg_hi = _mm_shuffle_epi8(avg_hi, _mm_set_epi8(13, 12, 9, 8, 5, 4, 1, 0, 0, 0, 0, 0, 0, 0 ,0 ,0));
    } else {
        avg_lo = _mm_shufflelo_epi16(avg_lo, _MM_SHUFFLE(3, 3, 2, 0));
        avg_lo = _mm_shufflehi_epi16(avg_lo, _MM_SHUFFLE(3, 3, 2, 0));
        avg_lo = _mm_shuffle_epi32(avg_lo, _MM_SHUFFLE(3, 3, 2, 0));

        avg_hi = _mm_shufflelo_epi16(avg_hi, _MM_SHUFFLE(3, 3, 2, 0));
        avg_hi = _mm_shufflehi_epi16(avg_hi, _MM_SHUFFLE(3, 3, 2, 0));
        avg_hi = _mm_shuffle_epi32(avg_hi, _MM_SHUFFLE(2, 0, 3, 3));
    }

    return simd_blend_epi8<flags>(_mm_set_epi32(0xFFFFFFFF, 0xFFFFFFFF, 0, 0), avg_hi, avg_lo);
}

template <CpuFlags flags>
MT_FORCEINLINE static __m128i get_single_mask_value_420_mpeg2(const __m128i &row1_lo, const __m128i &row1_hi, const __m128i &row2_lo, const __m128i &row2_hi, __m128i &right_hi) {
  const __m128i evenmask = _mm_set1_epi32(0x0000FFFF);
  // go to 32bit domain
  auto mid_lo = _mm_add_epi32(_mm_and_si128(row1_lo, evenmask), _mm_and_si128(row2_lo, evenmask)); // evens
  auto mid_hi = _mm_add_epi32(_mm_and_si128(row1_hi, evenmask), _mm_and_si128(row2_hi, evenmask)); // evens

  auto old_right_hi = right_hi;

  auto right_lo = _mm_add_epi32(_mm_srli_epi32(row1_lo, 16), _mm_srli_epi32(row2_lo, 16)); // odds
  right_hi = _mm_add_epi32(_mm_srli_epi32(row1_hi, 16), _mm_srli_epi32(row2_hi, 16));

  auto left_hi = _mm_or_si128(_mm_slli_si128(right_hi, 4), _mm_srli_si128(right_lo, 12)); // 32 byte srli across 128bit registers
  auto left_lo = _mm_or_si128(_mm_slli_si128(right_lo, 4), _mm_srli_si128(old_right_hi, 12)); // shift in from previous right_hi

  // mask: (left + mid*2 + right) / 8. Left, mid and right are already sums of two pixels
  auto four = _mm_set1_epi32(4); // rounder
  auto sum_lo = _mm_add_epi32(_mm_add_epi32(_mm_add_epi32(left_lo, _mm_slli_epi32(mid_lo, 1)), right_lo), four); // left + 2 * mid + right + 4
  auto sum_hi = _mm_add_epi32(_mm_add_epi32(_mm_add_epi32(left_hi, _mm_slli_epi32(mid_hi, 1)), right_hi), four); // left + 2 * mid + right + 4
  auto avg_lo = _mm_srli_epi32(sum_lo, 3);
  auto avg_hi = _mm_srli_epi32(sum_hi, 3);
  // back to 16 bits
  return simd_packus_epi32<flags>(avg_lo, avg_hi);
}

template <CpuFlags flags>
MT_FORCEINLINE static __m128i get_single_mask_value_420_topleft(const __m128i& row0_lo, const __m128i& row0_hi, const __m128i& row1_lo, const __m128i& row1_hi, const __m128i& row2_lo, const __m128i& row2_hi, __m128i& right_hi) {
  const __m128i evenmask = _mm_set1_epi32(0x0000FFFF);
  // go to 32bit domain, 1-2-1 weights
  auto mid_lo = _mm_add_epi32(
    _mm_slli_epi32(_mm_and_si128(row1_lo, evenmask), 1), // *2
    _mm_add_epi32(_mm_and_si128(row0_lo, evenmask), _mm_and_si128(row2_lo, evenmask))); // evens
  auto mid_hi = _mm_add_epi32(
    _mm_slli_epi32(_mm_and_si128(row1_hi, evenmask), 1), // *2
    _mm_add_epi32(_mm_and_si128(row0_hi, evenmask), _mm_and_si128(row2_hi, evenmask))); // evens

  auto old_right_hi = right_hi;

  auto right_lo = _mm_add_epi32(
    _mm_slli_epi32(_mm_srli_epi32(row1_lo, 16), 1), // *2
    _mm_add_epi32(_mm_srli_epi32(row0_lo, 16), _mm_srli_epi32(row2_lo, 16))); // odds
  right_hi = _mm_add_epi32(
    _mm_slli_epi32(_mm_srli_epi32(row1_hi, 16), 1), // *2
    _mm_add_epi32(_mm_srli_epi32(row0_hi, 16), _mm_srli_epi32(row2_hi, 16)));

  auto left_hi = _mm_or_si128(_mm_slli_si128(right_hi, 4), _mm_srli_si128(right_lo, 12)); // 32 byte srli across 128bit registers
  auto left_lo = _mm_or_si128(_mm_slli_si128(right_lo, 4), _mm_srli_si128(old_right_hi, 12)); // shift in from previous right_hi

  // mask: (left + mid*2 + right) / 16. Left, mid and right are already sums of 3 pixels or weight 4
  auto eight = _mm_set1_epi32(8); // rounder
  auto sum_lo = _mm_add_epi32(_mm_add_epi32(_mm_add_epi32(left_lo, _mm_slli_epi32(mid_lo, 1)), right_lo), eight); // left + 2 * mid + right + 8
  auto sum_hi = _mm_add_epi32(_mm_add_epi32(_mm_add_epi32(left_hi, _mm_slli_epi32(mid_hi, 1)), right_hi), eight); // left + 2 * mid + right + 8
  auto avg_lo = _mm_srli_epi32(sum_lo, 4); // /16
  auto avg_hi = _mm_srli_epi32(sum_hi, 4);
  // back to 16 bits
  return simd_packus_epi32<flags>(avg_lo, avg_hi);
}

template <CpuFlags flags>
MT_FORCEINLINE static __m128i get_single_mask_value_422(const __m128i &row1_lo, const __m128i &row1_hi) {
  auto row1_lo_sh = _mm_srli_si128(row1_lo, 2);
  auto row1_hi_sh = _mm_srli_si128(row1_hi, 2);

  auto avg_lo = _mm_avg_epu16(row1_lo, row1_lo_sh);
  auto avg_hi = _mm_avg_epu16(row1_hi, row1_hi_sh);

  if (flags >= CPU_SSSE3) {
    avg_lo = _mm_shuffle_epi8(avg_lo, _mm_set_epi8(0, 0, 0, 0, 0, 0, 0, 0, 13, 12, 9, 8, 5, 4, 1, 0));
    avg_hi = _mm_shuffle_epi8(avg_hi, _mm_set_epi8(13, 12, 9, 8, 5, 4, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0));
  }
  else {
    avg_lo = _mm_shufflelo_epi16(avg_lo, _MM_SHUFFLE(3, 3, 2, 0));
    avg_lo = _mm_shufflehi_epi16(avg_lo, _MM_SHUFFLE(3, 3, 2, 0));
    avg_lo = _mm_shuffle_epi32(avg_lo, _MM_SHUFFLE(3, 3, 2, 0));

    avg_hi = _mm_shufflelo_epi16(avg_hi, _MM_SHUFFLE(3, 3, 2, 0));
    avg_hi = _mm_shufflehi_epi16(avg_hi, _MM_SHUFFLE(3, 3, 2, 0));
    avg_hi = _mm_shuffle_epi32(avg_hi, _MM_SHUFFLE(2, 0, 3, 3));
  }

  return simd_blend_epi8<flags>(_mm_set_epi32(0xFFFFFFFF, 0xFFFFFFFF, 0, 0), avg_hi, avg_lo);
}

template <CpuFlags flags>
MT_FORCEINLINE static __m128i get_single_mask_value_422_mpeg2(const __m128i &row1_lo, const __m128i &row1_hi, __m128i &right_hi) {
  const __m128i evenmask = _mm_set1_epi32(0x0000FFFF);
  // go to 32bit domain
  auto mid_lo = _mm_and_si128(row1_lo, evenmask); // evens
  auto mid_hi = _mm_and_si128(row1_hi, evenmask); // evens

  auto old_right_hi = right_hi;

  auto right_lo = _mm_srli_epi32(row1_lo, 16); // odds
  right_hi = _mm_srli_epi32(row1_hi, 16);

  auto left_hi = _mm_or_si128(_mm_slli_si128(right_hi, 4), _mm_srli_si128(right_lo, 12)); // 32 byte srli across 128bit registers
  auto left_lo = _mm_or_si128(_mm_slli_si128(right_lo, 4), _mm_srli_si128(old_right_hi, 12)); // shift in from previous right_hi

  // mask: (left + mid*2 + right) / 4. 
  auto two = _mm_set1_epi32(2); // rounder
  auto sum_lo = _mm_add_epi32(_mm_add_epi32(_mm_add_epi32(left_lo, _mm_slli_epi32(mid_lo, 1)), right_lo), two); // left + 2 * mid + right + 2
  auto sum_hi = _mm_add_epi32(_mm_add_epi32(_mm_add_epi32(left_hi, _mm_slli_epi32(mid_hi, 1)), right_hi), two); // left + 2 * mid + right + 2
  auto avg_lo = _mm_srli_epi32(sum_lo, 2);
  auto avg_hi = _mm_srli_epi32(sum_hi, 2);
  // back to 16 bits
  return simd_packus_epi32<flags>(avg_lo, avg_hi);
}

template<CpuFlags flags, int bits_per_pixel>
MT_FORCEINLINE static __m128i merge_core_simd(const __m128i &dst, const __m128i &src, const __m128i &mask, const __m128i &max_pixel_value, const __m128i &zero) {
  auto dst_lo = _mm_unpacklo_epi16(dst, zero);
  auto dst_hi = _mm_unpackhi_epi16(dst, zero);

  auto src_lo = _mm_unpacklo_epi16(src, zero);
  auto src_hi = _mm_unpackhi_epi16(src, zero);

  auto mask_lo = _mm_unpacklo_epi16(mask, zero);
  auto mask_hi = _mm_unpackhi_epi16(mask, zero);

  // 16 bits: return dst +(((src - dst) * (mask >> 1)) >> (bits_per_pixel - 1));
  // 10-14 bits: return dst +(((src - dst) * (mask)) >> (bits_per_pixel));

  auto diff_lo = _mm_sub_epi32(src_lo, dst_lo); // diff = src-dst
  auto diff_hi = _mm_sub_epi32(src_hi, dst_hi);

  __m128i lerp_lo, lerp_hi;

  if (bits_per_pixel == 16) {
    auto smask_lo = _mm_srai_epi32(mask_lo, 1); // smask = mask>>1
    auto smask_hi = _mm_srai_epi32(mask_hi, 1);

    lerp_lo = simd_mullo_epi32<flags>(diff_lo, smask_lo);
    lerp_hi = simd_mullo_epi32<flags>(diff_hi, smask_hi);

    lerp_lo = _mm_srai_epi32(lerp_lo, bits_per_pixel - 1); // for n bit, shr (n-1), scale back
    lerp_hi = _mm_srai_epi32(lerp_hi, bits_per_pixel - 1);
  }
  else {
    lerp_lo = simd_mullo_epi32<flags>(diff_lo, mask_lo);
    lerp_hi = simd_mullo_epi32<flags>(diff_hi, mask_hi);

    lerp_lo = _mm_srai_epi32(lerp_lo, bits_per_pixel);
    lerp_hi = _mm_srai_epi32(lerp_hi, bits_per_pixel);
  }

  auto result_lo = _mm_add_epi32(dst_lo, lerp_lo);
  auto result_hi = _mm_add_epi32(dst_hi, lerp_hi);

  auto result = simd_packus_epi32<flags>(result_lo, result_hi);
  if (bits_per_pixel < 16) // otherwise no clamp needed
    result = simd_min_epu16<flags>(result, max_pixel_value);

  auto mask_FFFF = _mm_cmpeq_epi16(mask, max_pixel_value); // mask == max ? FFFF : 0000
  result = simd_blend_epi8<flags>(mask_FFFF, src, result); // ensure that max mask value returns src

  auto mask_zero = _mm_cmpeq_epi16(mask, zero);
  result = simd_blend_epi8<flags>(mask_zero, dst, result); // ensure that zero mask value returns dst

  return result;
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

template <CpuFlags flags>
MT_FORCEINLINE static __m128i get_mask_420_stacked_simd(const uint8_t *pMsb, const uint8_t *pLsb, ptrdiff_t pitch, int x) {
    x = x*2;

    auto row1_lo = read_word_stacked_simd(pMsb, pLsb, x);
    auto row1_hi = read_word_stacked_simd(pMsb, pLsb, x+8);
    auto row2_lo = read_word_stacked_simd(pMsb+pitch, pLsb+pitch, x);
    auto row2_hi = read_word_stacked_simd(pMsb+pitch, pLsb+pitch, x+8);

    return get_single_mask_value_420<flags>(row1_lo, row1_hi, row2_lo, row2_hi);
}

template <CpuFlags flags>
MT_FORCEINLINE static __m128i get_mask_420_mpeg2_stacked_simd(const uint8_t *pMsb, const uint8_t *pLsb, ptrdiff_t pitch, int x, __m128i &right_hi) {
  x = x * 2;

  auto row1_lo = read_word_stacked_simd(pMsb, pLsb, x);
  auto row1_hi = read_word_stacked_simd(pMsb, pLsb, x + 8);
  auto row2_lo = read_word_stacked_simd(pMsb + pitch, pLsb + pitch, x);
  auto row2_hi = read_word_stacked_simd(pMsb + pitch, pLsb + pitch, x + 8);

  return get_single_mask_value_420_mpeg2<flags>(row1_lo, row1_hi, row2_lo, row2_hi, right_hi);
}

template <CpuFlags flags>
MT_FORCEINLINE static __m128i get_mask_422_stacked_simd(const uint8_t *pMsb, const uint8_t *pLsb, int x) {
  x = x * 2;

  auto row1_lo = read_word_stacked_simd(pMsb, pLsb, x);
  auto row1_hi = read_word_stacked_simd(pMsb, pLsb, x + 8);

  return get_single_mask_value_422<flags>(row1_lo, row1_hi);
}

template <CpuFlags flags>
MT_FORCEINLINE static __m128i get_mask_422_mpeg2_stacked_simd(const uint8_t *pMsb, const uint8_t *pLsb, int x, __m128i &right_hi) {
  x = x * 2;

  auto row1_lo = read_word_stacked_simd(pMsb, pLsb, x);
  auto row1_hi = read_word_stacked_simd(pMsb, pLsb, x + 8);

  return get_single_mask_value_422_mpeg2<flags>(row1_lo, row1_hi, right_hi);
}

template <CpuFlags flags, MaskMode mode, Processor16 merge_c, Processor16 merge_allow_leftminus1_c>
void merge16_t_stacked_simd(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch,
  const uint8_t *pMask, ptrdiff_t nMaskPitch, int nWidth, int nHeight, int nOrigHeight)
{
  int wMod8 = (nWidth / 8) * 8;
  auto pDst_s = pDst;
  auto pSrc1_s = pSrc1;
  auto pMask_s = pMask;

  auto pDstLsb = pDst + nDstPitch * nOrigHeight / 2;
  auto pSrc1Lsb = pSrc1 + nSrc1Pitch * nOrigHeight / 2;
  auto pMaskLsb = pMask + nMaskPitch * nOrigHeight / (mode == MASK420 || mode == MASK420_MPEG2 ? 1 : 2); // though mpeg2 not supported for stacked

  auto zero = _mm_setzero_si128();
#pragma warning(disable: 4309)
  auto ffff = _mm_set1_epi16(0xFFFF); // max_pixel_value
#pragma warning(default: 4309)
  auto ff = _mm_set1_epi16(0x00FF);

  for (int j = 0; j < nHeight / 2; ++j) {

    // mpeg2:
    __m128i right_hi;
    if (mode == MASK420_MPEG2) {
      // prepare "right_hi": we need only pixel column #0 as the rightmost 32 bit word (12-15th byte)
      auto row1 = read_word_stacked_simd(pMask, pMaskLsb, 0);
      auto row2 = read_word_stacked_simd(pMask + nMaskPitch, pMaskLsb + nMaskPitch, 0);

#pragma warning(disable: 4309)
      auto evenmask = _mm_set1_epi32(0x0000FFFF);
#pragma warning(default: 4309)
      right_hi = _mm_add_epi32(_mm_slli_si128(_mm_and_si128(row1, evenmask), 12), _mm_slli_si128(_mm_and_si128(row2, evenmask), 12));
    }
    else if (mode == MASK422_MPEG2) {
      // prepare "right_hi": we need only pixel column #0 as the rightmost 32 bit word (12-15th byte)
      auto row1 = read_word_stacked_simd(pMask, pMaskLsb, 0);
#pragma warning(disable: 4309)
      auto evenmask = _mm_set1_epi32(0x0000FFFF);
#pragma warning(default: 4309)
      right_hi = _mm_slli_si128(_mm_and_si128(row1, evenmask), 12);
    }

    for (int i = 0; i < wMod8; i += 8) {
      auto dst = read_word_stacked_simd(pDst, pDstLsb, i);
      auto src = read_word_stacked_simd(pSrc1, pSrc1Lsb, i);
      __m128i mask;

      if (mode == MASK420)
        mask = get_mask_420_stacked_simd<flags>(pMask, pMaskLsb, nMaskPitch, i);
      else if (mode == MASK420_MPEG2)
        mask = get_mask_420_mpeg2_stacked_simd<flags>(pMask, pMaskLsb, nMaskPitch, i, right_hi);
      else if (mode == MASK422)
        mask = get_mask_422_stacked_simd<flags>(pMask, pMaskLsb, i);
      else if (mode == MASK422_MPEG2)
        mask = get_mask_422_mpeg2_stacked_simd<flags>(pMask, pMaskLsb, i, right_hi);
      else
        mask = read_word_stacked_simd(pMask, pMaskLsb, i);

      auto result = merge_core_simd<flags, 16>(dst, src, mask, ffff, zero);

      write_word_stacked_simd(pDst, pDstLsb, i, result, ff, zero);
    }

    pDst += nDstPitch;
    pDstLsb += nDstPitch;
    pSrc1 += nSrc1Pitch;
    pSrc1Lsb += nSrc1Pitch;

    if (mode == MASK420 || mode == MASK420_MPEG2) {
      pMask += nMaskPitch * 2;
      pMaskLsb += nMaskPitch * 2;
    }
    else {
      pMask += nMaskPitch;
      pMaskLsb += nMaskPitch;
    }
  }

  if (nWidth > wMod8) {
    const int maskShift = (mode == MASK420 || mode == MASK420_MPEG2 || mode == MASK422 || mode == MASK422_MPEG2) ? wMod8 * 2 : wMod8;
    if (wMod8 != 0 && (mode == MASK420_MPEG2 || mode == MASK422_MPEG2)) // indexing leftmost - 1 is allowed
      merge_allow_leftminus1_c(pDst_s + wMod8, nDstPitch, pSrc1_s + wMod8, nSrc1Pitch, pMask_s + maskShift, nMaskPitch, (nWidth - wMod8), nHeight, nOrigHeight);
    else
      merge_c(pDst_s + wMod8, nDstPitch, pSrc1_s + wMod8, nSrc1Pitch, pMask_s + maskShift, nMaskPitch, (nWidth - wMod8), nHeight, nOrigHeight);
  }
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


template <CpuFlags flags>
MT_FORCEINLINE static __m128i get_mask_420_simd(const uint8_t *ptr, ptrdiff_t pitch, int x) {
    x = x*2;

    auto row1_lo = simd_load_si128<MemoryMode::SSE2_UNALIGNED>(reinterpret_cast<const __m128i*>(ptr+x));
    auto row1_hi = simd_load_si128<MemoryMode::SSE2_UNALIGNED>(reinterpret_cast<const __m128i*>(ptr+x+16));
    auto row2_lo = simd_load_si128<MemoryMode::SSE2_UNALIGNED>(reinterpret_cast<const __m128i*>(ptr+pitch+x));
    auto row2_hi = simd_load_si128<MemoryMode::SSE2_UNALIGNED>(reinterpret_cast<const __m128i*>(ptr+pitch+x+16));

    return get_single_mask_value_420<flags>(row1_lo, row1_hi, row2_lo, row2_hi);
}

template <CpuFlags flags>
MT_FORCEINLINE static __m128i get_mask_420_mpeg2_simd(const uint8_t *ptr, ptrdiff_t pitch, int x, __m128i &right_hi) {
  x = x * 2;

  auto row1_lo = simd_load_si128<MemoryMode::SSE2_UNALIGNED>(reinterpret_cast<const __m128i*>(ptr + x));
  auto row1_hi = simd_load_si128<MemoryMode::SSE2_UNALIGNED>(reinterpret_cast<const __m128i*>(ptr + x + 16));
  auto row2_lo = simd_load_si128<MemoryMode::SSE2_UNALIGNED>(reinterpret_cast<const __m128i*>(ptr + pitch + x));
  auto row2_hi = simd_load_si128<MemoryMode::SSE2_UNALIGNED>(reinterpret_cast<const __m128i*>(ptr + pitch + x + 16));

  return get_single_mask_value_420_mpeg2<flags>(row1_lo, row1_hi, row2_lo, row2_hi, right_hi); // right: in-out
}

template <CpuFlags flags>
MT_FORCEINLINE static __m128i get_mask_420_topleft_simd(const uint8_t* ptr, ptrdiff_t pitch_prev, ptrdiff_t pitch, int x, __m128i& right_hi) {
  x = x * 2;

  auto row0_lo = simd_load_si128<MemoryMode::SSE2_UNALIGNED>(reinterpret_cast<const __m128i*>(ptr + pitch_prev + x));
  auto row0_hi = simd_load_si128<MemoryMode::SSE2_UNALIGNED>(reinterpret_cast<const __m128i*>(ptr + pitch_prev + x + 16));
  auto row1_lo = simd_load_si128<MemoryMode::SSE2_UNALIGNED>(reinterpret_cast<const __m128i*>(ptr + x));
  auto row1_hi = simd_load_si128<MemoryMode::SSE2_UNALIGNED>(reinterpret_cast<const __m128i*>(ptr + x + 16));
  auto row2_lo = simd_load_si128<MemoryMode::SSE2_UNALIGNED>(reinterpret_cast<const __m128i*>(ptr + pitch + x));
  auto row2_hi = simd_load_si128<MemoryMode::SSE2_UNALIGNED>(reinterpret_cast<const __m128i*>(ptr + pitch + x + 16));

  return get_single_mask_value_420_topleft<flags>(row0_lo, row0_hi, row1_lo, row1_hi, row2_lo, row2_hi, right_hi); // right: in-out
}

template <CpuFlags flags>
MT_FORCEINLINE static __m128i get_mask_422_simd(const uint8_t *ptr, int x) {
  x = x * 2;

  auto row1_lo = simd_load_si128<MemoryMode::SSE2_UNALIGNED>(reinterpret_cast<const __m128i*>(ptr + x));
  auto row1_hi = simd_load_si128<MemoryMode::SSE2_UNALIGNED>(reinterpret_cast<const __m128i*>(ptr + x + 16));
  
  return get_single_mask_value_422<flags>(row1_lo, row1_hi);
}

template <CpuFlags flags>
MT_FORCEINLINE static __m128i get_mask_422_mpeg2_simd(const uint8_t *ptr, int x, __m128i &right_hi) {
  x = x * 2;

  auto row1_lo = simd_load_si128<MemoryMode::SSE2_UNALIGNED>(reinterpret_cast<const __m128i*>(ptr + x));
  auto row1_hi = simd_load_si128<MemoryMode::SSE2_UNALIGNED>(reinterpret_cast<const __m128i*>(ptr + x + 16));

  return get_single_mask_value_422_mpeg2<flags>(row1_lo, row1_hi, right_hi); // right: in-out
}

template <CpuFlags flags, MaskMode mode, Processor16 merge_c, Processor16 merge_allow_leftminus1_c, int bits_per_pixel>
static void merge16_t_simd(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch,
                            const uint8_t *pMask, ptrdiff_t nMaskPitch, int nWidth, int nHeight, int nOrigHeight)
{
    nWidth *= 2; // really rowsize: width * sizeof(uint16), see also division at C trailer

    int wMod16 = (nWidth / 16) * 16;
    auto pDst_s = pDst;
    auto pSrc1_s = pSrc1;
    auto pMask_s = pMask;

    int max = (1 << bits_per_pixel) - 1;

    auto zero = _mm_setzero_si128();
#pragma warning(disable: 4244)
    auto max_pixel_value = _mm_set1_epi16(max); // for checking max
#pragma warning(default: 4244)

    for ( int j = 0; j < nHeight; ++j ) {
      const auto top_or_same = j == 0 ? 0 : -nMaskPitch; // for MASK420_TOPLEFT

      // mpeg2:
      __m128i right_hi;
      if constexpr(mode == MASK420_MPEG2) {
        // prepare "right_hi": we need only pixel column #0 as the rightmost 32 bit word (12-15th byte)
        auto row1 = simd_load_si128<MemoryMode::SSE2_UNALIGNED>(reinterpret_cast<const __m128i*>(pMask + 0));
        auto row2 = simd_load_si128<MemoryMode::SSE2_UNALIGNED>(reinterpret_cast<const __m128i*>(pMask + nMaskPitch + 0));
#pragma warning(disable: 4309)
        auto evenmask = _mm_set1_epi32(0x0000FFFF);
#pragma warning(default: 4309)
        right_hi = _mm_add_epi32(_mm_slli_si128(_mm_and_si128(row1, evenmask), 12), _mm_slli_si128(_mm_and_si128(row2, evenmask), 12));
      } else if (mode == MASK422_MPEG2) {
        // prepare "right_hi": we need only pixel column #0 as the rightmost 32 bit word (12-15th byte)
        auto row1 = simd_load_si128<MemoryMode::SSE2_UNALIGNED>(reinterpret_cast<const __m128i*>(pMask + 0));
#pragma warning(disable: 4309)
        auto evenmask = _mm_set1_epi32(0x0000FFFF);
#pragma warning(default: 4309)
        right_hi = _mm_slli_si128(_mm_and_si128(row1, evenmask), 12);
      }
      else if constexpr (mode == MASK420_TOPLEFT) {
        // prepare "right_hi": we need only pixel column #0 as the rightmost 32 bit word (12-15th byte)
        auto row0 = simd_load_si128<MemoryMode::SSE2_UNALIGNED>(reinterpret_cast<const __m128i*>(pMask + top_or_same + 0));
        auto row1 = simd_load_si128<MemoryMode::SSE2_UNALIGNED>(reinterpret_cast<const __m128i*>(pMask + 0));
        auto row2 = simd_load_si128<MemoryMode::SSE2_UNALIGNED>(reinterpret_cast<const __m128i*>(pMask + nMaskPitch + 0));
#pragma warning(disable: 4309)
        auto evenmask = _mm_set1_epi32(0x0000FFFF);
#pragma warning(default: 4309)
        right_hi = _mm_add_epi32(
          _mm_slli_epi32(_mm_slli_si128(_mm_and_si128(row1, evenmask), 12),1), // *2 weight
          _mm_add_epi32(_mm_slli_si128(_mm_and_si128(row0, evenmask), 12), _mm_slli_si128(_mm_and_si128(row2, evenmask), 12)));
      }
      else if (mode == MASK422_MPEG2) {
        // prepare "right_hi": we need only pixel column #0 as the rightmost 32 bit word (12-15th byte)
        auto row1 = simd_load_si128<MemoryMode::SSE2_UNALIGNED>(reinterpret_cast<const __m128i*>(pMask + 0));
#pragma warning(disable: 4309)
        auto evenmask = _mm_set1_epi32(0x0000FFFF);
#pragma warning(default: 4309)
        right_hi = _mm_slli_si128(_mm_and_si128(row1, evenmask), 12);
      }

      for ( int i = 0; i < wMod16; i+=16 ) {
            auto dst = simd_load_si128<MemoryMode::SSE2_UNALIGNED>(reinterpret_cast<const __m128i*>(pDst+i));
            auto src = simd_load_si128<MemoryMode::SSE2_UNALIGNED>(reinterpret_cast<const __m128i*>(pSrc1+i));
            __m128i mask;

            if (mode == MASK420)
              mask = get_mask_420_simd<flags>(pMask, nMaskPitch, i);
            else if(mode == MASK420_MPEG2)
              mask = get_mask_420_mpeg2_simd<flags>(pMask, nMaskPitch, i, right_hi); // right_hi in/out
            else if (mode == MASK420_TOPLEFT)
              mask = get_mask_420_topleft_simd<flags>(pMask, top_or_same, nMaskPitch, i, right_hi); // right_hi in/out
            else if (mode == MASK422)
              mask = get_mask_422_simd<flags>(pMask, i);
            else if (mode == MASK422_MPEG2)
              mask = get_mask_422_mpeg2_simd<flags>(pMask, i, right_hi);
            else
              mask = simd_load_si128<MemoryMode::SSE2_UNALIGNED>(reinterpret_cast<const __m128i*>(pMask + i));

            __m128i result = merge_core_simd<flags, bits_per_pixel>(dst, src, mask, max_pixel_value, zero);

            simd_store_si128<MemoryMode::SSE2_UNALIGNED>(reinterpret_cast<__m128i*>(pDst+i), result);
        }

        pDst += nDstPitch;
        pSrc1 += nSrc1Pitch;

        if (mode == MASK420 || mode == MASK420_MPEG2 || mode == MASK420_TOPLEFT) {
            pMask += nMaskPitch * 2;
        } else {
            pMask += nMaskPitch; // 422, 444
        }
    }

    if (nWidth > wMod16) {
        const int maskShift = (mode == MASK420 || mode == MASK420_MPEG2 || mode == MASK420_TOPLEFT || mode == MASK422 || mode == MASK422_MPEG2) ? wMod16 * 2 : wMod16;
        if(wMod16 != 0 && (mode == MASK420_MPEG2 || mode == MASK420_TOPLEFT || mode == MASK422_MPEG2)) // indexing leftmost - 1 is allowed
          merge_allow_leftminus1_c(pDst_s + wMod16, nDstPitch, pSrc1_s + wMod16, nSrc1Pitch, pMask_s + maskShift, nMaskPitch, (nWidth - wMod16) / sizeof(uint16_t), nHeight, nOrigHeight);
        else
          merge_c(pDst_s + wMod16, nDstPitch, pSrc1_s + wMod16, nSrc1Pitch, pMask_s + maskShift, nMaskPitch, (nWidth-wMod16) / sizeof(uint16_t), nHeight, nOrigHeight);
    }
}

/* Stacked, always 16 bit */
Processor16 *merge16_c_stacked = &merge16_t_stacked_c<MASK444>;
Processor16 *merge16_luma_420_c_stacked = &merge16_t_stacked_c<MASK420>;
Processor16 *merge16_luma_420_mpeg2_c_stacked = &merge16_t_stacked_c<MASK420_MPEG2>;
Processor16 *merge16_luma_422_c_stacked = &merge16_t_stacked_c<MASK422>;
Processor16 *merge16_luma_422_mpeg2_c_stacked = &merge16_t_stacked_c<MASK422_MPEG2>;

Processor16 *merge16_sse2_stacked = merge16_t_stacked_simd<CPU_SSE2, MASK444, merge16_t_stacked_c<MASK444>, merge16_t_stacked_c<MASK444>>;
Processor16 *merge16_sse4_1_stacked = merge16_t_stacked_simd<CPU_SSE4_1, MASK444, merge16_t_stacked_c<MASK444>, merge16_t_stacked_c<MASK444>>;

Processor16 *merge16_luma_420_sse2_stacked = merge16_t_stacked_simd<CPU_SSE2, MASK420, merge16_t_stacked_c<MASK420>, merge16_t_stacked_c<MASK420>>;
Processor16 *merge16_luma_420_ssse3_stacked = merge16_t_stacked_simd<CPU_SSSE3, MASK420, merge16_t_stacked_c<MASK420>, merge16_t_stacked_c<MASK420>>;
Processor16 *merge16_luma_420_sse4_1_stacked = merge16_t_stacked_simd<CPU_SSE4_1, MASK420, merge16_t_stacked_c<MASK420>, merge16_t_stacked_c<MASK420>>;
Processor16 *merge16_luma_420_mpeg2_sse2_stacked = merge16_t_stacked_simd<CPU_SSE2, MASK420_MPEG2, merge16_t_stacked_c<MASK420_MPEG2>, merge16_t_stacked_allow_leftminus1_c<MASK420_MPEG2>>;
Processor16 *merge16_luma_420_mpeg2_ssse3_stacked = merge16_t_stacked_simd<CPU_SSSE3, MASK420_MPEG2, merge16_t_stacked_c<MASK420_MPEG2>, merge16_t_stacked_allow_leftminus1_c<MASK420_MPEG2>>;
Processor16 *merge16_luma_420_mpeg2_sse4_1_stacked = merge16_t_stacked_simd<CPU_SSE4_1, MASK420_MPEG2, merge16_t_stacked_c<MASK420_MPEG2>, merge16_t_stacked_allow_leftminus1_c<MASK420_MPEG2>>;

Processor16 *merge16_luma_422_sse2_stacked = merge16_t_stacked_simd<CPU_SSE2, MASK422, merge16_t_stacked_c<MASK422>, merge16_t_stacked_c<MASK422>>;
Processor16 *merge16_luma_422_ssse3_stacked = merge16_t_stacked_simd<CPU_SSSE3, MASK422, merge16_t_stacked_c<MASK422>, merge16_t_stacked_c<MASK422>>;
Processor16 *merge16_luma_422_sse4_1_stacked = merge16_t_stacked_simd<CPU_SSE4_1, MASK422, merge16_t_stacked_c<MASK422>, merge16_t_stacked_c<MASK422>>;
Processor16 *merge16_luma_422_mpeg2_sse2_stacked = merge16_t_stacked_simd<CPU_SSE2, MASK422_MPEG2, merge16_t_stacked_c<MASK422_MPEG2>, merge16_t_stacked_allow_leftminus1_c<MASK422_MPEG2>>;
Processor16 *merge16_luma_422_mpeg2_ssse3_stacked = merge16_t_stacked_simd<CPU_SSSE3, MASK422_MPEG2, merge16_t_stacked_c<MASK422_MPEG2>, merge16_t_stacked_allow_leftminus1_c<MASK422_MPEG2>>;
Processor16 *merge16_luma_422_mpeg2_sse4_1_stacked = merge16_t_stacked_simd<CPU_SSE4_1, MASK422_MPEG2, merge16_t_stacked_c<MASK422_MPEG2>, merge16_t_stacked_allow_leftminus1_c<MASK422_MPEG2>>;

/* Native */
// specialize them
// 4:1:1 is only in 8 bits
#define MAKE_TEMPLATES(bits_per_pixel) \
template void merge16_t_c<MASK444, bits_per_pixel>(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch, const uint8_t *pMask, ptrdiff_t nMaskPitch, int nWidth, int nHeight, int nOrigHeight); \
\
template void merge16_t_c<MASK420, bits_per_pixel>(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch, const uint8_t *pMask, ptrdiff_t nMaskPitch, int nWidth, int nHeight, int nOrigHeight); \
template void merge16_t_c<MASK420_MPEG2, bits_per_pixel>(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch, const uint8_t *pMask, ptrdiff_t nMaskPitch, int nWidth, int nHeight, int nOrigHeight); \
template void merge16_t_allow_leftminus1_c<MASK420_MPEG2, bits_per_pixel>(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch, const uint8_t *pMask, ptrdiff_t nMaskPitch, int nWidth, int nHeight, int nOrigHeight); \
\
template void merge16_t_c<MASK422, bits_per_pixel>(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch, const uint8_t *pMask, ptrdiff_t nMaskPitch, int nWidth, int nHeight, int nOrigHeight); \
template void merge16_t_c<MASK422_MPEG2, bits_per_pixel>(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch, const uint8_t *pMask, ptrdiff_t nMaskPitch, int nWidth, int nHeight, int nOrigHeight); \
template void merge16_t_allow_leftminus1_c<MASK422_MPEG2, bits_per_pixel>(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch, const uint8_t *pMask, ptrdiff_t nMaskPitch, int nWidth, int nHeight, int nOrigHeight); \
\
template void merge16_t_simd<CPU_SSE2, MASK444, merge16_t_c<MASK444, bits_per_pixel>, merge16_t_c<MASK444, bits_per_pixel>, bits_per_pixel>(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch, const uint8_t *pMask, ptrdiff_t nMaskPitch, int nWidth, int nHeight, int nOrigHeight); \
template void merge16_t_simd<CPU_SSE4_1, MASK444, merge16_t_c<MASK444, bits_per_pixel>, merge16_t_c<MASK444, bits_per_pixel>, bits_per_pixel>(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch, const uint8_t *pMask, ptrdiff_t nMaskPitch, int nWidth, int nHeight, int nOrigHeight); \
\
template void merge16_t_simd<CPU_SSE2, MASK420, merge16_t_c<MASK420, bits_per_pixel>, merge16_t_c<MASK420, bits_per_pixel>, bits_per_pixel>(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch, const uint8_t *pMask, ptrdiff_t nMaskPitch, int nWidth, int nHeight, int nOrigHeight); \
template void merge16_t_simd<CPU_SSSE3, MASK420, merge16_t_c<MASK420, bits_per_pixel>, merge16_t_c<MASK420, bits_per_pixel>, bits_per_pixel>(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch, const uint8_t *pMask, ptrdiff_t nMaskPitch, int nWidth, int nHeight, int nOrigHeight); \
template void merge16_t_simd<CPU_SSE4_1, MASK420, merge16_t_c<MASK420, bits_per_pixel>, merge16_t_c<MASK420, bits_per_pixel>, bits_per_pixel>(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch, const uint8_t *pMask, ptrdiff_t nMaskPitch, int nWidth, int nHeight, int nOrigHeight); \
template void merge16_t_simd<CPU_SSE2, MASK420_MPEG2, merge16_t_c<MASK420_MPEG2, bits_per_pixel>, merge16_t_allow_leftminus1_c<MASK420_MPEG2, bits_per_pixel>, bits_per_pixel>(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch, const uint8_t *pMask, ptrdiff_t nMaskPitch, int nWidth, int nHeight, int nOrigHeight); \
template void merge16_t_simd<CPU_SSSE3, MASK420_MPEG2, merge16_t_c<MASK420_MPEG2, bits_per_pixel>, merge16_t_allow_leftminus1_c<MASK420_MPEG2, bits_per_pixel>, bits_per_pixel>(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch, const uint8_t *pMask, ptrdiff_t nMaskPitch, int nWidth, int nHeight, int nOrigHeight); \
template void merge16_t_simd<CPU_SSE4_1, MASK420_MPEG2, merge16_t_c<MASK420_MPEG2, bits_per_pixel>, merge16_t_allow_leftminus1_c<MASK420_MPEG2, bits_per_pixel>, bits_per_pixel>(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch, const uint8_t *pMask, ptrdiff_t nMaskPitch, int nWidth, int nHeight, int nOrigHeight); \
template void merge16_t_simd<CPU_SSE2, MASK420_TOPLEFT, merge16_t_c<MASK420_TOPLEFT, bits_per_pixel>, merge16_t_allow_leftminus1_c<MASK420_TOPLEFT, bits_per_pixel>, bits_per_pixel>(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch, const uint8_t *pMask, ptrdiff_t nMaskPitch, int nWidth, int nHeight, int nOrigHeight); \
template void merge16_t_simd<CPU_SSE4_1, MASK420_TOPLEFT, merge16_t_c<MASK420_TOPLEFT, bits_per_pixel>, merge16_t_allow_leftminus1_c<MASK420_TOPLEFT, bits_per_pixel>, bits_per_pixel>(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch, const uint8_t *pMask, ptrdiff_t nMaskPitch, int nWidth, int nHeight, int nOrigHeight); \
\
template void merge16_t_simd<CPU_SSE2, MASK422, merge16_t_c<MASK422, bits_per_pixel>, merge16_t_c<MASK422, bits_per_pixel>, bits_per_pixel>(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch, const uint8_t *pMask, ptrdiff_t nMaskPitch, int nWidth, int nHeight, int nOrigHeight); \
template void merge16_t_simd<CPU_SSSE3, MASK422, merge16_t_c<MASK422, bits_per_pixel>, merge16_t_c<MASK422, bits_per_pixel>, bits_per_pixel>(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch, const uint8_t *pMask, ptrdiff_t nMaskPitch, int nWidth, int nHeight, int nOrigHeight); \
template void merge16_t_simd<CPU_SSE4_1, MASK422, merge16_t_c<MASK422, bits_per_pixel>, merge16_t_c<MASK422, bits_per_pixel>, bits_per_pixel>(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch, const uint8_t *pMask, ptrdiff_t nMaskPitch, int nWidth, int nHeight, int nOrigHeight); \
template void merge16_t_simd<CPU_SSE2, MASK422_MPEG2, merge16_t_c<MASK422_MPEG2, bits_per_pixel>, merge16_t_allow_leftminus1_c<MASK422_MPEG2, bits_per_pixel>, bits_per_pixel>(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch, const uint8_t *pMask, ptrdiff_t nMaskPitch, int nWidth, int nHeight, int nOrigHeight); \
template void merge16_t_simd<CPU_SSSE3, MASK422_MPEG2, merge16_t_c<MASK422_MPEG2, bits_per_pixel>, merge16_t_allow_leftminus1_c<MASK422_MPEG2, bits_per_pixel>, bits_per_pixel>(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch, const uint8_t *pMask, ptrdiff_t nMaskPitch, int nWidth, int nHeight, int nOrigHeight); \
template void merge16_t_simd<CPU_SSE4_1, MASK422_MPEG2, merge16_t_c<MASK422_MPEG2, bits_per_pixel>, merge16_t_allow_leftminus1_c<MASK422_MPEG2, bits_per_pixel>, bits_per_pixel>(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch, const uint8_t *pMask, ptrdiff_t nMaskPitch, int nWidth, int nHeight, int nOrigHeight);

MAKE_TEMPLATES(10)
MAKE_TEMPLATES(12)
MAKE_TEMPLATES(14)
MAKE_TEMPLATES(16)
#undef MAKE_TEMPLATES

#define MAKE_EXPORTS(bits_per_pixel) \
Processor16 *merge16_##bits_per_pixel##_c = &merge16_t_c<MASK444, bits_per_pixel>; \
Processor16 *merge16_luma_420_##bits_per_pixel##_c = &merge16_t_c<MASK420, bits_per_pixel>; \
Processor16 *merge16_luma_420_mpeg2_##bits_per_pixel##_c = &merge16_t_c<MASK420_MPEG2, bits_per_pixel>; \
Processor16 *merge16_luma_420_topleft_##bits_per_pixel##_c = &merge16_t_c<MASK420_TOPLEFT, bits_per_pixel>; \
Processor16 *merge16_luma_422_##bits_per_pixel##_c = &merge16_t_c<MASK422, bits_per_pixel>; \
Processor16 *merge16_luma_422_mpeg2_##bits_per_pixel##_c = &merge16_t_c<MASK422_MPEG2, bits_per_pixel>; \
Processor16 *merge16_##bits_per_pixel##_sse2 = merge16_t_simd<CPU_SSE2, MASK444, merge16_t_c<MASK444, bits_per_pixel>, merge16_t_c<MASK444, bits_per_pixel>, bits_per_pixel>; \
Processor16 *merge16_##bits_per_pixel##_sse4_1 = merge16_t_simd<CPU_SSE4_1, MASK444, merge16_t_c<MASK444, bits_per_pixel>, merge16_t_c<MASK444, bits_per_pixel>, bits_per_pixel>; \
Processor16 *merge16_luma_420_##bits_per_pixel##_sse2 = merge16_t_simd<CPU_SSE2, MASK420, merge16_t_c<MASK420, bits_per_pixel>, merge16_t_c<MASK420, bits_per_pixel>, bits_per_pixel>; \
Processor16 *merge16_luma_420_##bits_per_pixel##_ssse3 = merge16_t_simd<CPU_SSSE3, MASK420, merge16_t_c<MASK420, bits_per_pixel>, merge16_t_c<MASK420, bits_per_pixel>, bits_per_pixel>; \
Processor16 *merge16_luma_420_##bits_per_pixel##_sse4_1 = merge16_t_simd<CPU_SSE4_1, MASK420, merge16_t_c<MASK420, bits_per_pixel>, merge16_t_c<MASK420, bits_per_pixel>, bits_per_pixel>; \
Processor16 *merge16_luma_420_mpeg2_##bits_per_pixel##_sse2 = merge16_t_simd<CPU_SSE2, MASK420_MPEG2, merge16_t_c<MASK420_MPEG2, bits_per_pixel>, merge16_t_allow_leftminus1_c<MASK420_MPEG2, bits_per_pixel>, bits_per_pixel>; \
Processor16 *merge16_luma_420_mpeg2_##bits_per_pixel##_ssse3 = merge16_t_simd<CPU_SSSE3, MASK420_MPEG2, merge16_t_c<MASK420_MPEG2, bits_per_pixel>, merge16_t_allow_leftminus1_c<MASK420_MPEG2, bits_per_pixel>, bits_per_pixel>; \
Processor16 *merge16_luma_420_mpeg2_##bits_per_pixel##_sse4_1 = merge16_t_simd<CPU_SSE4_1, MASK420_MPEG2, merge16_t_c<MASK420_MPEG2, bits_per_pixel>, merge16_t_allow_leftminus1_c<MASK420_MPEG2, bits_per_pixel>, bits_per_pixel>; \
Processor16 *merge16_luma_420_topleft_##bits_per_pixel##_sse2 = merge16_t_simd<CPU_SSE2, MASK420_TOPLEFT, merge16_t_c<MASK420_TOPLEFT, bits_per_pixel>, merge16_t_allow_leftminus1_c<MASK420_TOPLEFT, bits_per_pixel>, bits_per_pixel>; \
Processor16 *merge16_luma_420_topleft_##bits_per_pixel##_sse4_1 = merge16_t_simd<CPU_SSE4_1, MASK420_TOPLEFT, merge16_t_c<MASK420_TOPLEFT, bits_per_pixel>, merge16_t_allow_leftminus1_c<MASK420_TOPLEFT, bits_per_pixel>, bits_per_pixel>; \
Processor16 *merge16_luma_422_##bits_per_pixel##_sse2 = merge16_t_simd<CPU_SSE2, MASK422, merge16_t_c<MASK422, bits_per_pixel>, merge16_t_c<MASK422, bits_per_pixel>, bits_per_pixel>; \
Processor16 *merge16_luma_422_##bits_per_pixel##_ssse3 = merge16_t_simd<CPU_SSSE3, MASK422, merge16_t_c<MASK422, bits_per_pixel>, merge16_t_c<MASK422, bits_per_pixel>, bits_per_pixel>; \
Processor16 *merge16_luma_422_##bits_per_pixel##_sse4_1 = merge16_t_simd<CPU_SSE4_1, MASK422, merge16_t_c<MASK422, bits_per_pixel>, merge16_t_c<MASK422, bits_per_pixel>, bits_per_pixel>; \
Processor16 *merge16_luma_422_mpeg2_##bits_per_pixel##_sse2 = merge16_t_simd<CPU_SSE2, MASK422_MPEG2, merge16_t_c<MASK422_MPEG2, bits_per_pixel>, merge16_t_allow_leftminus1_c<MASK422_MPEG2, bits_per_pixel>, bits_per_pixel>; \
Processor16 *merge16_luma_422_mpeg2_##bits_per_pixel##_ssse3 = merge16_t_simd<CPU_SSSE3, MASK422_MPEG2, merge16_t_c<MASK422_MPEG2, bits_per_pixel>, merge16_t_allow_leftminus1_c<MASK422_MPEG2, bits_per_pixel>, bits_per_pixel>; \
Processor16 *merge16_luma_422_mpeg2_##bits_per_pixel##_sse4_1 = merge16_t_simd<CPU_SSE4_1, MASK422_MPEG2, merge16_t_c<MASK422_MPEG2, bits_per_pixel>, merge16_t_allow_leftminus1_c<MASK422_MPEG2, bits_per_pixel>, bits_per_pixel>;

MAKE_EXPORTS(10)
MAKE_EXPORTS(12)
MAKE_EXPORTS(14)
MAKE_EXPORTS(16)
#undef MAKE_EXPORTS

} } } }