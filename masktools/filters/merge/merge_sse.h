#ifndef __Mt_merge_SSE_H__
#define __Mt_merge_SSE_H__

// merge: x86 kernels, instantiated in merge_sse2.cpp and merge_sse41.cpp

#include "merge.h"
#include "../../common/simd.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Merge {

// C versions, for the right edge
   void merge_luma_420_mpeg2_allow_leftminus1_c(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch,
     const uint8_t *pMask, ptrdiff_t nSrc2Pitch, int nWidth, int nHeight);
   void merge_luma_420_topleft_allow_leftminus1_c(uint8_t* pDst, ptrdiff_t nDstPitch, const uint8_t* pSrc1, ptrdiff_t nSrc1Pitch,
     const uint8_t* pMask, ptrdiff_t nSrc2Pitch, int nWidth, int nHeight);
   void merge_luma_422_mpeg2_allow_leftminus1_c(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch,
     const uint8_t *pMask, ptrdiff_t nSrc2Pitch, int nWidth, int nHeight);

namespace {

   // SSE2 or SSE4 because of blend
   template <MemoryMode mem_mode, CpuFlags flags>
   MT_FORCEINLINE __m128i merge_sse2_core(uint8_t *pDst, const uint8_t *pSrc, const __m128i& mask_lo, const __m128i& mask_hi,
      const __m128i& v128, const __m128i& zero, const __m128i& maxMaskFF) {
      auto dst = simd_load_si128<mem_mode>(pDst);
      auto dst_lo = _mm_unpacklo_epi8(dst, zero);
      auto dst_hi = _mm_unpackhi_epi8(dst, zero);
      
      auto src = simd_load_si128<mem_mode>(pSrc);
      auto src1_lo = _mm_unpacklo_epi8(src, zero);
      auto src1_hi = _mm_unpackhi_epi8(src, zero);

      auto dst_lo_sh = _mm_slli_epi16(dst_lo, 8);
      auto diff_lo = _mm_sub_epi16(src1_lo, dst_lo);
      auto tmp1_lo = _mm_mullo_epi16(diff_lo, mask_lo); // (p2-p1)*mask
      auto tmp2_lo = _mm_or_si128(dst_lo_sh, v128);    // p1<<8 + 128 == p1<<8 | 128
      auto result_lo = _mm_add_epi16(tmp1_lo, tmp2_lo);
      result_lo = _mm_srli_epi16(result_lo, 8);

      auto dst_hi_sh = _mm_slli_epi16(dst_hi, 8);
      auto diff_hi = _mm_sub_epi16(src1_hi, dst_hi);
      auto tmp1_hi = _mm_mullo_epi16(diff_hi, mask_hi); // (p2-p1)*mask
      auto tmp2_hi = _mm_or_si128(dst_hi_sh, v128);    // p1<<8 + 128 == p1<<8 | 128
      auto result_hi = _mm_add_epi16(tmp1_hi, tmp2_hi);
      result_hi = _mm_srli_epi16(result_hi, 8);

      auto result = _mm_packus_epi16(result_lo, result_hi);

      // 2.2.7:
      // when mask is FF, keep src
      // when mask is 00, keep dst
      auto mask = _mm_packus_epi16(mask_lo, mask_hi);
      auto mask_FF = _mm_cmpeq_epi8(mask, maxMaskFF); // mask == max ? FF : 00
      auto mask_00 = _mm_cmpeq_epi8(mask, zero);

      result = simd_blend_epi8<flags>(mask_FF, src, result); // ensure that max mask value returns src
      result = simd_blend_epi8<flags>(mask_00, dst, result); // ensure that zero mask value returns dst

      return result;
   }

   template <MemoryMode mem_mode, CpuFlags flags>
   void merge_sse2_t(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch,
      const uint8_t *pMask, ptrdiff_t nSrc2Pitch, int nWidth, int nHeight)
   {
      int wMod16 = (nWidth / 16) * 16;
      auto pDst_s = pDst;
      auto pSrc1_s = pSrc1;
      auto pMask_s = pMask;
      auto v128 = _mm_set1_epi16(0x0080);
      auto zero = _mm_setzero_si128();
#pragma warning(disable: 4309)
      auto maxMaskFF = _mm_set1_epi8(0xFF);
#pragma warning(default: 4309)
      for (int j = 0; j < nHeight; ++j) {
         for (int i = 0; i < wMod16; i += 16) {
            auto src2 = simd_load_si128<mem_mode>(pMask + i); // original mask
            auto mask_t1 = _mm_unpacklo_epi8(src2, zero);
            auto mask_t2 = _mm_unpackhi_epi8(src2, zero);

            auto result = merge_sse2_core<mem_mode, flags>(pDst + i, pSrc1 + i, mask_t1, mask_t2, v128, zero, maxMaskFF);
            // 2.2.7: applied logic: if mask == 255 -> src returned, mask == 0 -> dst is returned
            // old behaviour:
            // when mask is 255 then second clip's 255 becomes 254 and 0 becomes 1
            // in 16 bit merge this problem is solved: special ensures that max mask returns second clip
            // mask 0 returns first clip

            simd_store_si128<mem_mode>(pDst + i, result);
         }
         pDst += nDstPitch;
         pSrc1 += nSrc1Pitch;
         pMask += nSrc2Pitch;
      }

      if (nWidth > wMod16) {
         merge_c(pDst_s + wMod16, nDstPitch, pSrc1_s + wMod16, nSrc1Pitch, pMask_s + wMod16, nSrc2Pitch, nWidth - wMod16, nHeight);
      }
   }

   template <MemoryMode mem_mode, CpuFlags flags>
   void merge_luma_420_sse2_t(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch,
     const uint8_t *pMask, ptrdiff_t nSrc2Pitch, int nWidth, int nHeight)
   {
     int wMod16 = (nWidth / 16) * 16;
     auto pDst_s = pDst;
     auto pSrc1_s = pSrc1;
     auto pMask_s = pMask;
     auto v255 = _mm_set1_epi16(0x00FF);
     auto v128 = _mm_set1_epi16(0x0080);
     auto zero = _mm_setzero_si128();
#pragma warning(disable: 4309)
     auto maxMaskFF = _mm_set1_epi8(0xFF);
#pragma warning(default: 4309)
     for (int j = 0; j < nHeight; ++j) {
       for (int i = 0; i < wMod16; i += 16) {
         _mm_prefetch(reinterpret_cast<const char*>(pMask) + i * 2 + 64, _MM_HINT_T0);
         _mm_prefetch(reinterpret_cast<const char*>(pMask) + nSrc2Pitch + i * 2 + 64, _MM_HINT_T0);
         // preparing mask
         auto src2_row1_t1 = simd_load_si128<mem_mode>(pMask + i * 2);
         auto src2_row1_t2 = simd_load_si128<mem_mode>(pMask + i * 2 + 16);
         auto src2_row2_t1 = simd_load_si128<mem_mode>(pMask + nSrc2Pitch + i * 2);
         auto src2_row2_t2 = simd_load_si128<mem_mode>(pMask + nSrc2Pitch + i * 2 + 16);
         auto avg_t1 = _mm_avg_epu8(src2_row1_t1, src2_row2_t1);
         auto avg_t2 = _mm_avg_epu8(src2_row1_t2, src2_row2_t2);
         auto shifted_t1 = _mm_srli_si128(avg_t1, 1);
         auto shifted_t2 = _mm_srli_si128(avg_t2, 1);
         // make average but we don't care for every second one
         avg_t1 = _mm_avg_epu8(avg_t1, shifted_t1);
         avg_t2 = _mm_avg_epu8(avg_t2, shifted_t2);
         // mask out to have unsigned 16 bit
         auto mask_t1 = _mm_and_si128(avg_t1, v255);
         auto mask_t2 = _mm_and_si128(avg_t2, v255);

         auto result = merge_sse2_core<mem_mode, flags>(pDst + i, pSrc1 + i, mask_t1, mask_t2, v128, zero, maxMaskFF);

         simd_store_si128<mem_mode>(pDst + i, result);
       }
       pDst += nDstPitch;
       pSrc1 += nSrc1Pitch;
       pMask += nSrc2Pitch * 2;
     }
     if (nWidth > wMod16) {
       merge_luma_420_c(pDst_s + wMod16, nDstPitch, pSrc1_s + wMod16, nSrc1Pitch, pMask_s + wMod16 * 2, nSrc2Pitch, nWidth - wMod16, nHeight);
     }
   }

   template <MemoryMode mem_mode, CpuFlags flags>
   void merge_luma_420_mpeg2_sse2_t(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch,
     const uint8_t *pMask, ptrdiff_t nSrc2Pitch, int nWidth, int nHeight)
   {
     int wMod16 = (nWidth / 16) * 16;
     auto pDst_s = pDst;
     auto pSrc1_s = pSrc1;
     auto pMask_s = pMask;
     auto v128 = _mm_set1_epi16(0x0080);
     auto zero = _mm_setzero_si128();
#pragma warning(disable: 4309)
     auto maxMaskFF = _mm_set1_epi8(0xFF);
#pragma warning(default: 4309)
     for (int j = 0; j < nHeight; ++j) {
       // prepare "right_hi": we need only pixel column #0 as the rightmost word (14-15th byte)
       auto row1 = simd_load_si128<mem_mode>(pMask + 0 * 2);
       auto row2 = simd_load_si128<mem_mode>(pMask + nSrc2Pitch + 0 * 2);
       __m128i right_lo, right_hi;
#pragma warning(disable: 4309)
       auto evenmask = _mm_set1_epi16(0x00FF);
#pragma warning(default: 4309)
       right_hi = _mm_add_epi16(_mm_slli_si128(_mm_and_si128(row1, evenmask), 14), _mm_slli_si128(_mm_and_si128(row2, evenmask), 14));

       for (int i = 0; i < wMod16; i += 16) {
         // preparing mask
         auto row1_lo = simd_load_si128<mem_mode>(pMask + i * 2);
         auto row1_hi = simd_load_si128<mem_mode>(pMask + i * 2 + 16);
         auto row2_lo = simd_load_si128<mem_mode>(pMask + nSrc2Pitch + i * 2);
         auto row2_hi = simd_load_si128<mem_mode>(pMask + nSrc2Pitch + i * 2 + 16);

         auto mid_lo = _mm_add_epi16(_mm_and_si128(row1_lo, evenmask), _mm_and_si128(row2_lo, evenmask));
         auto mid_hi = _mm_add_epi16(_mm_and_si128(row1_hi, evenmask), _mm_and_si128(row2_hi, evenmask));

         auto old_right_hi = right_hi;

         right_lo = _mm_add_epi16(_mm_srli_epi16(row1_lo, 8), _mm_srli_epi16(row2_lo, 8));
         right_hi = _mm_add_epi16(_mm_srli_epi16(row1_hi, 8), _mm_srli_epi16(row2_hi, 8));

         // left: (current_right_lo, right_hi << 1 word, prev_right_hi (1 word)
         auto left_hi = _mm_or_si128(_mm_slli_si128(right_hi, 2), _mm_srli_si128(right_lo, 14)); // 32 byte srli across 128bit registers
         auto left_lo = _mm_or_si128(_mm_slli_si128(right_lo, 2), _mm_srli_si128(old_right_hi, 14)); // shift in from previous right_hi

         // mask: (left + mid*2 + right) / 8. Left, mid and right are already sums of two pixels
         auto four = _mm_set1_epi16(4); // rounder
         auto mask_lo = _mm_srli_epi16(_mm_add_epi16(_mm_add_epi16(_mm_add_epi16(_mm_slli_epi16(mid_lo, 1), left_lo), right_lo), four), 3);
         auto mask_hi = _mm_srli_epi16(_mm_add_epi16(_mm_add_epi16(_mm_add_epi16(_mm_slli_epi16(mid_hi, 1), left_hi), right_hi), four), 3);

         auto result = merge_sse2_core<mem_mode, flags>(pDst + i, pSrc1 + i, mask_lo, mask_hi, v128, zero, maxMaskFF);

         simd_store_si128<mem_mode>(pDst + i, result);
       }
       pDst += nDstPitch;
       pSrc1 += nSrc1Pitch;
       pMask += nSrc2Pitch * 2;
     }
     if (nWidth > wMod16) {
       if (wMod16 != 0) // indexing leftmost - 1 is allowed
         merge_luma_420_mpeg2_allow_leftminus1_c(pDst_s + wMod16, nDstPitch, pSrc1_s + wMod16, nSrc1Pitch, pMask_s + wMod16 * 2, nSrc2Pitch, nWidth - wMod16, nHeight);
       else
         merge_luma_420_mpeg2_c(pDst_s + wMod16, nDstPitch, pSrc1_s + wMod16, nSrc1Pitch, pMask_s + wMod16 * 2, nSrc2Pitch, nWidth - wMod16, nHeight);
     }
   }

   template <MemoryMode mem_mode, CpuFlags flags>
   void merge_luma_420_topleft_sse2_t(uint8_t* pDst, ptrdiff_t nDstPitch, const uint8_t* pSrc1, ptrdiff_t nSrc1Pitch,
     const uint8_t* pMask, ptrdiff_t nSrc2Pitch, int nWidth, int nHeight)
   {
     int wMod16 = (nWidth / 16) * 16;
     auto pDst_s = pDst;
     auto pSrc1_s = pSrc1;
     auto pMask_s = pMask;
     auto v128 = _mm_set1_epi16(0x0080);
     auto zero = _mm_setzero_si128();
#pragma warning(disable: 4309)
     auto maxMaskFF = _mm_set1_epi8(0xFF);
#pragma warning(default: 4309)

     for (int j = 0; j < nHeight; ++j) {
       const auto top_or_same = j == 0 ? 0 : -nSrc2Pitch;
       // no previous line for 'top'

       // prepare "right_hi": we need only pixel column #0 as the rightmost word (14-15th byte)
       auto row0 = simd_load_si128<mem_mode>(pMask + top_or_same + 0 * 2);
       auto row1 = simd_load_si128<mem_mode>(pMask + 0 * 2);
       auto row2 = simd_load_si128<mem_mode>(pMask + nSrc2Pitch + 0 * 2);
       __m128i right_lo, right_hi;
#pragma warning(disable: 4309)
       auto evenmask = _mm_set1_epi16(0x00FF);
#pragma warning(default: 4309)
       auto vmiddle1 = _mm_slli_si128(_mm_and_si128(row1, evenmask), 14);
       auto vmiddle1mul2 = _mm_slli_epi16(vmiddle1, 1); // *2 double weight
       right_hi = _mm_add_epi16(vmiddle1mul2,
         _mm_add_epi16(
           _mm_slli_si128(_mm_and_si128(row0, evenmask), 14),
           _mm_slli_si128(_mm_and_si128(row2, evenmask), 14)
         )
       );

       for (int i = 0; i < wMod16; i += 16) {
         // subsampled 16 pixels take 2x16 pixels of original luma mask

         // preparing mask
         auto row0_lo = simd_load_si128<mem_mode>(pMask + top_or_same + i * 2);
         auto row0_hi = simd_load_si128<mem_mode>(pMask + top_or_same + i * 2 + 16);
         auto row1_lo = simd_load_si128<mem_mode>(pMask + i * 2);
         auto row1_hi = simd_load_si128<mem_mode>(pMask + i * 2 + 16);
         auto row2_lo = simd_load_si128<mem_mode>(pMask + nSrc2Pitch + i * 2);
         auto row2_hi = simd_load_si128<mem_mode>(pMask + nSrc2Pitch + i * 2 + 16);

         // row0 - row1 - row2: weights 1-2-1
         //   oXo mid
         //   oXo
         //   oXo
         auto row1mul2_lo = _mm_slli_epi16(_mm_and_si128(row1_lo, evenmask), 1); // *2
         auto row1mul2_hi = _mm_slli_epi16(_mm_and_si128(row1_hi, evenmask), 1);
         auto mid_lo = _mm_add_epi16(row1mul2_lo, _mm_add_epi16(_mm_and_si128(row0_lo, evenmask), _mm_and_si128(row2_lo, evenmask)));
         auto mid_hi = _mm_add_epi16(row1mul2_hi, _mm_add_epi16(_mm_and_si128(row0_hi, evenmask), _mm_and_si128(row2_hi, evenmask)));

         auto old_right_hi = right_hi;

         //   ooX right
         //   ooX
         //   ooX
         auto row1rmul2_lo = _mm_slli_epi16(_mm_srli_epi16(row1_lo, 8), 1); // *2
         auto row1rmul2_hi = _mm_slli_epi16(_mm_srli_epi16(row1_hi, 8), 1);
         right_lo = _mm_add_epi16(row1rmul2_lo, _mm_add_epi16(_mm_srli_epi16(row0_lo, 8), _mm_srli_epi16(row2_lo, 8)));
         right_hi = _mm_add_epi16(row1rmul2_hi, _mm_add_epi16(_mm_srli_epi16(row0_hi, 8), _mm_srli_epi16(row2_hi, 8)));

         // left: (current_right_lo, right_hi << 1 word, prev_right_hi (1 word)
         auto left_hi = _mm_or_si128(_mm_slli_si128(right_hi, 2), _mm_srli_si128(right_lo, 14)); // 32 byte srli across 128bit registers
         auto left_lo = _mm_or_si128(_mm_slli_si128(right_lo, 2), _mm_srli_si128(old_right_hi, 14)); // shift in from previous right_hi

         // mask: (left + mid*2 + right) / 16. Left, mid and right are already sums of 3 pixels with weight of 4
         auto eight = _mm_set1_epi16(8); // rounder
         auto mask_lo = _mm_srli_epi16(_mm_add_epi16(_mm_add_epi16(_mm_add_epi16(_mm_slli_epi16(mid_lo, 1), left_lo), right_lo), eight), 4);
         auto mask_hi = _mm_srli_epi16(_mm_add_epi16(_mm_add_epi16(_mm_add_epi16(_mm_slli_epi16(mid_hi, 1), left_hi), right_hi), eight), 4);

         auto result = merge_sse2_core<mem_mode, flags>(pDst + i, pSrc1 + i, mask_lo, mask_hi, v128, zero, maxMaskFF);

         simd_store_si128<mem_mode>(pDst + i, result);
       }
       pDst += nDstPitch;
       pSrc1 += nSrc1Pitch;
       pMask += nSrc2Pitch * 2;
     }
     if (nWidth > wMod16) {
       if (wMod16 != 0) // indexing leftmost - 1 is allowed
         merge_luma_420_topleft_allow_leftminus1_c(pDst_s + wMod16, nDstPitch, pSrc1_s + wMod16, nSrc1Pitch, pMask_s + wMod16 * 2, nSrc2Pitch, nWidth - wMod16, nHeight);
       else
         merge_luma_420_topleft_c(pDst_s + wMod16, nDstPitch, pSrc1_s + wMod16, nSrc1Pitch, pMask_s + wMod16 * 2, nSrc2Pitch, nWidth - wMod16, nHeight);
     }
   }

   template <MemoryMode mem_mode, CpuFlags flags>
   void merge_luma_422_sse2_t(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch,
     const uint8_t *pMask, ptrdiff_t nSrc2Pitch, int nWidth, int nHeight)
   {
     int wMod16 = (nWidth / 16) * 16;
     auto pDst_s = pDst;
     auto pSrc1_s = pSrc1;
     auto pMask_s = pMask;
#pragma warning(disable: 4309)
     auto v255 = _mm_set1_epi16((short)0x00FF);
     auto v128 = _mm_set1_epi16((short)0x0080);
     auto maxMaskFF = _mm_set1_epi8(0xFF);
#pragma warning(default: 4309)
     auto zero = _mm_setzero_si128();
     for (int j = 0; j < nHeight; ++j) {
       for (int i = 0; i < wMod16; i += 16) {
         _mm_prefetch(reinterpret_cast<const char*>(pMask) + i * 2 + 64, _MM_HINT_T0);
         // preparing mask
         auto src2_row1_t1 = simd_load_si128<mem_mode>(pMask + i * 2);
         auto src2_row1_t2 = simd_load_si128<mem_mode>(pMask + i * 2 + 16);
         auto shifted_t1 = _mm_srli_si128(src2_row1_t1, 1);
         auto shifted_t2 = _mm_srli_si128(src2_row1_t2, 1);
         auto avg_t1 = _mm_avg_epu8(src2_row1_t1, shifted_t1);
         auto avg_t2 = _mm_avg_epu8(src2_row1_t2, shifted_t2);
         auto mask_t1 = _mm_and_si128(avg_t1, v255);
         auto mask_t2 = _mm_and_si128(avg_t2, v255);

         auto result = merge_sse2_core<mem_mode, flags>(pDst + i, pSrc1 + i, mask_t1, mask_t2, v128, zero, maxMaskFF);

         simd_store_si128<mem_mode>(pDst + i, result);
       }
       pDst += nDstPitch;
       pSrc1 += nSrc1Pitch;
       pMask += nSrc2Pitch;
     }
     if (nWidth > wMod16) {
       merge_luma_422_c(pDst_s + wMod16, nDstPitch, pSrc1_s + wMod16, nSrc1Pitch, pMask_s + wMod16 * 2, nSrc2Pitch, nWidth - wMod16, nHeight);
     }
   }

   template <MemoryMode mem_mode, CpuFlags flags>
   void merge_luma_422_mpeg2_sse2_t(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch,
     const uint8_t *pMask, ptrdiff_t nSrc2Pitch, int nWidth, int nHeight)
   {
     int wMod16 = (nWidth / 16) * 16;
     auto pDst_s = pDst;
     auto pSrc1_s = pSrc1;
     auto pMask_s = pMask;
#pragma warning(disable: 4309)
     auto v128 = _mm_set1_epi16((short)0x0080);
     auto maxMaskFF = _mm_set1_epi8(0xFF);
#pragma warning(default: 4309)
     auto zero = _mm_setzero_si128();
     for (int j = 0; j < nHeight; ++j) {
       // prepare "right_hi": we need only pixel column #0 as the rightmost word (14-15th byte)
       auto row1 = simd_load_si128<mem_mode>(pMask + 0 * 2);
       __m128i right_lo, right_hi;
#pragma warning(disable: 4309)
       auto evenmask = _mm_set1_epi16(0x00FF);
#pragma warning(default: 4309)
       right_hi = _mm_slli_si128(_mm_and_si128(row1, evenmask), 14);
       for (int i = 0; i < wMod16; i += 16) {
         _mm_prefetch(reinterpret_cast<const char*>(pMask) + i * 2 + 64, _MM_HINT_T0);
         // preparing mask
         auto row1_lo = simd_load_si128<mem_mode>(pMask + i * 2);
         auto row1_hi = simd_load_si128<mem_mode>(pMask + i * 2 + 16);

         auto mid_lo = _mm_and_si128(row1_lo, evenmask);
         auto mid_hi = _mm_and_si128(row1_hi, evenmask);

         auto old_right_hi = right_hi;

         right_lo = _mm_srli_epi16(row1_lo, 8);
         right_hi = _mm_srli_epi16(row1_hi, 8);

         // left: (current_right_lo, right_hi << 1 word), prev_right_hi (1 word)
         auto left_hi = _mm_or_si128(_mm_slli_si128(right_hi, 2), _mm_srli_si128(right_lo, 14)); // 32 byte srli across 128bit registers
         auto left_lo = _mm_or_si128(_mm_slli_si128(right_lo, 2), _mm_srli_si128(old_right_hi, 14)); // shift in from previous right_hi

         // mask: (left + mid*2 + right) / 4
         auto two = _mm_set1_epi16(2); // rounder
         auto mask_lo = _mm_srli_epi16(_mm_add_epi16(_mm_add_epi16(_mm_add_epi16(_mm_slli_epi16(mid_lo, 1), left_lo), right_lo), two), 2);
         auto mask_hi = _mm_srli_epi16(_mm_add_epi16(_mm_add_epi16(_mm_add_epi16(_mm_slli_epi16(mid_hi, 1), left_hi), right_hi), two), 2);

         auto result = merge_sse2_core<mem_mode, flags>(pDst + i, pSrc1 + i, mask_lo, mask_hi, v128, zero, maxMaskFF);

         simd_store_si128<mem_mode>(pDst + i, result);
       }
       pDst += nDstPitch;
       pSrc1 += nSrc1Pitch;
       pMask += nSrc2Pitch;
     }
     if (nWidth > wMod16) {
       if (wMod16 != 0) // indexing leftmost - 1 is allowed
         merge_luma_422_mpeg2_allow_leftminus1_c(pDst_s + wMod16, nDstPitch, pSrc1_s + wMod16, nSrc1Pitch, pMask_s + wMod16 * 2, nSrc2Pitch, nWidth - wMod16, nHeight);
       else
         merge_luma_422_mpeg2_c(pDst_s + wMod16, nDstPitch, pSrc1_s + wMod16, nSrc1Pitch, pMask_s + wMod16 * 2, nSrc2Pitch, nWidth - wMod16, nHeight);
     }
   }

   // 411: the masks of 16 chroma pixels from 64 luma mask pixels of the row, as words (lo: 0-7, hi: 8-15);
   // pair averages of pair averages, as the C version
   template <MemoryMode mem_mode>
   MT_FORCEINLINE void mask_411_sse2(const uint8_t *pMask, __m128i &mask_lo, __m128i &mask_hi) {
     auto v255 = _mm_set1_epi16(0x00FF);
     auto m0 = simd_load_si128<mem_mode>(pMask);
     auto m1 = simd_load_si128<mem_mode>(pMask + 16);
     auto m2 = simd_load_si128<mem_mode>(pMask + 32);
     auto m3 = simd_load_si128<mem_mode>(pMask + 48);
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

   template <MemoryMode mem_mode, CpuFlags flags>
   void merge_luma_411_sse2_t(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch,
     const uint8_t *pMask, ptrdiff_t nSrc2Pitch, int nWidth, int nHeight)
   {
     int wMod16 = (nWidth / 16) * 16;
     auto pDst_s = pDst;
     auto pSrc1_s = pSrc1;
     auto pMask_s = pMask;
#pragma warning(disable: 4309)
     auto v128 = _mm_set1_epi16((short)0x0080);
     auto maxMaskFF = _mm_set1_epi8(0xFF);
#pragma warning(default: 4309)
     auto zero = _mm_setzero_si128();
     for (int j = 0; j < nHeight; ++j) {
       for (int i = 0; i < wMod16; i += 16) {
         _mm_prefetch(reinterpret_cast<const char*>(pMask) + i * 4 + 64, _MM_HINT_T0);
         // preparing mask
         __m128i mask_t1, mask_t2;
         mask_411_sse2<mem_mode>(pMask + i * 4, mask_t1, mask_t2);

         auto result = merge_sse2_core<mem_mode, flags>(pDst + i, pSrc1 + i, mask_t1, mask_t2, v128, zero, maxMaskFF);

         simd_store_si128<mem_mode>(pDst + i, result);
       }
       pDst += nDstPitch;
       pSrc1 += nSrc1Pitch;
       pMask += nSrc2Pitch;
     }
     if (nWidth > wMod16) {
       merge_luma_411_c(pDst_s + wMod16, nDstPitch, pSrc1_s + wMod16, nSrc1Pitch, pMask_s + wMod16 * 4, nSrc2Pitch, nWidth - wMod16, nHeight);
     }
   }

} // namespace

} } } }

#endif // __Mt_merge_SSE_H__
