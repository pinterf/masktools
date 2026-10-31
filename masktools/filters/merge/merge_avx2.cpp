#include "merge.h"

#ifdef INTEL_INTRINSICS

#include "../../common/simd_avx2.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Merge {

   static void merge_avx2_c(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch,
      const uint8_t *pMask, ptrdiff_t nSrc2Pitch, int nWidth, int nHeight)
   {
      for (int y = 0; y < nHeight; ++y)
      {
         for (int x = 0; x < nWidth; ++x) {
           const int nMask = pMask[x];
           if (nMask == 255)
             pDst[x] = pSrc1[x]; // max mask value (255): keep source
           else if (nMask != 0)
             pDst[x] = static_cast<uint8_t>(((256 - int(nMask)) * pDst[x] + int(nMask) * pSrc1[x] + 128) >> 8);
           // nMask == 0: keep pDst as is
         }
         pDst += nDstPitch;
         pSrc1 += nSrc1Pitch;
         pMask += nSrc2Pitch;
      }
   }

   static void merge_luma_420_avx2_c(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch,
      const uint8_t *pMask, ptrdiff_t nSrc2Pitch, int nWidth, int nHeight)
   {
      for (int y = 0; y < nHeight; ++y)
      {
         for (int x = 0; x < nWidth; ++x)
         {
           // 420: both width and height is halved, averaging from 4 pixels of full size mask
            const int nMask = (((pMask[x * 2] + pMask[x * 2 + nSrc2Pitch] + 1) >> 1) + ((pMask[x * 2 + 1] + pMask[x * 2 + nSrc2Pitch + 1] + 1) >> 1) + 1) >> 1;
            if (nMask == 255)
              pDst[x] = pSrc1[x];
            else if (nMask != 0)
              pDst[x] = static_cast<uint8_t>(((256 - int(nMask)) * pDst[x] + int(nMask) * pSrc1[x] + 128) >> 8);
            // nMask == 0: keep pDst as is
         }
         pDst += nDstPitch;
         pSrc1 += nSrc1Pitch;
         pMask += nSrc2Pitch * 2;
      }
   }

   static void merge_luma_422_avx2_c(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch,
     const uint8_t *pMask, ptrdiff_t nSrc2Pitch, int nWidth, int nHeight)
   {
     for (int y = 0; y < nHeight; ++y)
     {
       for (int x = 0; x < nWidth; ++x)
       {
         const int nMask = (pMask[x * 2] + pMask[x * 2 + 1] + 1) >> 1; // 422: only width is halved, averaging from two pixels of full size mask
         if (nMask == 255)
           pDst[x] = pSrc1[x];
         else if (nMask != 0)
           pDst[x] = static_cast<uint8_t>(((256 - int(nMask)) * pDst[x] + int(nMask) * pSrc1[x] + 128) >> 8);
         // nMask == 0: keep pDst as is
       }
       pDst += nDstPitch;
       pSrc1 += nSrc1Pitch;
       pMask += nSrc2Pitch;
     }
   }

   static void merge_luma_411_avx2_c(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch,
     const uint8_t *pMask, ptrdiff_t nSrc2Pitch, int nWidth, int nHeight)
   {
     for (int y = 0; y < nHeight; ++y)
     {
       for (int x = 0; x < nWidth; ++x)
       {
         const int nMask = (
           ((pMask[x * 4 + 0] + pMask[x * 4 + 1] + 1) >> 1) +
           ((pMask[x * 4 + 2] + pMask[x * 4 + 3] + 1) >> 1)
           + 1) >> 2; // 411: averaging from four pixels of full size mask
         if (nMask == 255)
           pDst[x] = pSrc1[x];
         else if (nMask != 0)
           pDst[x] = static_cast<uint8_t>(((256 - int(nMask)) * pDst[x] + int(nMask) * pSrc1[x] + 128) >> 8);
         // nMask == 0: keep pDst as is
       }
       pDst += nDstPitch;
       pSrc1 += nSrc1Pitch;
       pMask += nSrc2Pitch;
     }
   }

   template <MemoryMode mem_mode>
   MT_FORCEINLINE __m256i merge_avx2_core(uint8_t *pDst, const uint8_t *pSrc, const __m256i& mask_lo, const __m256i& mask_hi,
      const __m256i& v128, const __m256i& zero, const __m256i& maxMaskFF) {
      auto dst = simd256_load_si256<mem_mode>(pDst);
      auto dst_lo = _mm256_unpacklo_epi8(dst, zero);
      auto dst_hi = _mm256_unpackhi_epi8(dst, zero);
      
      auto src = simd256_load_si256<mem_mode>(pSrc);
      auto src1_lo = _mm256_unpacklo_epi8(src, zero);
      auto src1_hi = _mm256_unpackhi_epi8(src, zero);

      auto dst_lo_sh = _mm256_slli_epi16(dst_lo, 8);
      auto diff_lo = _mm256_sub_epi16(src1_lo, dst_lo);
      auto tmp1_lo = _mm256_mullo_epi16(diff_lo, mask_lo); // (p2-p1)*mask
      auto tmp2_lo = _mm256_or_si256(dst_lo_sh, v128);    // p1<<8 + 128 == p1<<8 | 128
      auto result_lo = _mm256_add_epi16(tmp1_lo, tmp2_lo);
      result_lo = _mm256_srli_epi16(result_lo, 8);

      auto dst_hi_sh = _mm256_slli_epi16(dst_hi, 8);
      auto diff_hi = _mm256_sub_epi16(src1_hi, dst_hi);
      auto tmp1_hi = _mm256_mullo_epi16(diff_hi, mask_hi); // (p2-p1)*mask
      auto tmp2_hi = _mm256_or_si256(dst_hi_sh, v128);    // p1<<8 + 128 == p1<<8 | 128
      auto result_hi = _mm256_add_epi16(tmp1_hi, tmp2_hi);
      result_hi = _mm256_srli_epi16(result_hi, 8);

      auto result = _mm256_packus_epi16(result_lo, result_hi);
      // 2.2.7:
      // when mask is FF, keep src
      // when mask is 00, keep dst
      auto mask = _mm256_packus_epi16(mask_lo, mask_hi);
      auto mask_FF = _mm256_cmpeq_epi8(mask, maxMaskFF); // mask == max ? FF : 00
      auto mask_00 = _mm256_cmpeq_epi8(mask, zero);

      result = simd256_blend_epi8(mask_FF, src, result); // ensure that max mask value returns src
      result = simd256_blend_epi8(mask_00, dst, result); // ensure that zero mask value returns dst

      return result;
   }

   template <MemoryMode mem_mode>
   void merge_avx2_t(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch,
      const uint8_t *pMask, ptrdiff_t nSrc2Pitch, int nWidth, int nHeight)
   {
      int wMod32 = (nWidth / 32) * 32;
      auto pDst_s = pDst;
      auto pSrc1_s = pSrc1;
      auto pMask_s = pMask;
      auto v128 = _mm256_set1_epi16(0x0080);
      auto zero = _mm256_setzero_si256();
#pragma warning(disable: 4309)
      auto maxMaskFF = _mm256_set1_epi8(0xFF);
#pragma warning(default: 4309)
      for (int j = 0; j < nHeight; ++j) {
         for (int i = 0; i < wMod32; i += 32) {
            auto src2 = simd256_load_si256<mem_mode>(pMask + i);
            auto mask_t1 = _mm256_unpacklo_epi8(src2, zero);
            auto mask_t2 = _mm256_unpackhi_epi8(src2, zero);

            auto result = merge_avx2_core<mem_mode>(pDst + i, pSrc1 + i, mask_t1, mask_t2, v128, zero, maxMaskFF);
            // remark: when mask is 255 then second clip's 255 becomes 254 and 0 becomes 1
            // it 16 bit merge this problem is solved: special ensures that mask 255 returns second clip
            // mask 0 returns first clip

            simd256_store_si256<mem_mode>(pDst + i, result);
         }
         pDst += nDstPitch;
         pSrc1 += nSrc1Pitch;
         pMask += nSrc2Pitch;
      }

      if (nWidth > wMod32) {
         merge_avx2_c(pDst_s + wMod32, nDstPitch, pSrc1_s + wMod32, nSrc1Pitch, pMask_s + wMod32, nSrc2Pitch, nWidth - wMod32, nHeight);
      }
   }
   
   template <MemoryMode mem_mode>
   void merge_luma_420_avx2_t(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch,
     const uint8_t *pMask, ptrdiff_t nSrc2Pitch, int nWidth, int nHeight)
   {
     int wMod32 = (nWidth / 32) * 32;
     auto pDst_s = pDst;
     auto pSrc1_s = pSrc1;
     auto pMask_s = pMask;
     auto v255 = _mm256_set1_epi16(0x00FF);
#pragma warning(disable: 4309)
     auto v128 = _mm256_set1_epi16(0x0080);
     auto maxMaskFF = _mm256_set1_epi8(0xFF);
#pragma warning(default: 4309)
     auto zero = _mm256_setzero_si256();
     for (int j = 0; j < nHeight; ++j) {
       for (int i = 0; i < wMod32; i += 32) {
         _mm_prefetch(reinterpret_cast<const char*>(pMask) + i * 2 + 2*64, _MM_HINT_T0);
         _mm_prefetch(reinterpret_cast<const char*>(pMask) + nSrc2Pitch + i * 2 + 2*64, _MM_HINT_T0);
         // preparing mask
         auto src2_row1_t1 = simd256_load_si256<mem_mode>(pMask + i * 2);
         auto src2_row1_t2 = simd256_load_si256<mem_mode>(pMask + i * 2 + 32);
         auto src2_row2_t1 = simd256_load_si256<mem_mode>(pMask + nSrc2Pitch + i * 2);
         auto src2_row2_t2 = simd256_load_si256<mem_mode>(pMask + nSrc2Pitch + i * 2 + 32);
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
         auto result = merge_avx2_core<mem_mode>(pDst + i, pSrc1 + i, mask_t1, mask_t2, v128, zero, maxMaskFF);

         simd256_store_si256<mem_mode>(pDst + i, result);
       }
       pDst += nDstPitch;
       pSrc1 += nSrc1Pitch;
       pMask += nSrc2Pitch * 2;
     }
     if (nWidth > wMod32) {
       // pMask offset: mask is not subsampled -> width*2
       merge_luma_420_avx2_c(pDst_s + wMod32, nDstPitch, pSrc1_s + wMod32, nSrc1Pitch, pMask_s + wMod32 * 2, nSrc2Pitch, nWidth - wMod32, nHeight);
     }
   }

   template <MemoryMode mem_mode>
   void merge_luma_422_avx2_t(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch,
     const uint8_t *pMask, ptrdiff_t nSrc2Pitch, int nWidth, int nHeight)
   {
     int wMod32 = (nWidth / 32) * 32;
     auto pDst_s = pDst;
     auto pSrc1_s = pSrc1;
     auto pMask_s = pMask;
     auto v255 = _mm256_set1_epi16(0x00FF);
     auto v128 = _mm256_set1_epi16(0x0080);
     auto zero = _mm256_setzero_si256();
#pragma warning(disable: 4309)
     auto maxMaskFF = _mm256_set1_epi8(0xFF);
#pragma warning(default: 4309)
     for (int j = 0; j < nHeight; ++j) {
       for (int i = 0; i < wMod32; i += 32) {
         _mm_prefetch(reinterpret_cast<const char*>(pMask) + i * 2 + 2*64, _MM_HINT_T0);
         // preparing mask
         auto src2_row1_t1 = simd256_load_si256<mem_mode>(pMask + i * 2);
         auto src2_row1_t2 = simd256_load_si256<mem_mode>(pMask + i * 2 + 32);
         auto shifted_t1 = _mm256_srli_si256(src2_row1_t1, 1); // srli with 2x128 lane is OK here
         auto shifted_t2 = _mm256_srli_si256(src2_row1_t2, 1);
         auto avg_t1 = _mm256_avg_epu8(src2_row1_t1, shifted_t1);
         auto avg_t2 = _mm256_avg_epu8(src2_row1_t2, shifted_t2);
         // adjust 128bit lanes to match 128bit lane-aware unpacks inside _core
         auto mask_t1 = _mm256_permute2x128_si256(avg_t1, avg_t2, (0 << 0) + (2 << 4)); // 0: src1lo 1:src1hi 2:src2lo 3:src2hi
         auto mask_t2 = _mm256_permute2x128_si256(avg_t1, avg_t2, (1 << 0) + (3 << 4)); // 0: src1lo 1:src1hi 2:src2lo 3:src2hi
         mask_t1 = _mm256_and_si256(mask_t1, v255);
         mask_t2 = _mm256_and_si256(mask_t2, v255);

         auto result = merge_avx2_core<mem_mode>(pDst + i, pSrc1 + i, mask_t1, mask_t2, v128, zero, maxMaskFF);

         simd256_store_si256<mem_mode>(pDst + i, result);
       }
       pDst += nDstPitch;
       pSrc1 += nSrc1Pitch;
       pMask += nSrc2Pitch;
     }
     if (nWidth > wMod32) {
       // pMask offset: mask is not subsampled -> width*2
       merge_luma_422_avx2_c(pDst_s + wMod32, nDstPitch, pSrc1_s + wMod32, nSrc1Pitch, pMask_s + wMod32 * 2, nSrc2Pitch, nWidth - wMod32, nHeight);
     }
   }

   template <MemoryMode mem_mode>
   void merge_luma_411_avx2_t(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch,
     const uint8_t *pMask, ptrdiff_t nSrc2Pitch, int nWidth, int nHeight)
   {
     int wMod32 = (nWidth / 32) * 32;
     auto pDst_s = pDst;
     auto pSrc1_s = pSrc1;
     auto pMask_s = pMask;
     auto v255 = _mm256_set1_epi16(0x00FF);
     auto v128 = _mm256_set1_epi16(0x0080);
     auto zero = _mm256_setzero_si256();
#pragma warning(disable: 4309)
     auto maxMaskFF = _mm256_set1_epi8(0xFF);
#pragma warning(default: 4309)
     for (int j = 0; j < nHeight; ++j) {
       for (int i = 0; i < wMod32; i += 32) {
         _mm_prefetch(reinterpret_cast<const char*>(pMask) + i * 4 + 2*64, _MM_HINT_T0);
         // preparing mask
         auto src2_row1_t1 = simd256_load_si256<mem_mode>(pMask + i * 4);
         auto src2_row1_t2 = simd256_load_si256<mem_mode>(pMask + i * 4 + 1*32);
         auto src2_row2_t1 = simd256_load_si256<mem_mode>(pMask + i * 4 + 2*32);
         auto src2_row2_t2 = simd256_load_si256<mem_mode>(pMask + i * 4 + 3*32);
         auto avg_t1 = _mm256_avg_epu8(src2_row1_t1, src2_row2_t1);
         auto avg_t2 = _mm256_avg_epu8(src2_row1_t2, src2_row2_t2);
         auto shifted_t1 = _mm256_srli_si256(src2_row1_t1, 1); // srli with 2x128 lane is OK here
         auto shifted_t2 = _mm256_srli_si256(src2_row1_t2, 1);
         avg_t1 = _mm256_avg_epu8(avg_t1, shifted_t1);
         avg_t2 = _mm256_avg_epu8(avg_t2, shifted_t2);
         auto mask_t1 = _mm256_permute2x128_si256(avg_t1, avg_t2, (0 << 0) + (2 << 4)); // 0: src1lo 1:src1hi 2:src2lo 3:src2hi
         auto mask_t2 = _mm256_permute2x128_si256(avg_t1, avg_t2, (1 << 0) + (3 << 4)); // 0: src1lo 1:src1hi 2:src2lo 3:src2hi
         mask_t1 = _mm256_and_si256(mask_t1, v255);
         mask_t2 = _mm256_and_si256(mask_t2, v255);

         auto result = merge_avx2_core<mem_mode>(pDst + i, pSrc1 + i, mask_t1, mask_t2, v128, zero, maxMaskFF);

         simd256_store_si256<mem_mode>(pDst + i, result);
       }
       pDst += nDstPitch;
       pSrc1 += nSrc1Pitch;
       pMask += nSrc2Pitch;
     }
     if (nWidth > wMod32) {
       merge_luma_411_avx2_c(pDst_s + wMod32, nDstPitch, pSrc1_s + wMod32, nSrc1Pitch, pMask_s + wMod32 * 4, nSrc2Pitch, nWidth - wMod32, nHeight);
     }
   }

   // no MPEG2 option here in 8bit AVX2
   Processor *merge_avx2 = merge_avx2_t<MemoryMode::SSE2_UNALIGNED>;
   Processor *merge_luma_420_avx2 = merge_luma_420_avx2_t<MemoryMode::SSE2_UNALIGNED>;
   Processor *merge_luma_422_avx2 = merge_luma_422_avx2_t<MemoryMode::SSE2_UNALIGNED>;
   Processor *merge_luma_411_avx2 = merge_luma_411_avx2_t<MemoryMode::SSE2_UNALIGNED>;

} } } }

#endif // INTEL_INTRINSICS