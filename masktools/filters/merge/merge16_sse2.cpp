#include "merge.h"

#ifdef INTEL_INTRINSICS

#include "../../common/simd.h"
#include "merge16_c.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Merge {

namespace {

// 32 bit lanes holding values < 2^(16 + s) -> 16 bit (x >> s): the 16 bit result is sign extended for packs
template<int s>
MT_FORCEINLINE static __m128i pack_shifted_epi32(const __m128i &lo, const __m128i &hi) {
  return _mm_packs_epi32(_mm_srai_epi32(_mm_slli_epi32(lo, 16 - s), 16), _mm_srai_epi32(_mm_slli_epi32(hi, 16 - s), 16));
}

// average of the horizontal pairs, the result in the even words
MT_FORCEINLINE static __m128i get_single_mask_value_422(const __m128i &row1_lo, const __m128i &row1_hi) {
  auto avg_lo = _mm_avg_epu16(row1_lo, _mm_srli_epi32(row1_lo, 16));
  auto avg_hi = _mm_avg_epu16(row1_hi, _mm_srli_epi32(row1_hi, 16));
  return pack_shifted_epi32<0>(avg_lo, avg_hi);
}

MT_FORCEINLINE static __m128i get_single_mask_value_420(const __m128i &row1_lo, const __m128i &row1_hi, const __m128i &row2_lo, const __m128i &row2_hi) {
  return get_single_mask_value_422(_mm_avg_epu16(row1_lo, row2_lo), _mm_avg_epu16(row1_hi, row2_hi));
}

// mpeg2 / top_left: (left + 2 * mid + right + rounder) >> s, left / mid / right are already column sums
template<int s>
MT_FORCEINLINE static __m128i get_mask_121_from_columns(const __m128i &mid_lo, const __m128i &mid_hi, const __m128i &right_lo, const __m128i &right_hi, const __m128i &old_right_hi) {
  auto left_hi = _mm_or_si128(_mm_slli_si128(right_hi, 4), _mm_srli_si128(right_lo, 12)); // 32 byte srli across 128bit registers
  auto left_lo = _mm_or_si128(_mm_slli_si128(right_lo, 4), _mm_srli_si128(old_right_hi, 12)); // shift in from previous right_hi

  auto rounder = _mm_set1_epi32(1 << (s - 1));
  auto sum_lo = _mm_add_epi32(_mm_add_epi32(_mm_add_epi32(left_lo, _mm_slli_epi32(mid_lo, 1)), right_lo), rounder);
  auto sum_hi = _mm_add_epi32(_mm_add_epi32(_mm_add_epi32(left_hi, _mm_slli_epi32(mid_hi, 1)), right_hi), rounder);
  return pack_shifted_epi32<s>(sum_lo, sum_hi);
}

MT_FORCEINLINE static __m128i get_single_mask_value_420_mpeg2(const __m128i &row1_lo, const __m128i &row1_hi, const __m128i &row2_lo, const __m128i &row2_hi, __m128i &right_hi) {
  const __m128i evenmask = _mm_set1_epi32(0x0000FFFF);
  // go to 32bit domain
  auto mid_lo = _mm_add_epi32(_mm_and_si128(row1_lo, evenmask), _mm_and_si128(row2_lo, evenmask)); // evens
  auto mid_hi = _mm_add_epi32(_mm_and_si128(row1_hi, evenmask), _mm_and_si128(row2_hi, evenmask)); // evens

  auto old_right_hi = right_hi;

  auto right_lo = _mm_add_epi32(_mm_srli_epi32(row1_lo, 16), _mm_srli_epi32(row2_lo, 16)); // odds
  right_hi = _mm_add_epi32(_mm_srli_epi32(row1_hi, 16), _mm_srli_epi32(row2_hi, 16));

  return get_mask_121_from_columns<3>(mid_lo, mid_hi, right_lo, right_hi, old_right_hi); // sums of two rows: / 8
}

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

  return get_mask_121_from_columns<4>(mid_lo, mid_hi, right_lo, right_hi, old_right_hi); // 1-2-1 column sums: / 16
}

MT_FORCEINLINE static __m128i get_single_mask_value_422_mpeg2(const __m128i &row1_lo, const __m128i &row1_hi, __m128i &right_hi) {
  const __m128i evenmask = _mm_set1_epi32(0x0000FFFF);
  // go to 32bit domain
  auto mid_lo = _mm_and_si128(row1_lo, evenmask); // evens
  auto mid_hi = _mm_and_si128(row1_hi, evenmask); // evens

  auto old_right_hi = right_hi;

  auto right_lo = _mm_srli_epi32(row1_lo, 16); // odds
  right_hi = _mm_srli_epi32(row1_hi, 16);

  return get_mask_121_from_columns<2>(mid_lo, mid_hi, right_lo, right_hi, old_right_hi); // / 4
}

// C: mask == 0: dst, mask == max: src, else dst + (((src - dst) * m) >> shift); m = mask (10-14 bit) or mask >> 1 (16 bit).
// Same as (src * m + dst * (2^shift - m)) >> shift, which gives dst for m = 0 and src for m = 2^shift: mask == max is
// mapped to m = 2^shift, no blends needed.
template<bool lessThan16bits>
MT_FORCEINLINE static __m128i merge_core_simd(const __m128i &dst, const __m128i &src, const __m128i &mask, const __m128i &max_pixel_value, const __m128i &range, const __m128i &shift) {
  auto is_max = _mm_cmpeq_epi16(mask, max_pixel_value); // FFFF = -1
  if constexpr (lessThan16bits) {
    // every value fits in int16 (m, w <= 2^14): madd, sum < 2^28
    auto m = _mm_sub_epi16(mask, is_max);
    auto w = _mm_sub_epi16(range, m);
    auto lo = _mm_madd_epi16(_mm_unpacklo_epi16(src, dst), _mm_unpacklo_epi16(m, w));
    auto hi = _mm_madd_epi16(_mm_unpackhi_epi16(src, dst), _mm_unpackhi_epi16(m, w));
    return _mm_packs_epi32(_mm_sra_epi32(lo, shift), _mm_sra_epi32(hi, shift));
  }
  else {
    // unsigned 16x16 products, m + w = 32768, sum < 2^31
    auto m = _mm_sub_epi16(_mm_srli_epi16(mask, 1), is_max);
    auto w = _mm_sub_epi16(range, m);
    auto s_lo = _mm_mullo_epi16(src, m);
    auto s_hi = _mm_mulhi_epu16(src, m);
    auto d_lo = _mm_mullo_epi16(dst, w);
    auto d_hi = _mm_mulhi_epu16(dst, w);
    auto lo = _mm_add_epi32(_mm_unpacklo_epi16(s_lo, s_hi), _mm_unpacklo_epi16(d_lo, d_hi));
    auto hi = _mm_add_epi32(_mm_unpackhi_epi16(s_lo, s_hi), _mm_unpackhi_epi16(d_lo, d_hi));
    return pack_shifted_epi32<15>(lo, hi);
  }
}

MT_FORCEINLINE static __m128i get_mask_420_simd(const uint8_t *ptr, ptrdiff_t pitch, int x) {
  x = x * 2;

  auto row1_lo = _mm_loadu_si128(reinterpret_cast<const __m128i*>(ptr + x));
  auto row1_hi = _mm_loadu_si128(reinterpret_cast<const __m128i*>(ptr + x + 16));
  auto row2_lo = _mm_loadu_si128(reinterpret_cast<const __m128i*>(ptr + pitch + x));
  auto row2_hi = _mm_loadu_si128(reinterpret_cast<const __m128i*>(ptr + pitch + x + 16));

  return get_single_mask_value_420(row1_lo, row1_hi, row2_lo, row2_hi);
}

MT_FORCEINLINE static __m128i get_mask_420_mpeg2_simd(const uint8_t *ptr, ptrdiff_t pitch, int x, __m128i &right_hi) {
  x = x * 2;

  auto row1_lo = _mm_loadu_si128(reinterpret_cast<const __m128i*>(ptr + x));
  auto row1_hi = _mm_loadu_si128(reinterpret_cast<const __m128i*>(ptr + x + 16));
  auto row2_lo = _mm_loadu_si128(reinterpret_cast<const __m128i*>(ptr + pitch + x));
  auto row2_hi = _mm_loadu_si128(reinterpret_cast<const __m128i*>(ptr + pitch + x + 16));

  return get_single_mask_value_420_mpeg2(row1_lo, row1_hi, row2_lo, row2_hi, right_hi); // right: in-out
}

MT_FORCEINLINE static __m128i get_mask_420_topleft_simd(const uint8_t* ptr, ptrdiff_t pitch_prev, ptrdiff_t pitch, int x, __m128i& right_hi) {
  x = x * 2;

  auto row0_lo = _mm_loadu_si128(reinterpret_cast<const __m128i*>(ptr + pitch_prev + x));
  auto row0_hi = _mm_loadu_si128(reinterpret_cast<const __m128i*>(ptr + pitch_prev + x + 16));
  auto row1_lo = _mm_loadu_si128(reinterpret_cast<const __m128i*>(ptr + x));
  auto row1_hi = _mm_loadu_si128(reinterpret_cast<const __m128i*>(ptr + x + 16));
  auto row2_lo = _mm_loadu_si128(reinterpret_cast<const __m128i*>(ptr + pitch + x));
  auto row2_hi = _mm_loadu_si128(reinterpret_cast<const __m128i*>(ptr + pitch + x + 16));

  return get_single_mask_value_420_topleft(row0_lo, row0_hi, row1_lo, row1_hi, row2_lo, row2_hi, right_hi); // right: in-out
}

MT_FORCEINLINE static __m128i get_mask_422_simd(const uint8_t *ptr, int x) {
  x = x * 2;

  auto row1_lo = _mm_loadu_si128(reinterpret_cast<const __m128i*>(ptr + x));
  auto row1_hi = _mm_loadu_si128(reinterpret_cast<const __m128i*>(ptr + x + 16));

  return get_single_mask_value_422(row1_lo, row1_hi);
}

MT_FORCEINLINE static __m128i get_mask_422_mpeg2_simd(const uint8_t *ptr, int x, __m128i &right_hi) {
  x = x * 2;

  auto row1_lo = _mm_loadu_si128(reinterpret_cast<const __m128i*>(ptr + x));
  auto row1_hi = _mm_loadu_si128(reinterpret_cast<const __m128i*>(ptr + x + 16));

  return get_single_mask_value_422_mpeg2(row1_lo, row1_hi, right_hi); // right: in-out
}

template <MaskMode mode, Processor16 merge_c, Processor16 merge_allow_leftminus1_c, bool lessThan16bits>
static void merge16_t_simd(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch,
                            const uint8_t *pMask, ptrdiff_t nMaskPitch, int nWidth, int nHeight, int bits_per_pixel)
{
    nWidth *= 2; // really rowsize: width * sizeof(uint16), see also division at C trailer

    int wMod16 = (nWidth / 16) * 16;
    auto pDst_s = pDst;
    auto pSrc1_s = pSrc1;
    auto pMask_s = pMask;

    const int shift_bits = lessThan16bits ? bits_per_pixel : 15;
    const auto max_pixel_value = _mm_set1_epi16((int16_t)((1 << bits_per_pixel) - 1));
    const auto range = _mm_set1_epi16((int16_t)(1 << shift_bits)); // m + w
    const auto shift = _mm_cvtsi32_si128(shift_bits);
    const __m128i evenmask = _mm_set1_epi32(0x0000FFFF);

    for ( int j = 0; j < nHeight; ++j ) {
      const auto top_or_same = j == 0 ? 0 : -nMaskPitch; // for MASK420_TOPLEFT

      // mpeg2, top_left: "right_hi" needs only pixel column #0 as the rightmost 32 bit word (12-15th byte)
      __m128i right_hi;
      if constexpr (mode == MASK420_MPEG2) {
        auto row1 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pMask));
        auto row2 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pMask + nMaskPitch));
        right_hi = _mm_add_epi32(_mm_slli_si128(_mm_and_si128(row1, evenmask), 12), _mm_slli_si128(_mm_and_si128(row2, evenmask), 12));
      }
      else if constexpr (mode == MASK422_MPEG2) {
        auto row1 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pMask));
        right_hi = _mm_slli_si128(_mm_and_si128(row1, evenmask), 12);
      }
      else if constexpr (mode == MASK420_TOPLEFT) {
        auto row0 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pMask + top_or_same));
        auto row1 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pMask));
        auto row2 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pMask + nMaskPitch));
        right_hi = _mm_add_epi32(
          _mm_slli_epi32(_mm_slli_si128(_mm_and_si128(row1, evenmask), 12), 1), // *2 weight
          _mm_add_epi32(_mm_slli_si128(_mm_and_si128(row0, evenmask), 12), _mm_slli_si128(_mm_and_si128(row2, evenmask), 12)));
      }

      for ( int i = 0; i < wMod16; i+=16 ) {
            auto dst = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pDst+i));
            auto src = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pSrc1+i));
            __m128i mask;

            if constexpr (mode == MASK420)
              mask = get_mask_420_simd(pMask, nMaskPitch, i);
            else if constexpr (mode == MASK420_MPEG2)
              mask = get_mask_420_mpeg2_simd(pMask, nMaskPitch, i, right_hi); // right_hi in/out
            else if constexpr (mode == MASK420_TOPLEFT)
              mask = get_mask_420_topleft_simd(pMask, top_or_same, nMaskPitch, i, right_hi); // right_hi in/out
            else if constexpr (mode == MASK422)
              mask = get_mask_422_simd(pMask, i);
            else if constexpr (mode == MASK422_MPEG2)
              mask = get_mask_422_mpeg2_simd(pMask, i, right_hi);
            else
              mask = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pMask + i));

            __m128i result = merge_core_simd<lessThan16bits>(dst, src, mask, max_pixel_value, range, shift);

            _mm_storeu_si128(reinterpret_cast<__m128i*>(pDst+i), result);
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
          merge_allow_leftminus1_c(pDst_s + wMod16, nDstPitch, pSrc1_s + wMod16, nSrc1Pitch, pMask_s + maskShift, nMaskPitch, (nWidth - wMod16) / sizeof(uint16_t), nHeight, bits_per_pixel);
        else
          merge_c(pDst_s + wMod16, nDstPitch, pSrc1_s + wMod16, nSrc1Pitch, pMask_s + maskShift, nMaskPitch, (nWidth-wMod16) / sizeof(uint16_t), nHeight, bits_per_pixel);
    }
}

// the exported processors: one kernel switch per call
template <MaskMode mode, Processor16 merge_c, Processor16 merge_allow_leftminus1_c>
static void merge16_sse2_t(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch,
                          const uint8_t *pMask, ptrdiff_t nMaskPitch, int nWidth, int nHeight, int bits_per_pixel)
{
  if (bits_per_pixel == 16)
    merge16_t_simd<mode, merge_c, merge_allow_leftminus1_c, false>(pDst, nDstPitch, pSrc1, nSrc1Pitch, pMask, nMaskPitch, nWidth, nHeight, bits_per_pixel);
  else
    merge16_t_simd<mode, merge_c, merge_allow_leftminus1_c, true>(pDst, nDstPitch, pSrc1, nSrc1Pitch, pMask, nMaskPitch, nWidth, nHeight, bits_per_pixel);
}

} // namespace

Processor16 *merge16_sse2 = merge16_sse2_t<MASK444, merge16_t_c<MASK444>, merge16_t_c<MASK444>>;
Processor16 *merge16_luma_420_sse2 = merge16_sse2_t<MASK420, merge16_t_c<MASK420>, merge16_t_c<MASK420>>;
Processor16 *merge16_luma_420_mpeg2_sse2 = merge16_sse2_t<MASK420_MPEG2, merge16_t_c<MASK420_MPEG2>, merge16_t_allow_leftminus1_c<MASK420_MPEG2>>;
Processor16 *merge16_luma_420_topleft_sse2 = merge16_sse2_t<MASK420_TOPLEFT, merge16_t_c<MASK420_TOPLEFT>, merge16_t_allow_leftminus1_c<MASK420_TOPLEFT>>;
Processor16 *merge16_luma_422_sse2 = merge16_sse2_t<MASK422, merge16_t_c<MASK422>, merge16_t_c<MASK422>>;
Processor16 *merge16_luma_422_mpeg2_sse2 = merge16_sse2_t<MASK422_MPEG2, merge16_t_c<MASK422_MPEG2>, merge16_t_allow_leftminus1_c<MASK422_MPEG2>>;

} } } }

#endif // INTEL_INTRINSICS
