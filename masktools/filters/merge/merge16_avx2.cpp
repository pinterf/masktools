#include "merge.h"

#ifdef INTEL_INTRINSICS

#include "../../common/simd_avx2.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Merge {

namespace {

/* Common */

// no MASK420_MPEG2, MASK420_TOPLEFT and MASK422_MPEG2 support in AVX2

enum MaskMode {
    MASK420,
    MASK422,
    MASK444
};

template<int bits_per_pixel>
MT_FORCEINLINE static uint16_t merge16_core_avx2_c(uint16_t dst, uint16_t src, uint16_t mask) {
    if (mask == 0) {
        return dst;
    } else if (mask == ((1 << bits_per_pixel) - 1)) {
        return src;
    } else {
      if (bits_per_pixel < 16)
        return dst + (((src - dst) * mask) >> bits_per_pixel);
      else
        return dst + (((src - dst) * (mask >> 1)) >> (bits_per_pixel - 1)); // >> 1: avoid overflow
    }
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

template<int bits_per_pixel>
MT_FORCEINLINE static __m256i merge_core_simd(const __m256i &dst, const __m256i &src, const __m256i &mask, const __m256i &max_pixel_value, const __m256i &zero) {
  auto dst_lo = _mm256_unpacklo_epi16(dst, zero);
  auto dst_hi = _mm256_unpackhi_epi16(dst, zero);

  auto src_lo = _mm256_unpacklo_epi16(src, zero);
  auto src_hi = _mm256_unpackhi_epi16(src, zero);

  auto diff_lo = _mm256_sub_epi32(src_lo, dst_lo); // diff = src-dst
  auto diff_hi = _mm256_sub_epi32(src_hi, dst_hi);

  auto mask_lo = _mm256_unpacklo_epi16(mask, zero);
  auto mask_hi = _mm256_unpackhi_epi16(mask, zero);

  // return dst +(((src - dst) * (mask >> 1)) >> (bits_per_pixel - 1));

  __m256i lerp_lo, lerp_hi;
  __m256i result_lo, result_hi;

  if (bits_per_pixel == 16) {
    auto smask_lo = _mm256_srai_epi32(mask_lo, 1); // smask = mask>>1 to avoid int mul overflow
    auto smask_hi = _mm256_srai_epi32(mask_hi, 1);

    lerp_lo = simd256_mullo_epi32(diff_lo, smask_lo);
    lerp_lo = _mm256_srai_epi32(lerp_lo, bits_per_pixel - 1); // for n bit, shr (n-1), scale back

    lerp_hi = simd256_mullo_epi32(diff_hi, smask_hi);
    lerp_hi = _mm256_srai_epi32(lerp_hi, bits_per_pixel - 1);

    result_lo = _mm256_add_epi32(dst_lo, lerp_lo);
    result_hi = _mm256_add_epi32(dst_hi, lerp_hi);
  }
  else {
    lerp_lo = simd256_mullo_epi32(diff_lo, mask_lo);
    lerp_lo = _mm256_srai_epi32(lerp_lo, bits_per_pixel);
    result_lo = _mm256_add_epi32(dst_lo, lerp_lo);

    lerp_hi = simd256_mullo_epi32(diff_hi, mask_hi);
    lerp_hi = _mm256_srai_epi32(lerp_hi, bits_per_pixel);
    result_hi = _mm256_add_epi32(dst_hi, lerp_hi);
  }

  auto result = _mm256_packus_epi32(result_lo, result_hi);
  if (bits_per_pixel < 16) // otherwise no clamp needed
    result = _mm256_min_epu16(result, max_pixel_value); 

  auto mask_FFFF = _mm256_cmpeq_epi16(mask, max_pixel_value); // mask == max ? FFFF : 0000
  auto mask_zero = _mm256_cmpeq_epi16(mask, zero);
  result = simd256_blend_epi8(mask_zero, dst, result); // ensure that zero mask value returns dst
  result = simd256_blend_epi8(mask_FFFF, src, result); // ensure that max mask value returns src

  return result;
}

/* Native */

MT_FORCEINLINE static uint16_t get_mask_420_c(const uint8_t *ptr, ptrdiff_t pitch, int x) {
    x = x*2;

    return (((reinterpret_cast<const uint16_t*>(ptr)[x] + reinterpret_cast<const uint16_t*>(ptr+pitch)[x] + 1) >> 1) + 
        ((reinterpret_cast<const uint16_t*>(ptr)[x+1] +  reinterpret_cast<const uint16_t*>(ptr+pitch)[x+1] + 1) >> 1) + 1) >> 1;
}

MT_FORCEINLINE static uint16_t get_mask_422_c(const uint8_t *ptr, int x) {
  x = x * 2;

  return (reinterpret_cast<const uint16_t*>(ptr)[x] + reinterpret_cast<const uint16_t*>(ptr)[x + 1] + 1) >> 1;
}

template<MaskMode mode, int bits_per_pixel>
static void merge16_t_c(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch,
                         const uint8_t *pMask, ptrdiff_t nMaskPitch, int nWidth, int nHeight, int /*nOrigHeight*/)
{
    for ( int y = 0; y < nHeight; ++y )
    {
        for ( int x = 0; x < nWidth; ++x ) { 
            uint16_t dst = reinterpret_cast<const uint16_t*>(pDst)[x];
            uint16_t src = reinterpret_cast<const uint16_t*>(pSrc1)[x];
            uint16_t mask;

            if (mode == MASK420) {
                mask = get_mask_420_c(pMask, nMaskPitch, x);
            } else if(mode == MASK422)
                mask = get_mask_422_c(pMask, x);
            else {
                mask = reinterpret_cast<const uint16_t*>(pMask)[x];
            }

            uint16_t output = merge16_core_avx2_c<bits_per_pixel>(dst, src, mask);

            reinterpret_cast<uint16_t*>(pDst)[x] = output;
        }
        pDst += nDstPitch;
        pSrc1 += nSrc1Pitch;

        if (mode == MASK420) {
            pMask += nMaskPitch * 2;
        } else {
            pMask += nMaskPitch; // 422, 444
        }
    }
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

template <MaskMode mode, Processor16 merge_c, int bits_per_pixel>
static void merge16_t_simd(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch,
                            const uint8_t *pMask, ptrdiff_t nMaskPitch, int nWidth, int nHeight, int nOrigHeight)
{
    nWidth *= 2; // really rowsize: width * sizeof(uint16), see also division at C trailer

    int wMod32 = (nWidth / 32) * 32;
    auto pDst_s = pDst;
    auto pSrc1_s = pSrc1;
    auto pMask_s = pMask;

    int max = (1 << bits_per_pixel) - 1;

    auto zero = _mm256_setzero_si256();
#pragma warning(disable: 4310)
    auto max_pixel_value = _mm256_set1_epi16((short)max); // for checking max
#pragma warning(default: 4310)

    for ( int j = 0; j < nHeight; ++j ) {
        for ( int i = 0; i < wMod32; i+=32 ) {
            auto dst = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(reinterpret_cast<const __m256i*>(pDst+i)));
            auto src = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(reinterpret_cast<const __m256i*>(pSrc1+i)));
            __m256i mask;

            if (mode == MASK420) {
              mask = get_mask_420_simd(pMask, nMaskPitch, i); 
            } else if (mode == MASK422) {
              mask = get_mask_422_simd(pMask, i);
            } else {
              mask = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(reinterpret_cast<const __m256i*>(pMask + i)));
            }

            __m256i result = merge_core_simd<bits_per_pixel>(dst, src, mask, max_pixel_value, zero);

            _mm256_storeu_si256(reinterpret_cast<__m256i*>(reinterpret_cast<__m256i*>(pDst+i)), result);
        }

        pDst += nDstPitch;
        pSrc1 += nSrc1Pitch;

        if (mode == MASK420) {
            pMask += nMaskPitch * 2;
        } else {
            pMask += nMaskPitch; // 422, 444
        }
    }

    if (nWidth > wMod32) {
      const int maskShift = (mode == MASK420 || mode == MASK422) ? wMod32 * 2 : wMod32;
      merge_c(pDst_s + wMod32, nDstPitch, pSrc1_s + wMod32, nSrc1Pitch, pMask_s + maskShift, nMaskPitch, (nWidth - wMod32) / sizeof(uint16_t), nHeight, nOrigHeight);
    }
}

/* Native */
// specialize them
// 4:1:1 is only in 8 bits
// no MPEG2 option here in 16bit AVX2
#define MAKE_TEMPLATES(bits_per_pixel) \
template void merge16_t_c<MASK444, bits_per_pixel>(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch, const uint8_t *pMask, ptrdiff_t nMaskPitch, int nWidth, int nHeight, int nOrigHeight); \
template void merge16_t_c<MASK420, bits_per_pixel>(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch, const uint8_t *pMask, ptrdiff_t nMaskPitch, int nWidth, int nHeight, int nOrigHeight); \
template void merge16_t_c<MASK422, bits_per_pixel>(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch, const uint8_t *pMask, ptrdiff_t nMaskPitch, int nWidth, int nHeight, int nOrigHeight); \
template void merge16_t_simd<MASK444, merge16_t_c<MASK444, bits_per_pixel>, bits_per_pixel>(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch, const uint8_t *pMask, ptrdiff_t nMaskPitch, int nWidth, int nHeight, int nOrigHeight); \
template void merge16_t_simd<MASK420, merge16_t_c<MASK420, bits_per_pixel>, bits_per_pixel>(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch, const uint8_t *pMask, ptrdiff_t nMaskPitch, int nWidth, int nHeight, int nOrigHeight); \
template void merge16_t_simd<MASK422, merge16_t_c<MASK422, bits_per_pixel>, bits_per_pixel>(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch, const uint8_t *pMask, ptrdiff_t nMaskPitch, int nWidth, int nHeight, int nOrigHeight); \

MAKE_TEMPLATES(10)
MAKE_TEMPLATES(12)
MAKE_TEMPLATES(14)
MAKE_TEMPLATES(16)
#undef MAKE_TEMPLATES

} // namespace

#define MAKE_EXPORTS(bits_per_pixel) \
Processor16 *merge16_##bits_per_pixel##_avx2 = merge16_t_simd< MASK444, merge16_t_c<MASK444, bits_per_pixel>, bits_per_pixel>; \
Processor16 *merge16_luma_420_##bits_per_pixel##_avx2 = merge16_t_simd<MASK420, merge16_t_c<MASK420, bits_per_pixel>, bits_per_pixel>; \
Processor16 *merge16_luma_422_##bits_per_pixel##_avx2 = merge16_t_simd<MASK422, merge16_t_c<MASK422, bits_per_pixel>, bits_per_pixel>; \

MAKE_EXPORTS(10)
MAKE_EXPORTS(12)
MAKE_EXPORTS(14)
MAKE_EXPORTS(16)
#undef MAKE_EXPORTS

} } } }

#endif // INTEL_INTRINSICS