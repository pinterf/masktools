#ifndef __Mt_merge_C_H__
#define __Mt_merge_C_H__

// Private C kernels, also compiled locally for each SIMD tier's tails.
#include "merge.h"
#include "merge_mask.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Merge {

namespace {

MT_FORCEINLINE static uint8_t merge8_core_c(uint8_t dst, uint8_t src, uint8_t mask) {
  const uint32_t numerator = uint32_t(dst) * (255 - mask) + uint32_t(src) * mask + 127;
  // 0x8081 = ceil(2^23 / 255); this reciprocal gives the exact quotient over the blend numerator range.
  return static_cast<uint8_t>((numerator * 0x8081u) >> 23);
}

   void merge_t_c(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch,
      const uint8_t *pMask, ptrdiff_t nSrc2Pitch, int nWidth, int nHeight)
   {
      for (int y = 0; y < nHeight; ++y)
      {
         for (int x = 0; x < nWidth; ++x) {
           const int nMask = pMask[x];
           pDst[x] = merge8_core_c(pDst[x], pSrc1[x], static_cast<uint8_t>(nMask));
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


   void merge_luma_420_t_c(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch,
      const uint8_t *pMask, ptrdiff_t nSrc2Pitch, int nWidth, int nHeight)
   {
      for (int y = 0; y < nHeight; ++y)
      {
         for (int x = 0; x < nWidth; ++x)
         {
           // 420: both width and height is halved, averaging from 4 pixels of full size mask
            const int nMask = (((pMask[x * 2] + pMask[x * 2 + nSrc2Pitch] + 1) >> 1) + ((pMask[x * 2 + 1] + pMask[x * 2 + nSrc2Pitch + 1] + 1) >> 1) + 1) >> 1;
            pDst[x] = merge8_core_c(pDst[x], pSrc1[x], static_cast<uint8_t>(nMask));
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
         pDst[x] = merge8_core_c(pDst[x], pSrc1[x], static_cast<uint8_t>(nMask));
       }
       pDst += nDstPitch;
       pSrc1 += nSrc1Pitch;
       pMask += nSrc2Pitch * 2;
     }
   }

   void merge_luma_420_mpeg2_t_c(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch,
     const uint8_t *pMask, ptrdiff_t nSrc2Pitch, int nWidth, int nHeight) {
     internal_merge_luma_420_mpeg2_c<false>(pDst, nDstPitch, pSrc1, nSrc1Pitch, pMask, nSrc2Pitch, nWidth, nHeight);
   }

   void merge_luma_420_mpeg2_allow_leftminus1_t_c(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch,
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
         const int nMask = (left + 2 * mid + right + 8) >> 4;
         pDst[x] = merge8_core_c(pDst[x], pSrc1[x], static_cast<uint8_t>(nMask));
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
         const int nMask = (left + 2 * mid + right + 8) >> 4;
         pDst[x] = merge8_core_c(pDst[x], pSrc1[x], static_cast<uint8_t>(nMask));
       }
       pDst += nDstPitch;
       pSrc1 += nSrc1Pitch;
       pMask += nSrc2Pitch * 2;
     }
   }

   void merge_luma_420_topleft_t_c(uint8_t* pDst, ptrdiff_t nDstPitch, const uint8_t* pSrc1, ptrdiff_t nSrc1Pitch,
     const uint8_t* pMask, ptrdiff_t nSrc2Pitch, int nWidth, int nHeight) {
     internal_merge_luma_420_topleft_c<false>(pDst, nDstPitch, pSrc1, nSrc1Pitch, pMask, nSrc2Pitch, nWidth, nHeight);
   }

   void merge_luma_420_topleft_allow_leftminus1_t_c(uint8_t* pDst, ptrdiff_t nDstPitch, const uint8_t* pSrc1, ptrdiff_t nSrc1Pitch,
     const uint8_t* pMask, ptrdiff_t nSrc2Pitch, int nWidth, int nHeight) {
     internal_merge_luma_420_topleft_c<true>(pDst, nDstPitch, pSrc1, nSrc1Pitch, pMask, nSrc2Pitch, nWidth, nHeight);
   }

   void merge_luma_422_t_c(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch,
     const uint8_t *pMask, ptrdiff_t nSrc2Pitch, int nWidth, int nHeight)
   {
     for (int y = 0; y < nHeight; ++y)
     {
       for (int x = 0; x < nWidth; ++x)
       {
         const int nMask = (pMask[x * 2] + pMask[x * 2 + 1] + 1) >> 1; // 422: only width is halved, averaging from two pixels of full size mask
         pDst[x] = merge8_core_c(pDst[x], pSrc1[x], static_cast<uint8_t>(nMask));
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
         pDst[x] = merge8_core_c(pDst[x], pSrc1[x], static_cast<uint8_t>(nMask));
       }
       pDst += nDstPitch;
       pSrc1 += nSrc1Pitch;
       pMask += nSrc2Pitch;
     }
   }

   void merge_luma_422_mpeg2_t_c(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch,
     const uint8_t *pMask, ptrdiff_t nSrc2Pitch, int nWidth, int nHeight) {
     internal_merge_luma_422_mpeg2_c<false>(pDst, nDstPitch, pSrc1, nSrc1Pitch, pMask, nSrc2Pitch, nWidth, nHeight);
   }

   void merge_luma_422_mpeg2_allow_leftminus1_t_c(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch,
     const uint8_t *pMask, ptrdiff_t nSrc2Pitch, int nWidth, int nHeight) {
     internal_merge_luma_422_mpeg2_c<true>(pDst, nDstPitch, pSrc1, nSrc1Pitch, pMask, nSrc2Pitch, nWidth, nHeight);
   }

   template<MaskFilter H>
   void internal_merge_luma_411_c(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch,
     const uint8_t *pMask, ptrdiff_t nSrc2Pitch, int nWidth, int nHeight)
   {
     for (int y = 0; y < nHeight; ++y)
     {
       for (int x = 0; x < nWidth; ++x)
         pDst[x] = merge8_core_c(pDst[x], pSrc1[x], get_mask_filtered_c<uint8_t, H, MF_ID>(pMask, nSrc2Pitch, x, y, nWidth, nHeight));
       pDst += nDstPitch;
       pSrc1 += nSrc1Pitch;
       pMask += nSrc2Pitch;
     }
   }

   void merge_luma_411_t_c(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch,
     const uint8_t *pMask, ptrdiff_t nSrc2Pitch, int nWidth, int nHeight) {
     internal_merge_luma_411_c<MF_TRI8>(pDst, nDstPitch, pSrc1, nSrc1Pitch, pMask, nSrc2Pitch, nWidth, nHeight);
   }

   void merge_luma_411_mpeg2_t_c(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch,
     const uint8_t *pMask, ptrdiff_t nSrc2Pitch, int nWidth, int nHeight) {
     internal_merge_luma_411_c<MF_TENT7>(pDst, nDstPitch, pSrc1, nSrc1Pitch, pMask, nSrc2Pitch, nWidth, nHeight);
   }

template<bool left, bool top>
void merge_luma_410_t_c(uint8_t *dst, ptrdiff_t dst_pitch, const uint8_t *src, ptrdiff_t src_pitch,
  const uint8_t *mask, ptrdiff_t mask_pitch, int width, int height)
{
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x)
      dst[x] = merge8_core_c(dst[x], src[x], get_mask_filtered_c<uint8_t, left ? MF_TENT7 : MF_TRI8, top ? MF_TENT7 : MF_TRI8>(mask, mask_pitch, x, y, width, height));
    dst += dst_pitch;
    src += src_pitch;
    mask += mask_pitch * 4;
  }
}

template<bool top_left>
void merge_luma_440_t_c(uint8_t *dst, ptrdiff_t dst_pitch, const uint8_t *src, ptrdiff_t src_pitch,
  const uint8_t *mask, ptrdiff_t mask_pitch, int width, int height)
{
  for (int y = 0; y < height; ++y) {
    const uint8_t *above = y == 0 ? mask : mask - mask_pitch;
    for (int x = 0; x < width; ++x) {
      // MPEG1/2 are vertically centered; top_left is vertically co-sited, with a 1-2-1 filter.
      const int m = top_left ? (above[x] + 2 * mask[x] + mask[mask_pitch + x] + 2) >> 2 :
        (mask[x] + mask[mask_pitch + x] + 1) >> 1;
      dst[x] = merge8_core_c(dst[x], src[x], static_cast<uint8_t>(m));
    }
    dst += dst_pitch;
    src += src_pitch;
    mask += mask_pitch * 2;
  }
}

} // namespace

} } } }

#endif // __Mt_merge_C_H__