#include "merge.h"

#ifdef INTEL_INTRINSICS

#include "../../common/simd.h"
#include "merge16_c.h"
#include "merge_410_simd.h"

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

MT_FORCEINLINE static __m128i merge_magic_div_epi32(__m128i value, const __m128i &magic, const __m128i &shift) {
  // Recombine the exact quotients from the even and odd unsigned 64-bit reciprocal products.
  auto even = _mm_srl_epi64(_mm_mul_epu32(value, magic), shift);
  auto odd = _mm_srl_epi64(_mm_mul_epu32(_mm_srli_si128(value, 4), magic), shift);
  even = _mm_shuffle_epi32(even, _MM_SHUFFLE(2, 0, 2, 0));
  odd = _mm_shuffle_epi32(odd, _MM_SHUFFLE(2, 0, 2, 0));
  return _mm_unpacklo_epi32(even, odd);
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

MT_FORCEINLINE static __m128i merge_core_simd(const __m128i &dst, const __m128i &src, const __m128i &mask,
                                               const __m128i &max_value, const __m128i &half,
                                               const __m128i &magic, const __m128i &div_shift) {
  auto dst_lo = _mm_unpacklo_epi16(dst, _mm_setzero_si128());
  auto dst_hi = _mm_unpackhi_epi16(dst, _mm_setzero_si128());
  auto src_lo = _mm_unpacklo_epi16(src, _mm_setzero_si128());
  auto src_hi = _mm_unpackhi_epi16(src, _mm_setzero_si128());
  auto mask_lo = _mm_unpacklo_epi16(mask, _mm_setzero_si128());
  auto mask_hi = _mm_unpackhi_epi16(mask, _mm_setzero_si128());
  auto inverse_lo = _mm_sub_epi32(max_value, mask_lo);
  auto inverse_hi = _mm_sub_epi32(max_value, mask_hi);
  auto dst_weighted_lo = simd_mullo_epi32<CPU_SSE2>(dst_lo, inverse_lo);
  auto src_weighted_lo = simd_mullo_epi32<CPU_SSE2>(src_lo, mask_lo);
  auto dst_weighted_hi = simd_mullo_epi32<CPU_SSE2>(dst_hi, inverse_hi);
  auto src_weighted_hi = simd_mullo_epi32<CPU_SSE2>(src_hi, mask_hi);
  auto numerator_lo = _mm_add_epi32(_mm_add_epi32(dst_weighted_lo, src_weighted_lo), half);
  auto numerator_hi = _mm_add_epi32(_mm_add_epi32(dst_weighted_hi, src_weighted_hi), half);
  auto result_lo = merge_magic_div_epi32(numerator_lo, magic, div_shift);
  auto result_hi = merge_magic_div_epi32(numerator_hi, magic, div_shift);
  return simd_packus_epi32<CPU_SSE2>(result_lo, result_hi);
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

MT_FORCEINLINE static __m128i get_mask_411_simd(const uint8_t *ptr, int x) {
  ptr += x * 4; // x is a destination byte offset; the mask is four times wider.
  auto pairs_lo = get_single_mask_value_422(
    _mm_loadu_si128(reinterpret_cast<const __m128i*>(ptr)),
    _mm_loadu_si128(reinterpret_cast<const __m128i*>(ptr + 16)));
  auto pairs_hi = get_single_mask_value_422(
    _mm_loadu_si128(reinterpret_cast<const __m128i*>(ptr + 32)),
    _mm_loadu_si128(reinterpret_cast<const __m128i*>(ptr + 48)));
  // Two rounded pair reductions, exactly as get_mask_411_c (not a single rounded /4).
  return get_single_mask_value_422(pairs_lo, pairs_hi);
}

MT_FORCEINLINE static __m128i get_mask_422_mpeg2_simd(const uint8_t *ptr, int x, __m128i &right_hi) {
  x = x * 2;

  auto row1_lo = _mm_loadu_si128(reinterpret_cast<const __m128i*>(ptr + x));
  auto row1_hi = _mm_loadu_si128(reinterpret_cast<const __m128i*>(ptr + x + 16));

  return get_single_mask_value_422_mpeg2(row1_lo, row1_hi, right_hi); // right: in-out
}

template<bool top_left>
MT_FORCEINLINE __m128i get_mask_440_simd(const uint8_t *ptr, ptrdiff_t pitch, ptrdiff_t prev_pitch) {
  auto mid = _mm_loadu_si128(reinterpret_cast<const __m128i*>(ptr));
  auto below = _mm_loadu_si128(reinterpret_cast<const __m128i*>(ptr + pitch));
  if constexpr (!top_left)
    return _mm_avg_epu16(mid, below);
  else {
    auto above = _mm_loadu_si128(reinterpret_cast<const __m128i*>(ptr + prev_pitch));
    const auto zero = _mm_setzero_si128();
    // Widen before summing: the 1-2-1 numerator can exceed 16 bits.
    auto lo = _mm_add_epi32(_mm_unpacklo_epi16(above, zero), _mm_unpacklo_epi16(below, zero));
    auto hi = _mm_add_epi32(_mm_unpackhi_epi16(above, zero), _mm_unpackhi_epi16(below, zero));
    lo = _mm_add_epi32(_mm_add_epi32(lo, _mm_slli_epi32(_mm_unpacklo_epi16(mid, zero), 1)), _mm_set1_epi32(2));
    hi = _mm_add_epi32(_mm_add_epi32(hi, _mm_slli_epi32(_mm_unpackhi_epi16(mid, zero), 1)), _mm_set1_epi32(2));
    return pack_shifted_epi32<2>(lo, hi);
  }
}

template <MaskMode mode, Processor16 merge_c, Processor16 merge_allow_leftminus1_c>
static void merge16_t_simd(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch,
                            const uint8_t *pMask, ptrdiff_t nMaskPitch, int nWidth, int nHeight, int bits_per_pixel)
{
    nWidth *= 2; // really rowsize: width * sizeof(uint16), see also division at C trailer

    int wMod16 = (nWidth / 16) * 16;
    auto pDst_s = pDst;
    auto pSrc1_s = pSrc1;
    auto pMask_s = pMask;

    const uint32_t max = (1u << bits_per_pixel) - 1;
    const auto max_value = _mm_set1_epi32(static_cast<int32_t>(max));
    const auto half = _mm_set1_epi32(static_cast<int32_t>(max / 2));
    const MergeMagicDiv magic_div = get_merge_magic_div(bits_per_pixel);
    const auto magic = _mm_set1_epi32(static_cast<int32_t>(magic_div.div));
    const auto div_shift = _mm_cvtsi32_si128(32 + magic_div.shift);
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
            else if constexpr (mode == MASK411)
              mask = get_mask_411_simd(pMask, i);
            else if constexpr (mode == MASK440 || mode == MASK440_TOPLEFT)
              mask = get_mask_440_simd<mode == MASK440_TOPLEFT>(pMask + i, nMaskPitch, top_or_same);
            else
              mask = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pMask + i));

            __m128i result = merge_core_simd(dst, src, mask, max_value, half, magic, div_shift);

            _mm_storeu_si128(reinterpret_cast<__m128i*>(pDst+i), result);
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

    if (nWidth > wMod16) {
        const int maskShift = mode == MASK411 ? wMod16 * 4 :
          (mode == MASK444 || mode == MASK440 || mode == MASK440_TOPLEFT) ? wMod16 : wMod16 * 2;
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
  merge16_t_simd<mode, merge_c, merge_allow_leftminus1_c>(pDst, nDstPitch, pSrc1, nSrc1Pitch, pMask, nMaskPitch, nWidth, nHeight, bits_per_pixel);
}

} // namespace

Processor16 *merge16_sse2 = merge16_sse2_t<MASK444, merge16_t_c<MASK444>, merge16_t_c<MASK444>>;
Processor16 *merge16_luma_420_sse2 = merge16_sse2_t<MASK420, merge16_t_c<MASK420>, merge16_t_c<MASK420>>;
Processor16 *merge16_luma_420_mpeg2_sse2 = merge16_sse2_t<MASK420_MPEG2, merge16_t_c<MASK420_MPEG2>, merge16_t_allow_leftminus1_c<MASK420_MPEG2>>;
Processor16 *merge16_luma_420_topleft_sse2 = merge16_sse2_t<MASK420_TOPLEFT, merge16_t_c<MASK420_TOPLEFT>, merge16_t_allow_leftminus1_c<MASK420_TOPLEFT>>;
Processor16 *merge16_luma_422_sse2 = merge16_sse2_t<MASK422, merge16_t_c<MASK422>, merge16_t_c<MASK422>>;
Processor16 *merge16_luma_422_mpeg2_sse2 = merge16_sse2_t<MASK422_MPEG2, merge16_t_c<MASK422_MPEG2>, merge16_t_allow_leftminus1_c<MASK422_MPEG2>>;
Processor16 *merge16_luma_411_sse2 = merge16_sse2_t<MASK411, merge16_t_c<MASK411>, merge16_t_c<MASK411>>;
Processor16 *merge16_luma_440_sse2 = merge16_sse2_t<MASK440, merge16_t_c<MASK440>, merge16_t_c<MASK440>>;
Processor16 *merge16_luma_440_topleft_sse2 = merge16_sse2_t<MASK440_TOPLEFT, merge16_t_c<MASK440_TOPLEFT>, merge16_t_c<MASK440_TOPLEFT>>;
Processor16 *merge16_luma_410_sse2 = merge16_410_t<false, false, Mask410SSE,
  merge16_sse2_t<MASK444, merge16_t_c<MASK444>, merge16_t_c<MASK444>>>;
Processor16 *merge16_luma_410_mpeg2_sse2 = merge16_410_t<true, false, Mask410SSE,
  merge16_sse2_t<MASK444, merge16_t_c<MASK444>, merge16_t_c<MASK444>>>;
Processor16 *merge16_luma_410_topleft_sse2 = merge16_410_t<true, true, Mask410SSE,
  merge16_sse2_t<MASK444, merge16_t_c<MASK444>, merge16_t_c<MASK444>>>;

} } } }

#endif // INTEL_INTRINSICS
