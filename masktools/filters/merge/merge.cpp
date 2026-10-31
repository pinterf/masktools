#include "merge.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Merge {

   void merge_c(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch,
      const uint8_t *pMask, ptrdiff_t nSrc2Pitch, int nWidth, int nHeight)
   {
      for (int y = 0; y < nHeight; ++y)
      {
         for (int x = 0; x < nWidth; ++x) {
           const int nMask = pMask[x];
           if (nMask == 255)
             pDst[x] = pSrc1[x]; // max mask value (255): keep source
           else if(nMask != 0)
             pDst[x] = static_cast<uint8_t>(((256 - int(nMask)) * pDst[x] + int(nMask) * pSrc1[x] + 128) >> 8);
           // nMask == 0: keep pDst as is
         }
         pDst += nDstPitch;
         pSrc1 += nSrc1Pitch;
         pMask += nSrc2Pitch;
      }
   }

   // https://poynton.ca/PDFs/Merging_RGB_and_422.pdf
   // +------+------+
   // | 0.25 | 0.25 |
   // |------+------|
   // | 0.25 | 0.25 |
   // +------+------+


   void merge_luma_420_c(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch,
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

   // ------+------+-------+
   // 0.125 | 0.25 | 0.125 |
   // ------|------+-------|
   // 0.125 | 0.25 | 0.125 |
   // ------+------+-------+
   template<bool allow_leftminus1>
   void internal_merge_luma_420_mpeg2_c(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch,
     const uint8_t *pMask, ptrdiff_t nSrc2Pitch, int nWidth, int nHeight)
   {
     for (int y = 0; y < nHeight; ++y)
     {
       int right = allow_leftminus1 ? pMask[-1] + pMask[-1 + nSrc2Pitch] : pMask[0] + pMask[0 + nSrc2Pitch];
       for (int x = 0; x < nWidth; ++x)
       {
         int left = right;
         const int mid = pMask[x * 2] + pMask[x * 2 + nSrc2Pitch];
         right = pMask[x * 2 + 1] + pMask[x * 2 + nSrc2Pitch + 1];
         // 420: both width and height is halved, mpeg2: 1-2-1 weighted averaging from 6 pixels of full size mask
         const int nMask = (left + 2 * mid + right + 4) >> 3;
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

   void merge_luma_420_mpeg2_c(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch,
     const uint8_t *pMask, ptrdiff_t nSrc2Pitch, int nWidth, int nHeight) {
     internal_merge_luma_420_mpeg2_c<false>(pDst, nDstPitch, pSrc1, nSrc1Pitch, pMask, nSrc2Pitch, nWidth, nHeight);
   }

   void merge_luma_420_mpeg2_allow_leftminus1_c(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch,
     const uint8_t *pMask, ptrdiff_t nSrc2Pitch, int nWidth, int nHeight) {
     internal_merge_luma_420_mpeg2_c<true>(pDst, nDstPitch, pSrc1, nSrc1Pitch, pMask, nSrc2Pitch, nWidth, nHeight);
   }

   // ------+------+-------+
   // 1/16  | 1/8  | 1/16  |
   // ------|------+-------|
   // 1/8   | 1/4  | 1/8   |
   // ------|------+-------|
   // 1/16  | 1/8  | 1/16  |
   // ------+------+-------+
   template<bool allow_leftminus1>
   void internal_merge_luma_420_topleft_c(uint8_t* pDst, ptrdiff_t nDstPitch, const uint8_t* pSrc1, ptrdiff_t nSrc1Pitch,
     const uint8_t* pMask, ptrdiff_t nSrc2Pitch, int nWidth, int nHeight)
   {
     constexpr int left_or_same = allow_leftminus1 ? -1 : 0;

     // top line is special: no top
     {
       constexpr int top_or_same = 0;  // for MASK420_TOPLEFT, later -nMaskPitch
       int right = pMask[left_or_same + top_or_same] + (pMask[left_or_same] << 1) + pMask[left_or_same + nSrc2Pitch];
       for (int x = 0; x < nWidth; ++x)
       {
         int left = right;
         const int mid = pMask[x * 2 + top_or_same] + (pMask[x * 2] << 1) + pMask[x * 2 + nSrc2Pitch];
         right = pMask[x * 2 + 1 + top_or_same] + (pMask[x * 2 + 1] << 1) + pMask[x * 2 + nSrc2Pitch + 1];
         // 420: both width and height is halved, topleft:
         // 1-2-1
         // 2-4-2
         // 1-2-1 weighted averaging from 9 pixels of full size mask
         const int nMask = (left + 2 * mid + right + 16) >> 4;
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

     // rest
     for (int y = 1; y < nHeight; ++y)
     {
       const auto top_or_same = -nSrc2Pitch; // index to the previous line
       int right = pMask[left_or_same + top_or_same] + (pMask[left_or_same] << 1) + pMask[left_or_same + nSrc2Pitch];
       for (int x = 0; x < nWidth; ++x)
       {
         int left = right;
         const int mid = pMask[x * 2 + top_or_same] + (pMask[x * 2] << 1) + pMask[x * 2 + nSrc2Pitch];
         right = pMask[x * 2 + 1 + top_or_same]  + (pMask[x * 2 + 1] << 1) + pMask[x * 2 + nSrc2Pitch + 1];
         // 420: both width and height is halved, topleft:
         // 1-2-1
         // 2-4-2
         // 1-2-1 weighted averaging from 9 pixels of full size mask
         const int nMask = (left + 2 * mid + right + 16) >> 4;
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

   void merge_luma_420_topleft_c(uint8_t* pDst, ptrdiff_t nDstPitch, const uint8_t* pSrc1, ptrdiff_t nSrc1Pitch,
     const uint8_t* pMask, ptrdiff_t nSrc2Pitch, int nWidth, int nHeight) {
     internal_merge_luma_420_topleft_c<false>(pDst, nDstPitch, pSrc1, nSrc1Pitch, pMask, nSrc2Pitch, nWidth, nHeight);
   }

   void merge_luma_420_topleft_allow_leftminus1_c(uint8_t* pDst, ptrdiff_t nDstPitch, const uint8_t* pSrc1, ptrdiff_t nSrc1Pitch,
     const uint8_t* pMask, ptrdiff_t nSrc2Pitch, int nWidth, int nHeight) {
     internal_merge_luma_420_topleft_c<true>(pDst, nDstPitch, pSrc1, nSrc1Pitch, pMask, nSrc2Pitch, nWidth, nHeight);
   }

   void merge_luma_422_c(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch,
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

   template<bool allow_leftminus1>
   void internal_merge_luma_422_mpeg2_c(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch,
     const uint8_t *pMask, ptrdiff_t nSrc2Pitch, int nWidth, int nHeight)
   {
     for (int y = 0; y < nHeight; ++y)
     {
       int right = allow_leftminus1 ? pMask[-1] : pMask[0];
       for (int x = 0; x < nWidth; ++x)
       {
         int left = right;
         const int mid = pMask[x * 2];
         right = pMask[x * 2 + 1];
         // 422: width is halved, mpeg2: 1-2-1 weighted averaging from 3 pixels of full size mask
         const int nMask = (left + 2 * mid + right + 2) >> 2;
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

   void merge_luma_422_mpeg2_c(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch,
     const uint8_t *pMask, ptrdiff_t nSrc2Pitch, int nWidth, int nHeight) {
     internal_merge_luma_422_mpeg2_c<false>(pDst, nDstPitch, pSrc1, nSrc1Pitch, pMask, nSrc2Pitch, nWidth, nHeight);
   }

   void merge_luma_422_mpeg2_allow_leftminus1_c(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch,
     const uint8_t *pMask, ptrdiff_t nSrc2Pitch, int nWidth, int nHeight) {
     internal_merge_luma_422_mpeg2_c<true>(pDst, nDstPitch, pSrc1, nSrc1Pitch, pMask, nSrc2Pitch, nWidth, nHeight);
   }

   void merge_luma_411_c(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch,
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

} } } }