#include "merge.h"

#ifdef INTEL_INTRINSICS

#include "../../common/simd_avx2.h"
#include "merge16_c.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Merge {

namespace {

template<MaskMode mode>
MT_FORCEINLINE void mask_columns(const uint8_t *p, ptrdiff_t pitch, ptrdiff_t prev_pitch,
  __m256i &mid, __m256i &right) {
  const auto evenmask = _mm256_set1_epi32(65535);
  auto row = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(p));
  mid = _mm256_and_si256(row, evenmask);
  right = _mm256_srli_epi32(row, 16);
  if constexpr (mode != MASK422_MPEG2) {
    auto below = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(p + pitch));
    if constexpr (mode == MASK420_TOPLEFT) {
      auto above = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(p + prev_pitch));
      mid = _mm256_add_epi32(_mm256_slli_epi32(mid, 1), _mm256_and_si256(above, evenmask));
      right = _mm256_add_epi32(_mm256_slli_epi32(right, 1), _mm256_srli_epi32(above, 16));
    }
    mid = _mm256_add_epi32(mid, _mm256_and_si256(below, evenmask));
    right = _mm256_add_epi32(right, _mm256_srli_epi32(below, 16));
  }
}

MT_FORCEINLINE __m256i previous_column(__m256i right, __m256i previous) {
  // Carry the last column through the 128-bit lane boundary as well as the vector boundary.
  auto carry = _mm256_permute2x128_si256(previous, right, 0x21);
  return _mm256_alignr_epi8(right, carry, 12);
}

template<MaskMode mode>
MT_FORCEINLINE __m256i get_weighted_mask(const uint8_t *p, ptrdiff_t pitch, ptrdiff_t prev_pitch,
  __m256i &previous) {
  constexpr int shift = mode == MASK420_TOPLEFT ? 4 : mode == MASK420_MPEG2 ? 3 : 2;
  const auto rounder = _mm256_set1_epi32(1 << (shift - 1));
  __m256i mid_lo, right_lo, mid_hi, right_hi;
  mask_columns<mode>(p, pitch, prev_pitch, mid_lo, right_lo);
  mask_columns<mode>(p + 32, pitch, prev_pitch, mid_hi, right_hi);
  auto left_lo = previous_column(right_lo, previous);
  auto left_hi = previous_column(right_hi, right_lo);
  previous = right_hi;
  // Horizontal 1-2-1 over column sums: /4 (422), /8 (420), /16 (top-left 420).
  auto lo = _mm256_srli_epi32(_mm256_add_epi32(_mm256_add_epi32(
    _mm256_add_epi32(left_lo, _mm256_slli_epi32(mid_lo, 1)), right_lo), rounder), shift);
  auto hi = _mm256_srli_epi32(_mm256_add_epi32(_mm256_add_epi32(
    _mm256_add_epi32(left_hi, _mm256_slli_epi32(mid_hi, 1)), right_hi), rounder), shift);
  return _mm256_permute4x64_epi64(_mm256_packus_epi32(lo, hi), 0xD8);
}

MT_FORCEINLINE static __m256i get_single_mask_value_420(const __m256i &row1_lo, const __m256i &row1_hi, const __m256i &row2_lo, const __m256i &row2_hi) {
  auto avg_lo = _mm256_avg_epu16(row1_lo, row2_lo);
  auto avg_hi = _mm256_avg_epu16(row1_hi, row2_hi);

  auto avg_lo_sh = _mm256_srli_si256(avg_lo, 2);  // shift one pixel, srli with 2x128 lane is OK here
  auto avg_hi_sh = _mm256_srli_si256(avg_hi, 2);

  avg_lo = _mm256_avg_epu16(avg_lo, avg_lo_sh); // upper 16 bit of each 32 bits are crap, mask it
  avg_lo = _mm256_shuffle_epi8(avg_lo, _mm256_set_epi8(0, 0, 0, 0, 0, 0, 0, 0, 13, 12, 9, 8, 5, 4, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 13, 12, 9, 8, 5, 4, 1, 0));

  avg_hi = _mm256_avg_epu16(avg_hi, avg_hi_sh);
  avg_hi = _mm256_shuffle_epi8(avg_hi, _mm256_set_epi8(13, 12, 9, 8, 5, 4, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 13, 12, 9, 8, 5, 4, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0));
  
  auto avg = simd256_blend_epi8(_mm256_set_epi32(0xFFFFFFFF, 0xFFFFFFFF, 0, 0, 0xFFFFFFFF, 0xFFFFFFFF, 0, 0), avg_hi, avg_lo);

  // avg_lo_lo128->64, avg_hi_lo128->64, avg_lo_hi128->64, avg_hi_hi128->64

  return _mm256_permute4x64_epi64(avg, (0 << 0) + (2 << 2) + (1 << 4) + (3 << 6));
}

MT_FORCEINLINE static __m256i get_single_mask_value_422(const __m256i &row1_lo, const __m256i &row1_hi) {
  auto row1_lo_sh = _mm256_srli_si256(row1_lo, 2);  // shift one pixel, srli with 2x128 lane is OK here
  auto row1_hi_sh = _mm256_srli_si256(row1_hi, 2);

  auto avg_lo = _mm256_avg_epu16(row1_lo, row1_lo_sh); // upper 16 bit of each 32 bits are crap, mask it
  avg_lo = _mm256_shuffle_epi8(avg_lo, _mm256_set_epi8(0, 0, 0, 0, 0, 0, 0, 0, 13, 12, 9, 8, 5, 4, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 13, 12, 9, 8, 5, 4, 1, 0));

  auto avg_hi = _mm256_avg_epu16(row1_hi, row1_hi_sh);
  avg_hi = _mm256_shuffle_epi8(avg_hi, _mm256_set_epi8(13, 12, 9, 8, 5, 4, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 13, 12, 9, 8, 5, 4, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0));

  auto avg = simd256_blend_epi8(_mm256_set_epi32(0xFFFFFFFF, 0xFFFFFFFF, 0, 0, 0xFFFFFFFF, 0xFFFFFFFF, 0, 0), avg_hi, avg_lo);

  // avg_lo_lo128->64, avg_hi_lo128->64, avg_lo_hi128->64, avg_hi_hi128->64

  return _mm256_permute4x64_epi64(avg, (0 << 0) + (2 << 2) + (1 << 4) + (3 << 6));
}

MT_FORCEINLINE static __m256i merge_magic_div_epi32(const __m256i &value, const __m256i &magic, const __m128i &shift) {
  // Recombine the exact quotients from the even and odd unsigned 64-bit reciprocal products.
  auto even = _mm256_srl_epi64(_mm256_mul_epu32(value, magic), shift);
  auto odd = _mm256_srl_epi64(_mm256_mul_epu32(_mm256_srli_epi64(value, 32), magic), shift);
  even = _mm256_shuffle_epi32(even, _MM_SHUFFLE(2, 0, 2, 0));
  odd = _mm256_shuffle_epi32(odd, _MM_SHUFFLE(2, 0, 2, 0));
  return _mm256_unpacklo_epi32(even, odd);
}

MT_FORCEINLINE static __m256i merge_core_simd(const __m256i &dst, const __m256i &src, const __m256i &mask,
                                               const __m256i &max_value, const __m256i &half,
                                               const __m256i &magic, const __m128i &div_shift,
                                               const __m256i &zero) {
  auto dst_lo = _mm256_unpacklo_epi16(dst, zero);
  auto dst_hi = _mm256_unpackhi_epi16(dst, zero);

  auto src_lo = _mm256_unpacklo_epi16(src, zero);
  auto src_hi = _mm256_unpackhi_epi16(src, zero);

  auto mask_lo = _mm256_unpacklo_epi16(mask, zero);
  auto mask_hi = _mm256_unpackhi_epi16(mask, zero);
  auto inverse_lo = _mm256_sub_epi32(max_value, mask_lo);
  auto inverse_hi = _mm256_sub_epi32(max_value, mask_hi);
  auto numerator_lo = _mm256_add_epi32(_mm256_add_epi32(_mm256_mullo_epi32(dst_lo, inverse_lo), _mm256_mullo_epi32(src_lo, mask_lo)), half);
  auto numerator_hi = _mm256_add_epi32(_mm256_add_epi32(_mm256_mullo_epi32(dst_hi, inverse_hi), _mm256_mullo_epi32(src_hi, mask_hi)), half);
  auto result_lo = merge_magic_div_epi32(numerator_lo, magic, div_shift);
  auto result_hi = merge_magic_div_epi32(numerator_hi, magic, div_shift);
  auto result = _mm256_packus_epi32(result_lo, result_hi);
  return result;
}

MT_FORCEINLINE static __m256i get_mask_420_simd(const uint8_t *ptr, ptrdiff_t pitch, int x) {
    x = x*2;

    auto row1_lo = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(reinterpret_cast<const __m256i*>(ptr+x)));
    auto row1_hi = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(reinterpret_cast<const __m256i*>(ptr+x+32)));
    auto row2_lo = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(reinterpret_cast<const __m256i*>(ptr+pitch+x)));
    auto row2_hi = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(reinterpret_cast<const __m256i*>(ptr+pitch+x+32)));

    return get_single_mask_value_420(row1_lo, row1_hi, row2_lo, row2_hi);
}

MT_FORCEINLINE static __m256i get_mask_422_simd(const uint8_t *ptr, int x) {
  x = x * 2;

  auto row1_lo = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(reinterpret_cast<const __m256i*>(ptr + x)));
  auto row1_hi = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(reinterpret_cast<const __m256i*>(ptr + x + 32)));
  
  return get_single_mask_value_422(row1_lo, row1_hi);
}

template <MaskMode mode>
static void merge16_t_simd(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch,
                            const uint8_t *pMask, ptrdiff_t nMaskPitch, int nWidth, int nHeight, int bits_per_pixel)
{
    nWidth *= 2; // really rowsize: width * sizeof(uint16), see also division at C trailer

    int wMod32 = (nWidth / 32) * 32;
    auto pDst_s = pDst;
    auto pSrc1_s = pSrc1;
    auto pMask_s = pMask;

    auto zero = _mm256_setzero_si256();
    const uint32_t max = (1u << bits_per_pixel) - 1;
    const auto max_value = _mm256_set1_epi32(static_cast<int32_t>(max));
    const auto half = _mm256_set1_epi32(static_cast<int32_t>(max / 2));
    const MergeMagicDiv magic_div = get_merge_magic_div(bits_per_pixel);
    const auto magic = _mm256_set1_epi32(static_cast<int32_t>(magic_div.div));
    const auto div_shift = _mm_cvtsi32_si128(32 + magic_div.shift);

    for ( int j = 0; j < nHeight; ++j ) {
        const ptrdiff_t prev_pitch = j == 0 ? 0 : -nMaskPitch;
        __m256i previous = zero;
        if constexpr (mode == MASK420_MPEG2 || mode == MASK420_TOPLEFT || mode == MASK422_MPEG2) {
          if (wMod32 != 0) {
            // Repeat the first column at the left edge, as in the C mask filter.
            int first = reinterpret_cast<const uint16_t*>(pMask)[0];
            if constexpr (mode == MASK420_TOPLEFT)
              first = reinterpret_cast<const uint16_t*>(pMask + prev_pitch)[0] + 2 * first +
                reinterpret_cast<const uint16_t*>(pMask + nMaskPitch)[0];
            else if constexpr (mode == MASK420_MPEG2)
              first += reinterpret_cast<const uint16_t*>(pMask + nMaskPitch)[0];
            previous = _mm256_set1_epi32(first);
          }
        }
        for ( int i = 0; i < wMod32; i+=32 ) {
            auto dst = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(reinterpret_cast<const __m256i*>(pDst+i)));
            auto src = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(reinterpret_cast<const __m256i*>(pSrc1+i)));
            __m256i mask;

            if constexpr (mode == MASK420_MPEG2 || mode == MASK420_TOPLEFT || mode == MASK422_MPEG2) {
              mask = get_weighted_mask<mode>(pMask + i * 2, nMaskPitch, prev_pitch, previous);
            } else if (mode == MASK420) {
              mask = get_mask_420_simd(pMask, nMaskPitch, i); 
            } else if (mode == MASK422) {
              mask = get_mask_422_simd(pMask, i);
            } else {
              mask = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(reinterpret_cast<const __m256i*>(pMask + i)));
            }

            __m256i result = merge_core_simd(dst, src, mask, max_value, half, magic, div_shift, zero);

            _mm256_storeu_si256(reinterpret_cast<__m256i*>(reinterpret_cast<__m256i*>(pDst+i)), result);
        }

        pDst += nDstPitch;
        pSrc1 += nSrc1Pitch;

        if (mode == MASK420 || mode == MASK420_MPEG2 || mode == MASK420_TOPLEFT) {
            pMask += nMaskPitch * 2;
        } else {
            pMask += nMaskPitch; // 422, 444
        }
    }

    if (nWidth > wMod32) {
      const int maskShift = mode == MASK444 ? wMod32 : wMod32 * 2;
      Processor16 *tail = merge16_t_c<mode>;
      if constexpr (mode == MASK420_MPEG2 || mode == MASK420_TOPLEFT || mode == MASK422_MPEG2) {
        if (wMod32 != 0)
          tail = merge16_t_allow_leftminus1_c<mode>;
      }
      tail(pDst_s + wMod32, nDstPitch, pSrc1_s + wMod32, nSrc1Pitch, pMask_s + maskShift, nMaskPitch, (nWidth - wMod32) / sizeof(uint16_t), nHeight, bits_per_pixel);
    }
}

template <MaskMode mode>
static void merge16_avx2_t(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch,
                          const uint8_t *pMask, ptrdiff_t nMaskPitch, int nWidth, int nHeight, int bits_per_pixel)
{
  merge16_t_simd<mode>(pDst, nDstPitch, pSrc1, nSrc1Pitch, pMask, nMaskPitch, nWidth, nHeight, bits_per_pixel);
}

} // namespace

Processor16 *merge16_avx2 = merge16_avx2_t<MASK444>;
Processor16 *merge16_luma_420_avx2 = merge16_avx2_t<MASK420>;
Processor16 *merge16_luma_422_avx2 = merge16_avx2_t<MASK422>;
Processor16 *merge16_luma_420_mpeg2_avx2 = merge16_avx2_t<MASK420_MPEG2>;
Processor16 *merge16_luma_420_topleft_avx2 = merge16_avx2_t<MASK420_TOPLEFT>;
Processor16 *merge16_luma_422_mpeg2_avx2 = merge16_avx2_t<MASK422_MPEG2>;

} } } }

#endif // INTEL_INTRINSICS