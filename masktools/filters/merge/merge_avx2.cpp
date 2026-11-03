#include "merge.h"

#ifdef INTEL_INTRINSICS

#include "merge_c.h"
#include "../../common/simd_avx2.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Merge {

namespace {

   MT_FORCEINLINE __m256i merge_avx2_core(uint8_t *pDst, const uint8_t *pSrc, const __m256i& mask_lo, const __m256i& mask_hi,
      const __m256i& zero) {
      auto dst = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pDst));
      auto dst_lo = _mm256_unpacklo_epi8(dst, zero);
      auto dst_hi = _mm256_unpackhi_epi8(dst, zero);
      
      auto src = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pSrc));
      auto src1_lo = _mm256_unpacklo_epi8(src, zero);
      auto src1_hi = _mm256_unpackhi_epi8(src, zero);

      auto inverse_lo = _mm256_sub_epi16(_mm256_set1_epi16(255), mask_lo);
      auto inverse_hi = _mm256_sub_epi16(_mm256_set1_epi16(255), mask_hi);
      auto rounder = _mm256_set1_epi16(127);
      // 0x8081 = ceil(2^23 / 255); mulhi plus this shift computes exact division by 255.
      auto magic = _mm256_set1_epi16(static_cast<short>(0x8081));
      auto numerator_lo = _mm256_add_epi16(_mm256_add_epi16(_mm256_mullo_epi16(dst_lo, inverse_lo), _mm256_mullo_epi16(src1_lo, mask_lo)), rounder);
      auto numerator_hi = _mm256_add_epi16(_mm256_add_epi16(_mm256_mullo_epi16(dst_hi, inverse_hi), _mm256_mullo_epi16(src1_hi, mask_hi)), rounder);
      auto result_lo = _mm256_srli_epi16(_mm256_mulhi_epu16(numerator_lo, magic), 7);
      auto result_hi = _mm256_srli_epi16(_mm256_mulhi_epu16(numerator_hi, magic), 7);
      auto result = _mm256_packus_epi16(result_lo, result_hi);
      return result;
   }

   enum class WeightedMask { M420, TopLeft420, M422 };

   template<WeightedMask mode>
   MT_FORCEINLINE void mask_columns(const uint8_t *p, ptrdiff_t pitch, ptrdiff_t prev_pitch,
     __m256i &mid, __m256i &right) {
     auto row = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(p));
     const auto evenmask = _mm256_set1_epi16(255);
     mid = _mm256_and_si256(row, evenmask);
     right = _mm256_srli_epi16(row, 8);
     if constexpr (mode != WeightedMask::M422) {
       auto below = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(p + pitch));
       if constexpr (mode == WeightedMask::TopLeft420) {
         auto above = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(p + prev_pitch));
         mid = _mm256_add_epi16(_mm256_slli_epi16(mid, 1), _mm256_and_si256(above, evenmask));
         right = _mm256_add_epi16(_mm256_slli_epi16(right, 1), _mm256_srli_epi16(above, 8));
       }
       mid = _mm256_add_epi16(mid, _mm256_and_si256(below, evenmask));
       right = _mm256_add_epi16(right, _mm256_srli_epi16(below, 8));
     }
   }

   MT_FORCEINLINE __m256i previous_column(__m256i right, __m256i previous) {
     // Carry across both 128-bit lanes and vector blocks, not just within each lane.
     auto carry = _mm256_permute2x128_si256(previous, right, 0x21);
     return _mm256_alignr_epi8(right, carry, 14);
   }

   template<WeightedMask mode>
   void merge_weighted_avx2_t(uint8_t *pDst, ptrdiff_t dst_pitch, const uint8_t *pSrc, ptrdiff_t src_pitch,
     const uint8_t *pMask, ptrdiff_t mask_pitch, int width, int height) {
     constexpr int shift = mode == WeightedMask::TopLeft420 ? 4 : mode == WeightedMask::M420 ? 3 : 2;
     const int vector_width = width / 32 * 32;
     auto dst_start = pDst;
     auto src_start = pSrc;
     auto mask_start = pMask;
     const auto zero = _mm256_setzero_si256();
     const auto rounder = _mm256_set1_epi16(1 << (shift - 1));
     for (int y = 0; y < height; ++y) {
       const ptrdiff_t prev_pitch = y == 0 ? 0 : -mask_pitch;
       __m256i previous = zero;
       if (vector_width != 0) {
         // Repeat the first column at the left edge, as in the C mask filter.
         int first = pMask[0];
         if constexpr (mode == WeightedMask::TopLeft420)
           first = pMask[prev_pitch] + 2 * first + pMask[mask_pitch];
         else if constexpr (mode == WeightedMask::M420)
           first += pMask[mask_pitch];
         previous = _mm256_set1_epi16(static_cast<short>(first));
       }
       for (int x = 0; x < vector_width; x += 32) {
         __m256i mid_lo, right_lo, mid_hi, right_hi;
         mask_columns<mode>(pMask + x * 2, mask_pitch, prev_pitch, mid_lo, right_lo);
         mask_columns<mode>(pMask + x * 2 + 32, mask_pitch, prev_pitch, mid_hi, right_hi);
         auto left_lo = previous_column(right_lo, previous);
         auto left_hi = previous_column(right_hi, right_lo);
         previous = right_hi;
         // Horizontal 1-2-1 over column sums: /4 (422), /8 (420), /16 (top-left 420).
         auto lo = _mm256_srli_epi16(_mm256_add_epi16(_mm256_add_epi16(
           _mm256_add_epi16(left_lo, _mm256_slli_epi16(mid_lo, 1)), right_lo), rounder), shift);
         auto hi = _mm256_srli_epi16(_mm256_add_epi16(_mm256_add_epi16(
           _mm256_add_epi16(left_hi, _mm256_slli_epi16(mid_hi, 1)), right_hi), rounder), shift);
         // Match the lane-local byte unpacks in the blend core.
         auto mask_lo = _mm256_permute2x128_si256(lo, hi, 0x20);
         auto mask_hi = _mm256_permute2x128_si256(lo, hi, 0x31);
         _mm256_storeu_si256(reinterpret_cast<__m256i*>(pDst + x),
           merge_avx2_core(pDst + x, pSrc + x, mask_lo, mask_hi, zero));
       }
       pDst += dst_pitch;
       pSrc += src_pitch;
       pMask += mode == WeightedMask::M422 ? mask_pitch : mask_pitch * 2;
     }
     if (width > vector_width) {
       Processor *tail;
       if constexpr (mode == WeightedMask::TopLeft420)
         tail = vector_width ? merge_luma_420_topleft_allow_leftminus1_t_c : merge_luma_420_topleft_t_c;
       else if constexpr (mode == WeightedMask::M420)
         tail = vector_width ? merge_luma_420_mpeg2_allow_leftminus1_t_c : merge_luma_420_mpeg2_t_c;
       else
         tail = vector_width ? merge_luma_422_mpeg2_allow_leftminus1_t_c : merge_luma_422_mpeg2_t_c;
       tail(dst_start + vector_width, dst_pitch, src_start + vector_width, src_pitch,
         mask_start + vector_width * 2, mask_pitch, width - vector_width, height);
     }
   }

   void merge_avx2_t(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch,
      const uint8_t *pMask, ptrdiff_t nSrc2Pitch, int nWidth, int nHeight)
   {
      int wMod32 = (nWidth / 32) * 32;
      auto pDst_s = pDst;
      auto pSrc1_s = pSrc1;
      auto pMask_s = pMask;
      auto zero = _mm256_setzero_si256();
      for (int j = 0; j < nHeight; ++j) {
         for (int i = 0; i < wMod32; i += 32) {
            auto src2 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pMask + i));
            auto mask_t1 = _mm256_unpacklo_epi8(src2, zero);
            auto mask_t2 = _mm256_unpackhi_epi8(src2, zero);

            auto result = merge_avx2_core(pDst + i, pSrc1 + i, mask_t1, mask_t2, zero);
            _mm256_storeu_si256(reinterpret_cast<__m256i*>(pDst + i), result);
         }
         pDst += nDstPitch;
         pSrc1 += nSrc1Pitch;
         pMask += nSrc2Pitch;
      }

      if (nWidth > wMod32) {
         merge_t_c(pDst_s + wMod32, nDstPitch, pSrc1_s + wMod32, nSrc1Pitch, pMask_s + wMod32, nSrc2Pitch, nWidth - wMod32, nHeight);
      }
   }
   
   void merge_luma_420_avx2_t(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch,
     const uint8_t *pMask, ptrdiff_t nSrc2Pitch, int nWidth, int nHeight)
   {
     int wMod32 = (nWidth / 32) * 32;
     auto pDst_s = pDst;
     auto pSrc1_s = pSrc1;
     auto pMask_s = pMask;
     auto v255 = _mm256_set1_epi16(0x00FF);
#pragma warning(disable: 4309)
#pragma warning(default: 4309)
     auto zero = _mm256_setzero_si256();
     for (int j = 0; j < nHeight; ++j) {
       for (int i = 0; i < wMod32; i += 32) {
         // preparing mask
         auto src2_row1_t1 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pMask + i * 2));
         auto src2_row1_t2 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pMask + i * 2 + 32));
         auto src2_row2_t1 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pMask + nSrc2Pitch + i * 2));
         auto src2_row2_t2 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pMask + nSrc2Pitch + i * 2 + 32));
         auto avg_t1 = _mm256_avg_epu8(src2_row1_t1, src2_row2_t1);
         auto avg_t2 = _mm256_avg_epu8(src2_row1_t2, src2_row2_t2);
         auto shifted_t1 = _mm256_srli_si256(avg_t1,1); // srli with 2x128 lane is OK here
         auto shifted_t2 = _mm256_srli_si256(avg_t2,1);
         avg_t1 = _mm256_avg_epu8(avg_t1, shifted_t1);
         avg_t2 = _mm256_avg_epu8(avg_t2, shifted_t2);
         // adjust 128bit lanes to match 128bit lane-aware unpacks inside _core
         auto mask_t1 = _mm256_permute2x128_si256(avg_t1, avg_t2, (0 << 0) + (2 << 4)); // 0: src1lo 1:src1hi 2:src2lo 3:src2hi
         auto mask_t2 = _mm256_permute2x128_si256(avg_t1, avg_t2, (1 << 0) + (3 << 4));
         mask_t1 = _mm256_and_si256(mask_t1, v255);
         mask_t2 = _mm256_and_si256(mask_t2, v255);
         auto result = merge_avx2_core(pDst + i, pSrc1 + i, mask_t1, mask_t2, zero);

         _mm256_storeu_si256(reinterpret_cast<__m256i*>(pDst + i), result);
       }
       pDst += nDstPitch;
       pSrc1 += nSrc1Pitch;
       pMask += nSrc2Pitch * 2;
     }
     if (nWidth > wMod32) {
       // pMask offset: mask is not subsampled -> width*2
       merge_luma_420_t_c(pDst_s + wMod32, nDstPitch, pSrc1_s + wMod32, nSrc1Pitch, pMask_s + wMod32 * 2, nSrc2Pitch, nWidth - wMod32, nHeight);
     }
   }

   void merge_luma_422_avx2_t(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch,
     const uint8_t *pMask, ptrdiff_t nSrc2Pitch, int nWidth, int nHeight)
   {
     int wMod32 = (nWidth / 32) * 32;
     auto pDst_s = pDst;
     auto pSrc1_s = pSrc1;
     auto pMask_s = pMask;
     auto v255 = _mm256_set1_epi16(0x00FF);
     auto zero = _mm256_setzero_si256();
#pragma warning(disable: 4309)
#pragma warning(default: 4309)
     for (int j = 0; j < nHeight; ++j) {
       for (int i = 0; i < wMod32; i += 32) {
         // preparing mask
         auto src2_row1_t1 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pMask + i * 2));
         auto src2_row1_t2 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pMask + i * 2 + 32));
         auto shifted_t1 = _mm256_srli_si256(src2_row1_t1, 1); // srli with 2x128 lane is OK here
         auto shifted_t2 = _mm256_srli_si256(src2_row1_t2, 1);
         auto avg_t1 = _mm256_avg_epu8(src2_row1_t1, shifted_t1);
         auto avg_t2 = _mm256_avg_epu8(src2_row1_t2, shifted_t2);
         // adjust 128bit lanes to match 128bit lane-aware unpacks inside _core
         auto mask_t1 = _mm256_permute2x128_si256(avg_t1, avg_t2, (0 << 0) + (2 << 4)); // 0: src1lo 1:src1hi 2:src2lo 3:src2hi
         auto mask_t2 = _mm256_permute2x128_si256(avg_t1, avg_t2, (1 << 0) + (3 << 4)); // 0: src1lo 1:src1hi 2:src2lo 3:src2hi
         mask_t1 = _mm256_and_si256(mask_t1, v255);
         mask_t2 = _mm256_and_si256(mask_t2, v255);

         auto result = merge_avx2_core(pDst + i, pSrc1 + i, mask_t1, mask_t2, zero);

         _mm256_storeu_si256(reinterpret_cast<__m256i*>(pDst + i), result);
       }
       pDst += nDstPitch;
       pSrc1 += nSrc1Pitch;
       pMask += nSrc2Pitch;
     }
     if (nWidth > wMod32) {
       // pMask offset: mask is not subsampled -> width*2
       merge_luma_422_t_c(pDst_s + wMod32, nDstPitch, pSrc1_s + wMod32, nSrc1Pitch, pMask_s + wMod32 * 2, nSrc2Pitch, nWidth - wMod32, nHeight);
     }
   }

   // 411: the masks of 16 chroma pixels from 64 luma mask pixels of the row, as words (lo: 0-7, hi: 8-15);
   // pair averages of pair averages, as the C version
   MT_FORCEINLINE void mask_411_sse2(const uint8_t *pMask, __m128i &mask_lo, __m128i &mask_hi) {
     auto v255 = _mm_set1_epi16(0x00FF);
     auto m0 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pMask));
     auto m1 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pMask + 16));
     auto m2 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pMask + 32));
     auto m3 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pMask + 48));
     // pair averages at the even bytes
     auto p0 = _mm_and_si128(_mm_avg_epu8(m0, _mm_srli_si128(m0, 1)), v255);
     auto p1 = _mm_and_si128(_mm_avg_epu8(m1, _mm_srli_si128(m1, 1)), v255);
     auto p2 = _mm_and_si128(_mm_avg_epu8(m2, _mm_srli_si128(m2, 1)), v255);
     auto p3 = _mm_and_si128(_mm_avg_epu8(m3, _mm_srli_si128(m3, 1)), v255);
     auto p01 = _mm_packus_epi16(p0, p1);
     auto p23 = _mm_packus_epi16(p2, p3);
     // and their pair averages
     mask_lo = _mm_and_si128(_mm_avg_epu8(p01, _mm_srli_si128(p01, 1)), v255);
     mask_hi = _mm_and_si128(_mm_avg_epu8(p23, _mm_srli_si128(p23, 1)), v255);
   }

   void merge_luma_411_avx2_t(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch,
     const uint8_t *pMask, ptrdiff_t nSrc2Pitch, int nWidth, int nHeight)
   {
     int wMod32 = (nWidth / 32) * 32;
     auto pDst_s = pDst;
     auto pSrc1_s = pSrc1;
     auto pMask_s = pMask;
     auto zero = _mm256_setzero_si256();
#pragma warning(disable: 4309)
#pragma warning(default: 4309)
     for (int j = 0; j < nHeight; ++j) {
       for (int i = 0; i < wMod32; i += 32) {
         // preparing mask: two halves of 16 pixels
         __m128i lo0, hi0, lo1, hi1;
         mask_411_sse2(pMask + i * 4, lo0, hi0);
         mask_411_sse2(pMask + i * 4 + 64, lo1, hi1);
         auto mask_t1 = _mm256_set_m128i(lo1, lo0);
         auto mask_t2 = _mm256_set_m128i(hi1, hi0);

         auto result = merge_avx2_core(pDst + i, pSrc1 + i, mask_t1, mask_t2, zero);

         _mm256_storeu_si256(reinterpret_cast<__m256i*>(pDst + i), result);
       }
       pDst += nDstPitch;
       pSrc1 += nSrc1Pitch;
       pMask += nSrc2Pitch;
     }
     if (nWidth > wMod32) {
       merge_luma_411_t_c(pDst_s + wMod32, nDstPitch, pSrc1_s + wMod32, nSrc1Pitch, pMask_s + wMod32 * 4, nSrc2Pitch, nWidth - wMod32, nHeight);
     }
   }

} // namespace

   Processor *merge_avx2 = merge_avx2_t;
   Processor *merge_luma_420_mpeg2_avx2 = merge_weighted_avx2_t<WeightedMask::M420>;
   Processor *merge_luma_420_topleft_avx2 = merge_weighted_avx2_t<WeightedMask::TopLeft420>;
   Processor *merge_luma_422_mpeg2_avx2 = merge_weighted_avx2_t<WeightedMask::M422>;
   Processor *merge_luma_420_avx2 = merge_luma_420_avx2_t;
   Processor *merge_luma_422_avx2 = merge_luma_422_avx2_t;
   Processor *merge_luma_411_avx2 = merge_luma_411_avx2_t;

} } } }

#endif // INTEL_INTRINSICS