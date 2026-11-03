#include "merge.h"

#ifdef INTEL_INTRINSICS

#include "../../common/simd.h"
#include "merge_410_simd.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Merge {

// C versions, for the right edge
   void merge32_luma_420_mpeg2_allow_leftminus1_c(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch,
     const uint8_t *pMask, ptrdiff_t nSrc2Pitch, int nWidth, int nHeight);
   void merge32_luma_422_mpeg2_allow_leftminus1_c(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch,
     const uint8_t *pMask, ptrdiff_t nSrc2Pitch, int nWidth, int nHeight);

namespace {

   template<bool top_left>
   void merge32_luma_440_sse2_t(uint8_t *dst, ptrdiff_t dst_pitch, const uint8_t *src, ptrdiff_t src_pitch,
     const uint8_t *mask, ptrdiff_t mask_pitch, int width, int height)
   {
     const int wmod = width / 4 * 4;
     auto dst_start = dst;
     auto src_start = src;
     auto mask_start = mask;
     for (int y = 0; y < height; ++y) {
       const auto above = y == 0 ? mask : mask - mask_pitch;
       for (int x = 0; x < wmod; x += 4) {
         auto mid = _mm_loadu_ps(reinterpret_cast<const float*>(mask) + x);
         auto below = _mm_loadu_ps(reinterpret_cast<const float*>(mask + mask_pitch) + x);
         __m128 m;
         if constexpr (top_left) {
           auto prev = _mm_loadu_ps(reinterpret_cast<const float*>(above) + x);
           m = _mm_mul_ps(_mm_add_ps(_mm_add_ps(prev, _mm_mul_ps(mid, _mm_set1_ps(2.0f))), below), _mm_set1_ps(0.25f));
         }
         else
           m = _mm_mul_ps(_mm_add_ps(mid, below), _mm_set1_ps(0.5f));
         auto d = _mm_loadu_ps(reinterpret_cast<const float*>(dst) + x);
         auto s = _mm_loadu_ps(reinterpret_cast<const float*>(src) + x);
         // Keep the new format's C blend order, not the older SIMD d + m * (s - d) form.
         auto result = _mm_add_ps(_mm_mul_ps(_mm_sub_ps(_mm_set1_ps(1.0f), m), d), _mm_mul_ps(m, s));
         _mm_storeu_ps(reinterpret_cast<float*>(dst) + x, result);
       }
       dst += dst_pitch;
       src += src_pitch;
       mask += mask_pitch * 2;
     }
     if (width > wmod) {
       Processor32 *tail = top_left ? merge32_luma_440_topleft_c : merge32_luma_440_c;
       tail(dst_start + wmod * sizeof(float), dst_pitch, src_start + wmod * sizeof(float), src_pitch,
         mask_start + wmod * sizeof(float), mask_pitch, width - wmod, height);
     }
   }

   MT_FORCEINLINE __m128 merge32_sse2_core(uint8_t *pDst, const uint8_t *pSrc, const __m128& mask) {
      // ( 1- mask) * dst + mask * src = 
      // dst - dst * mask + src * mask = 
      // dst + mask * (src - dst)
      auto dst = _mm_loadu_ps(reinterpret_cast<const float*>(pDst));
      auto src = _mm_loadu_ps(reinterpret_cast<const float*>(pSrc));
      auto diff = _mm_sub_ps(src, dst);
      auto tmp = _mm_mul_ps(diff, mask); // (p2-p1)*mask
      return _mm_add_ps(tmp, dst); // +dst
   }

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
            auto mask = _mm_loadu_ps(reinterpret_cast<const float*>(pMask + i));
            auto result = merge32_sse2_core(pDst + i, pSrc1 + i, mask);
            _mm_storeu_ps(reinterpret_cast<float*>(pDst + i), result);
         }
         pDst += nDstPitch;
         pSrc1 += nSrc1Pitch;
         pMask += nSrc2Pitch;
      }

      if (nWidth > wMod16) {
        merge32_c(pDst_s + wMod16, nDstPitch, pSrc1_s + wMod16, nSrc1Pitch, pMask_s + wMod16, nSrc2Pitch, (nWidth - wMod16) / sizeof(float), nHeight);
      }
   }

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
         // preparing mask
         auto src2_row1_t1 = _mm_loadu_ps(reinterpret_cast<const float*>(pMask + i * 2));
         auto src2_row1_t2 = _mm_loadu_ps(reinterpret_cast<const float*>(pMask + i * 2 + 16));
         auto src2_row2_t1 = _mm_loadu_ps(reinterpret_cast<const float*>(pMask + nSrc2Pitch + i * 2));
         auto src2_row2_t2 = _mm_loadu_ps(reinterpret_cast<const float*>(pMask + nSrc2Pitch + i * 2 + 16));
         auto tmp1 = _mm_add_ps(src2_row1_t1, src2_row2_t1);
         auto tmp2 = _mm_add_ps(src2_row1_t2, src2_row2_t2);
         // sum horizontally
         auto sum = simd_hadd_ps(tmp1, tmp2);
         // make average
         auto mask = _mm_mul_ps(sum, vOneFourth);

         auto result = merge32_sse2_core(pDst + i, pSrc1 + i, mask);

         _mm_storeu_ps(reinterpret_cast<float*>(pDst + i), result);
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
       auto row1 = _mm_loadu_ps(reinterpret_cast<const float*>(pMask + 0 * 2));
       auto row2 = _mm_loadu_ps(reinterpret_cast<const float*>(pMask + 0 * 2 + 16));
       // sum vertically and keep evens
       auto right = _mm_add_ps(row1, row2);
       // move lowest to highest, others n/a
       right = _mm_shuffle_ps(right, right, (0 << 0) | (0 << 2) | (0 << 4) | (0 << 6));

       for (int i = 0; i < wMod16; i += 16) {
         // preparing mask
         auto row1_lo = _mm_loadu_ps(reinterpret_cast<const float*>(pMask + i * 2));
         auto row1_hi = _mm_loadu_ps(reinterpret_cast<const float*>(pMask + i * 2 + 16));
         auto row2_lo = _mm_loadu_ps(reinterpret_cast<const float*>(pMask + nSrc2Pitch + i * 2));
         auto row2_hi = _mm_loadu_ps(reinterpret_cast<const float*>(pMask + nSrc2Pitch + i * 2 + 16));
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

         auto result = merge32_sse2_core(pDst + i, pSrc1 + i, mask);

         _mm_storeu_ps(reinterpret_cast<float*>(pDst + i), result);
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
         // preparing mask
         auto mask_row_t1 = _mm_loadu_ps(reinterpret_cast<const float*>(pMask + i * 2));
         auto mask_row_t2 = _mm_loadu_ps(reinterpret_cast<const float*>(pMask + i * 2 + 16));
         // sum horizontally
         auto sum = simd_hadd_ps(mask_row_t1, mask_row_t2);
         // make average
         auto mask = _mm_mul_ps(sum, vHalf);
         auto result = merge32_sse2_core(pDst + i, pSrc1 + i, mask);

         _mm_storeu_ps(reinterpret_cast<float*>(pDst + i), result);
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
       auto row1 = _mm_loadu_ps(reinterpret_cast<const float*>(pMask + 0 * 2));
       // keep evens
       auto right = row1;
       // move lowest to highest, others n/a
       right = _mm_shuffle_ps(right, right, (0 << 0) | (0 << 2) | (0 << 4) | (0 << 6));

       for (int i = 0; i < wMod16; i += 16) {
         // preparing mask
         auto row1_lo = _mm_loadu_ps(reinterpret_cast<const float*>(pMask + i * 2));
         auto row1_hi = _mm_loadu_ps(reinterpret_cast<const float*>(pMask + i * 2 + 16));
         // mask evens and collapse
         auto mid = _mm_shuffle_ps(row1_lo, row1_hi, (0 << 0) | (2 << 2) | (0 << 4) | (2 << 6));
         // mask odds and collapse
         auto old_right = right;
         right = _mm_shuffle_ps(row1_lo, row1_hi, (1 << 0) | (3 << 2) | (1 << 4) | (3 << 6));

         auto left = _mm_castsi128_ps(_mm_or_si128(_mm_slli_si128(_mm_castps_si128(right), 4), _mm_srli_si128(_mm_castps_si128(old_right), 12))); // shift in from previous right

         // (left + mid*2 + right) / 4
         auto mask = _mm_mul_ps(_mm_add_ps(_mm_add_ps(_mm_add_ps(mid, mid), left), right), vOneFourth);

         auto result = merge32_sse2_core(pDst + i, pSrc1 + i, mask);

         _mm_storeu_ps(reinterpret_cast<float*>(pDst + i), result);
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

   Processor32 *merge32_luma_440_sse2 = merge32_luma_440_sse2_t<false>;
   Processor32 *merge32_luma_440_topleft_sse2 = merge32_luma_440_sse2_t<true>;
   Processor32 *merge32_luma_410_sse2 = merge_410_t<float, false, false, Mask410SSE, nullptr>;
   Processor32 *merge32_luma_410_mpeg2_sse2 = merge_410_t<float, true, false, Mask410SSE, nullptr>;
   Processor32 *merge32_luma_410_topleft_sse2 = merge_410_t<float, true, true, Mask410SSE, nullptr>;

   Processor32 *merge32_sse2 = merge32_sse2_t;
   Processor32 *merge32_luma_420_sse2 = merge32_luma_420_sse2_t;
   Processor32 *merge32_luma_420_mpeg2_sse2 = merge32_luma_420_mpeg2_sse2_t;
   Processor32 *merge32_luma_422_sse2 = merge32_luma_422_sse2_t;
   Processor32 *merge32_luma_422_mpeg2_sse2 = merge32_luma_422_mpeg2_sse2_t;

} } } }

#endif // INTEL_INTRINSICS
