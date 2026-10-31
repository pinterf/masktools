#include "merge.h"

#ifdef INTEL_INTRINSICS

#include "../../common/simd.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Merge {

// C versions, for the right edge
   void merge32_luma_420_mpeg2_allow_leftminus1_c(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch,
     const uint8_t *pMask, ptrdiff_t nSrc2Pitch, int nWidth, int nHeight);
   void merge32_luma_422_mpeg2_allow_leftminus1_c(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch,
     const uint8_t *pMask, ptrdiff_t nSrc2Pitch, int nWidth, int nHeight);

namespace {

   template <MemoryMode mem_mode>
   MT_FORCEINLINE __m128 merge32_sse2_core(uint8_t *pDst, const uint8_t *pSrc, const __m128& mask) {
      // ( 1- mask) * dst + mask * src = 
      // dst - dst * mask + src * mask = 
      // dst + mask * (src - dst)
      auto dst = simd_load_ps<mem_mode>(pDst);
      auto src = simd_load_ps<mem_mode>(pSrc);
      auto diff = _mm_sub_ps(src, dst);
      auto tmp = _mm_mul_ps(diff, mask); // (p2-p1)*mask
      return _mm_add_ps(tmp, dst); // +dst
   }

   template <MemoryMode mem_mode>
   void merge32_sse2_t(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch,
      const uint8_t *pMask, ptrdiff_t nSrc2Pitch, int nWidth, int nHeight)
   {
      nWidth *= sizeof(float);

      int wMod16 = (nWidth / 16) * 16;
      auto pDst_s = pDst;
      auto pSrc1_s = pSrc1;
      auto pMask_s = pMask;
      for (int j = 0; j < nHeight; ++j) {
         for (int i = 0; i < wMod16; i += 16) {
            auto mask = simd_load_ps<mem_mode>(pMask + i);
            auto result = merge32_sse2_core<mem_mode>(pDst + i, pSrc1 + i, mask);
            simd_store_ps<mem_mode>(pDst + i, result);
         }
         pDst += nDstPitch;
         pSrc1 += nSrc1Pitch;
         pMask += nSrc2Pitch;
      }

      if (nWidth > wMod16) {
        merge32_c(pDst_s + wMod16, nDstPitch, pSrc1_s + wMod16, nSrc1Pitch, pMask_s + wMod16, nSrc2Pitch, (nWidth - wMod16) / sizeof(float), nHeight);
      }
   }

   template <MemoryMode mem_mode>
   void merge32_luma_420_sse2_t(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch,
     const uint8_t *pMask, ptrdiff_t nSrc2Pitch, int nWidth, int nHeight)
   {
     nWidth *= sizeof(float);

     int wMod16 = (nWidth / 16) * 16;
     auto pDst_s = pDst;
     auto pSrc1_s = pSrc1;
     auto pMask_s = pMask;
     auto vOneFourth = _mm_set1_ps(0.25f);
     for (int j = 0; j < nHeight; ++j) {
       for (int i = 0; i < wMod16; i += 16) {
         _mm_prefetch(reinterpret_cast<const char*>(pMask) + i * 2 + 64, _MM_HINT_T0);
         _mm_prefetch(reinterpret_cast<const char*>(pMask) + nSrc2Pitch + i * 2 + 64, _MM_HINT_T0);
         // preparing mask
         auto src2_row1_t1 = simd_load_ps<mem_mode>(pMask + i * 2);
         auto src2_row1_t2 = simd_load_ps<mem_mode>(pMask + i * 2 + 16);
         auto src2_row2_t1 = simd_load_ps<mem_mode>(pMask + nSrc2Pitch + i * 2);
         auto src2_row2_t2 = simd_load_ps<mem_mode>(pMask + nSrc2Pitch + i * 2 + 16);
         auto tmp1 = _mm_add_ps(src2_row1_t1, src2_row2_t1);
         auto tmp2 = _mm_add_ps(src2_row1_t2, src2_row2_t2);
         // sum horizontally
         auto sum = _mm_hadd_ps(tmp1, tmp2);
         // make average
         auto mask = _mm_mul_ps(sum, vOneFourth);

         auto result = merge32_sse2_core<mem_mode>(pDst + i, pSrc1 + i, mask);

         simd_store_ps<mem_mode>(pDst + i, result);
       }
       pDst += nDstPitch;
       pSrc1 += nSrc1Pitch;
       pMask += nSrc2Pitch * 2;
     }
     if (nWidth > wMod16) {
       // pMask offset: mask is not subsampled -> width*2
       merge32_luma_420_c(pDst_s + wMod16, nDstPitch, pSrc1_s + wMod16, nSrc1Pitch, pMask_s + wMod16 * 2, nSrc2Pitch, (nWidth - wMod16) / sizeof(float), nHeight);
     }
   }

   template <MemoryMode mem_mode>
   void merge32_luma_420_mpeg2_sse2_t(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch,
     const uint8_t *pMask, ptrdiff_t nSrc2Pitch, int nWidth, int nHeight)
   {
     nWidth *= sizeof(float);

     int wMod16 = (nWidth / 16) * 16;
     auto pDst_s = pDst;
     auto pSrc1_s = pSrc1;
     auto pMask_s = pMask;
     auto vOneEight = _mm_set1_ps(0.125f);
     for (int j = 0; j < nHeight; ++j) {
       auto row1 = simd_load_ps<mem_mode>(pMask + 0 * 2);
       auto row2 = simd_load_ps<mem_mode>(pMask + 0 * 2 + 16);
       // sum vertically and keep evens
       auto right = _mm_add_ps(row1, row2);
       // move lowest to highest, others n/a
       right = _mm_shuffle_ps(right, right, (0 << 0) | (0 << 2) | (0 << 4) | (0 << 6));

       for (int i = 0; i < wMod16; i += 16) {
         // preparing mask
         auto row1_lo = simd_load_ps<mem_mode>(pMask + i * 2);
         auto row1_hi = simd_load_ps<mem_mode>(pMask + i * 2 + 16);
         auto row2_lo = simd_load_ps<mem_mode>(pMask + nSrc2Pitch + i * 2);
         auto row2_hi = simd_load_ps<mem_mode>(pMask + nSrc2Pitch + i * 2 + 16);
         // sum vertically
         auto tmp_lo = _mm_add_ps(row1_lo, row2_lo);
         auto tmp_hi = _mm_add_ps(row1_hi, row2_hi);
         // mask evens and collapse
         auto mid = _mm_shuffle_ps(tmp_lo, tmp_hi, (0 << 0) | (2 << 2) | (0 << 4) | (2 << 6));
         // mask odds and collapse
         auto old_right = right;
         right = _mm_shuffle_ps(tmp_lo, tmp_hi, (1 << 0) | (3 << 2) | (1 << 4) | (3 << 6));

         auto left = _mm_castsi128_ps(_mm_or_si128(_mm_slli_si128(_mm_castps_si128(right), 4), _mm_srli_si128(_mm_castps_si128(old_right), 12))); // shift in from previous right

         // (left + mid*2 + right) / 8
         auto mask = _mm_mul_ps(_mm_add_ps(_mm_add_ps(_mm_add_ps(mid, mid), left), right), vOneEight);

         auto result = merge32_sse2_core<mem_mode>(pDst + i, pSrc1 + i, mask);

         simd_store_ps<mem_mode>(pDst + i, result);
       }
       pDst += nDstPitch;
       pSrc1 += nSrc1Pitch;
       pMask += nSrc2Pitch * 2;
     }
     if (nWidth > wMod16) {
       // pMask offset: mask is not subsampled -> width*2
       if(wMod16 != 0)
         merge32_luma_420_mpeg2_allow_leftminus1_c(pDst_s + wMod16, nDstPitch, pSrc1_s + wMod16, nSrc1Pitch, pMask_s + wMod16 * 2, nSrc2Pitch, (nWidth - wMod16) / sizeof(float), nHeight);
       else
         merge32_luma_420_mpeg2_c(pDst_s + wMod16, nDstPitch, pSrc1_s + wMod16, nSrc1Pitch, pMask_s + wMod16 * 2, nSrc2Pitch, (nWidth - wMod16) / sizeof(float), nHeight);
     }
   }

   template <MemoryMode mem_mode>
   void merge32_luma_422_sse2_t(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch,
     const uint8_t *pMask, ptrdiff_t nSrc2Pitch, int nWidth, int nHeight)
   {
     nWidth *= sizeof(float);

     int wMod16 = (nWidth / 16) * 16;
     auto pDst_s = pDst;
     auto pSrc1_s = pSrc1;
     auto pMask_s = pMask;
     auto vHalf = _mm_set1_ps(0.5f);
     for (int j = 0; j < nHeight; ++j) {
       for (int i = 0; i < wMod16; i += 16) {
         _mm_prefetch(reinterpret_cast<const char*>(pMask) + i * 2 + 64, _MM_HINT_T0);
         // preparing mask
         auto mask_row_t1 = simd_load_ps<mem_mode>(pMask + i * 2);
         auto mask_row_t2 = simd_load_ps<mem_mode>(pMask + i * 2 + 16);
         // sum horizontally
         auto sum = _mm_hadd_ps(mask_row_t1, mask_row_t2);
         // make average
         auto mask = _mm_mul_ps(sum, vHalf);
         auto result = merge32_sse2_core<mem_mode>(pDst + i, pSrc1 + i, mask);

         simd_store_ps<mem_mode>(pDst + i, result);
       }
       pDst += nDstPitch;
       pSrc1 += nSrc1Pitch;
       pMask += nSrc2Pitch;
     }
     if (nWidth > wMod16) {
       // pMask offset: mask is not subsampled -> width*2
       merge32_luma_422_c(pDst_s + wMod16, nDstPitch, pSrc1_s + wMod16, nSrc1Pitch, pMask_s + wMod16 * 2, nSrc2Pitch, (nWidth - wMod16) / sizeof(float), nHeight);
     }
   }

   template <MemoryMode mem_mode>
   void merge32_luma_422_mpeg2_sse2_t(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch,
     const uint8_t *pMask, ptrdiff_t nSrc2Pitch, int nWidth, int nHeight)
   {
     nWidth *= sizeof(float);

     int wMod16 = (nWidth / 16) * 16;
     auto pDst_s = pDst;
     auto pSrc1_s = pSrc1;
     auto pMask_s = pMask;
     auto vOneFourth = _mm_set1_ps(0.25f);
     for (int j = 0; j < nHeight; ++j) {
       auto row1 = simd_load_ps<mem_mode>(pMask + 0 * 2);
       // keep evens
       auto right = row1;
       // move lowest to highest, others n/a
       right = _mm_shuffle_ps(right, right, (0 << 0) | (0 << 2) | (0 << 4) | (0 << 6));

       for (int i = 0; i < wMod16; i += 16) {
         // preparing mask
         auto row1_lo = simd_load_ps<mem_mode>(pMask + i * 2);
         auto row1_hi = simd_load_ps<mem_mode>(pMask + i * 2 + 16);
         // mask evens and collapse
         auto mid = _mm_shuffle_ps(row1_lo, row1_hi, (0 << 0) | (2 << 2) | (0 << 4) | (2 << 6));
         // mask odds and collapse
         auto old_right = right;
         right = _mm_shuffle_ps(row1_lo, row1_hi, (1 << 0) | (3 << 2) | (1 << 4) | (3 << 6));

         auto left = _mm_castsi128_ps(_mm_or_si128(_mm_slli_si128(_mm_castps_si128(right), 4), _mm_srli_si128(_mm_castps_si128(old_right), 12))); // shift in from previous right

         // (left + mid*2 + right) / 4
         auto mask = _mm_mul_ps(_mm_add_ps(_mm_add_ps(_mm_add_ps(mid, mid), left), right), vOneFourth);

         auto result = merge32_sse2_core<mem_mode>(pDst + i, pSrc1 + i, mask);

         simd_store_ps<mem_mode>(pDst + i, result);
       }
       pDst += nDstPitch;
       pSrc1 += nSrc1Pitch;
       pMask += nSrc2Pitch;
     }
     if (nWidth > wMod16) {
       // pMask offset: mask is not subsampled -> width*2
       if (wMod16 != 0)
         merge32_luma_422_mpeg2_allow_leftminus1_c(pDst_s + wMod16, nDstPitch, pSrc1_s + wMod16, nSrc1Pitch, pMask_s + wMod16 * 2, nSrc2Pitch, (nWidth - wMod16) / sizeof(float), nHeight);
       else
         merge32_luma_422_mpeg2_c(pDst_s + wMod16, nDstPitch, pSrc1_s + wMod16, nSrc1Pitch, pMask_s + wMod16 * 2, nSrc2Pitch, (nWidth - wMod16) / sizeof(float), nHeight);
     }
   }

} // namespace

   Processor32 *merge32_sse2 = merge32_sse2_t<MemoryMode::SSE2_UNALIGNED>;
   Processor32 *merge32_asse2 = merge32_sse2_t<MemoryMode::SSE2_ALIGNED>;
   Processor32 *merge32_luma_420_sse2 = merge32_luma_420_sse2_t<MemoryMode::SSE2_UNALIGNED>;
   Processor32 *merge32_luma_420_asse2 = merge32_luma_420_sse2_t<MemoryMode::SSE2_ALIGNED>;
   Processor32 *merge32_luma_420_mpeg2_sse2 = merge32_luma_420_mpeg2_sse2_t<MemoryMode::SSE2_UNALIGNED>;
   Processor32 *merge32_luma_420_mpeg2_asse2 = merge32_luma_420_mpeg2_sse2_t<MemoryMode::SSE2_ALIGNED>;
   Processor32 *merge32_luma_422_sse2 = merge32_luma_422_sse2_t<MemoryMode::SSE2_UNALIGNED>;
   Processor32 *merge32_luma_422_asse2 = merge32_luma_422_sse2_t<MemoryMode::SSE2_ALIGNED>;
   Processor32 *merge32_luma_422_mpeg2_sse2 = merge32_luma_422_mpeg2_sse2_t<MemoryMode::SSE2_UNALIGNED>;
   Processor32 *merge32_luma_422_mpeg2_asse2 = merge32_luma_422_mpeg2_sse2_t<MemoryMode::SSE2_ALIGNED>;

} } } }

#endif // INTEL_INTRINSICS
