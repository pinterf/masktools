#include "merge.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Merge {

   void merge32_c(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch,
      const uint8_t *pMask, ptrdiff_t nSrc2Pitch, int nWidth, int nHeight)
   {
      for (int y = 0; y < nHeight; ++y)
      {
         for (int x = 0; x < nWidth; ++x)
            reinterpret_cast<float *>(pDst)[x] = (1.0f - reinterpret_cast<const float *>(pMask)[x]) * reinterpret_cast<float *>(pDst)[x] + 
            reinterpret_cast<const float *>(pMask)[x] * reinterpret_cast<const float *>(pSrc1)[x];
         pDst += nDstPitch;
         pSrc1 += nSrc1Pitch;
         pMask += nSrc2Pitch;
      }
   }

   void merge32_luma_420_c(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch,
      const uint8_t *pMask, ptrdiff_t nSrc2Pitch, int nWidth, int nHeight)
   {
      for (int y = 0; y < nHeight; ++y)
      {
         for (int x = 0; x < nWidth; ++x)
         {
           // 420: both width and height is halved, averaging from 4 pixels of full size mask
            const float nMask = 0.25f * (reinterpret_cast<const float *>(pMask)[x * 2] + reinterpret_cast<const float *>(pMask + nSrc2Pitch)[x * 2] +
                                         reinterpret_cast<const float *>(pMask)[x * 2 + 1] + reinterpret_cast<const float *>(pMask + nSrc2Pitch)[x * 2 + 1]);
            reinterpret_cast<float *>(pDst)[x] = (1.0f - nMask) * reinterpret_cast<float *>(pDst)[x] + nMask * reinterpret_cast<const float *>(pSrc1)[x];
         }
         pDst += nDstPitch;
         pSrc1 += nSrc1Pitch;
         pMask += nSrc2Pitch * 2;
      }
   }

   template<bool allow_leftminus1>
   void internal_merge32_luma_420_mpeg2_c(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch,
     const uint8_t *pMask, ptrdiff_t nSrc2Pitch, int nWidth, int nHeight)
   {
     for (int y = 0; y < nHeight; ++y)
     {
       float right;
       right = allow_leftminus1 ?
         reinterpret_cast<const float*>(pMask)[-1] + reinterpret_cast<const float*>(pMask + nSrc2Pitch)[-1] :
         reinterpret_cast<const float*>(pMask)[0] + reinterpret_cast<const float*>(pMask + nSrc2Pitch)[0];

       for (int x = 0; x < nWidth; ++x)
       {
         float left = right;
         const float mid = reinterpret_cast<const float *>(pMask)[x * 2] + reinterpret_cast<const float *>(pMask + nSrc2Pitch)[x * 2];
         right = reinterpret_cast<const float *>(pMask)[x * 2 + 1] + reinterpret_cast<const float *>(pMask + nSrc2Pitch)[x * 2 + 1];

         const float nMask = 0.125f * (left + 2 * mid + right);
         // 420: both width and height is halved, 1-2-1 weighted averaging from 6 pixels of full size mask

         reinterpret_cast<float *>(pDst)[x] = (1.0f - nMask) * reinterpret_cast<float *>(pDst)[x] + nMask * reinterpret_cast<const float *>(pSrc1)[x];
       }
       pDst += nDstPitch;
       pSrc1 += nSrc1Pitch;
       pMask += nSrc2Pitch * 2;
     }
   }

   void merge32_luma_420_mpeg2_c(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch,
     const uint8_t *pMask, ptrdiff_t nSrc2Pitch, int nWidth, int nHeight)
   {
     internal_merge32_luma_420_mpeg2_c<false>(pDst, nDstPitch, pSrc1, nSrc1Pitch, pMask, nSrc2Pitch, nWidth, nHeight);
   }

   void merge32_luma_420_mpeg2_allow_leftminus1_c(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch,
     const uint8_t *pMask, ptrdiff_t nSrc2Pitch, int nWidth, int nHeight)
   {
     internal_merge32_luma_420_mpeg2_c<true>(pDst, nDstPitch, pSrc1, nSrc1Pitch, pMask, nSrc2Pitch, nWidth, nHeight);
   }

   template<bool allow_leftminus1>
   void internal_merge32_luma_420_topleft_c(uint8_t* pDst, ptrdiff_t nDstPitch, const uint8_t* pSrc1, ptrdiff_t nSrc1Pitch,
     const uint8_t* pMask, ptrdiff_t nSrc2Pitch, int nWidth, int nHeight)
   {
     constexpr auto left_or_same = allow_leftminus1 ? -1 : 0;
     for (int y = 0; y < nHeight; ++y)
     {
       const auto top_or_same = y == 0 ? 0 : -nSrc2Pitch; // for MASK420_TOPLEFT, later -nMaskPitch

       float right =
         reinterpret_cast<const float*>(pMask + top_or_same)[left_or_same] +
         (reinterpret_cast<const float*>(pMask)[left_or_same] * 2.0f) +
         reinterpret_cast<const float*>(pMask + nSrc2Pitch)[left_or_same];

       for (int x = 0; x < nWidth; ++x)
       {
         float left = right;
         const float mid = 
           reinterpret_cast<const float*>(pMask + top_or_same)[x * 2] +
           reinterpret_cast<const float*>(pMask)[x * 2] * 2.0f +
           reinterpret_cast<const float*>(pMask + nSrc2Pitch)[x * 2];
         right =
           reinterpret_cast<const float*>(pMask + top_or_same)[x * 2 + 1] +
           reinterpret_cast<const float*>(pMask)[x * 2 + 1] * 2.0f +
           reinterpret_cast<const float*>(pMask + nSrc2Pitch)[x * 2 + 1];

         const float nMask = 0.0625f * (left + 2 * mid + right); // 1/16
         // 420: both width and height is halved, 1-2-1|2-4-2|1-2-1 weighted averaging from 9 pixels of full size mask

         reinterpret_cast<float*>(pDst)[x] = (1.0f - nMask) * reinterpret_cast<float*>(pDst)[x] + nMask * reinterpret_cast<const float*>(pSrc1)[x];
       }
       pDst += nDstPitch;
       pSrc1 += nSrc1Pitch;
       pMask += nSrc2Pitch * 2;
     }
   }

   void merge32_luma_420_topleft_c(uint8_t* pDst, ptrdiff_t nDstPitch, const uint8_t* pSrc1, ptrdiff_t nSrc1Pitch,
     const uint8_t* pMask, ptrdiff_t nSrc2Pitch, int nWidth, int nHeight)
   {
     internal_merge32_luma_420_topleft_c<false>(pDst, nDstPitch, pSrc1, nSrc1Pitch, pMask, nSrc2Pitch, nWidth, nHeight);
   }

   void merge32_luma_420_topleft_allow_leftminus1_c(uint8_t* pDst, ptrdiff_t nDstPitch, const uint8_t* pSrc1, ptrdiff_t nSrc1Pitch,
     const uint8_t* pMask, ptrdiff_t nSrc2Pitch, int nWidth, int nHeight)
   {
     internal_merge32_luma_420_topleft_c<true>(pDst, nDstPitch, pSrc1, nSrc1Pitch, pMask, nSrc2Pitch, nWidth, nHeight);
   }

   void merge32_luma_422_c(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch,
     const uint8_t *pMask, ptrdiff_t nSrc2Pitch, int nWidth, int nHeight)
   {
     for (int y = 0; y < nHeight; ++y)
     {
       for (int x = 0; x < nWidth; ++x)
       {
         const float nMask = 0.5f * (reinterpret_cast<const float *>(pMask)[x * 2] + reinterpret_cast<const float *>(pMask)[x * 2 + 1]); // 422: only width is halved, averaging from two pixels of full size mask
         reinterpret_cast<float *>(pDst)[x] = (1.0f - nMask) * reinterpret_cast<float *>(pDst)[x] + nMask * reinterpret_cast<const float *>(pSrc1)[x];
       }
       pDst += nDstPitch;
       pSrc1 += nSrc1Pitch;
       pMask += nSrc2Pitch;
     }
   }

   void merge32_luma_411_c(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch,
     const uint8_t *pMask, ptrdiff_t nMaskPitch, int nWidth, int nHeight)
   {
     for (int y = 0; y < nHeight; ++y) {
       const float *mask = reinterpret_cast<const float *>(pMask);
       float *dst = reinterpret_cast<float *>(pDst);
       const float *src = reinterpret_cast<const float *>(pSrc1);
       for (int x = 0; x < nWidth; ++x) {
         // Float counterpart of the integer pairwise 4:1:1 reduction; no integer rounding.
         const float m = ((mask[x * 4] + mask[x * 4 + 1]) + (mask[x * 4 + 2] + mask[x * 4 + 3])) * 0.25f;
         dst[x] = (1.0f - m) * dst[x] + m * src[x];
       }
       pDst += nDstPitch;
       pSrc1 += nSrc1Pitch;
       pMask += nMaskPitch;
     }
   }

   template<bool allow_leftminus1>
   void internal_merge32_luma_422_mpeg2_c(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch,
     const uint8_t *pMask, ptrdiff_t nSrc2Pitch, int nWidth, int nHeight)
   {
     for (int y = 0; y < nHeight; ++y)
     {
       float right;
       right = allow_leftminus1 ?
         reinterpret_cast<const float*>(pMask)[-1]:
         reinterpret_cast<const float*>(pMask)[0];

       for (int x = 0; x < nWidth; ++x)
       {
         float left = right;
         const float mid = reinterpret_cast<const float *>(pMask)[x * 2];
         right = reinterpret_cast<const float *>(pMask)[x * 2 + 1];

         const float nMask = 0.25f * (left + 2 * mid + right);
         // 422: both width is halved, 1-2-1 weighted averaging from 3 pixels of full size mask

         reinterpret_cast<float *>(pDst)[x] = (1.0f - nMask) * reinterpret_cast<float *>(pDst)[x] + nMask * reinterpret_cast<const float *>(pSrc1)[x];
       }
       pDst += nDstPitch;
       pSrc1 += nSrc1Pitch;
       pMask += nSrc2Pitch;
     }
   }

   void merge32_luma_422_mpeg2_c(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch,
     const uint8_t *pMask, ptrdiff_t nSrc2Pitch, int nWidth, int nHeight)
   {
     internal_merge32_luma_422_mpeg2_c<false>(pDst, nDstPitch, pSrc1, nSrc1Pitch, pMask, nSrc2Pitch, nWidth, nHeight);
   }

   void merge32_luma_422_mpeg2_allow_leftminus1_c(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch,
     const uint8_t *pMask, ptrdiff_t nSrc2Pitch, int nWidth, int nHeight)
   {
     internal_merge32_luma_422_mpeg2_c<true>(pDst, nDstPitch, pSrc1, nSrc1Pitch, pMask, nSrc2Pitch, nWidth, nHeight);
   }

} } } }