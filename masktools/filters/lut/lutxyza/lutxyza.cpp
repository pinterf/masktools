#include "lutxyza.h"

using namespace Filtering;

void Filtering::MaskTools::Filters::Lut::Quad::lut_c(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch, 
  const uint8_t *pSrc2, ptrdiff_t nSrc2Pitch, const uint8_t *pSrc3, ptrdiff_t nSrc3Pitch, int nWidth, int nHeight, const uint8_t *lut)
{
   for ( int y = 0; y < nHeight; y++ )
   {
      for (int x = 0; x < nWidth; x++) {
        // x64! int is not enough
        pDst[x] = lut[((size_t)(pDst[x]) << 24) + (pSrc1[x] << 16) + (pSrc2[x] << 8) + (pSrc3[x])];
      }
      pDst += nDstPitch;
      pSrc1 += nSrc1Pitch;
      pSrc2 += nSrc2Pitch;
      pSrc3 += nSrc3Pitch;
   }
}

void Filtering::MaskTools::Filters::Lut::Quad::realtime8_c(uint8_t *dstp, ptrdiff_t dst_pitch, const uint8_t *srcp, ptrdiff_t nSrcPitch, 
  const uint8_t *srcp2, ptrdiff_t nSrc2Pitch, const uint8_t *srcp3, ptrdiff_t nSrc3Pitch, int width, int height, Parser::Context &ctx)
{
  for (int y = 0; y < height; y++)
  {
    for (int x = 0; x < width; x++) {
      dstp[x] = ctx.compute_byte_xyza(dstp[x], srcp[x], srcp2[x], srcp3[x]);
    }
    dstp += dst_pitch;
    srcp += nSrcPitch;
    srcp2 += nSrc2Pitch;
    srcp3 += nSrc3Pitch;
  }
}

template<int bits_per_pixel>
static void realtime16_t_c(uint8_t *dstp, ptrdiff_t dst_pitch, const uint8_t *srcp, ptrdiff_t nSrcPitch, 
  const uint8_t *srcp2, ptrdiff_t nSrc2Pitch, const uint8_t *srcp3, ptrdiff_t nSrc3Pitch, int width, int height, Parser::Context &ctx)
{
  const uint16_t max_pixel_value = (1 << bits_per_pixel) - 1;
  for (int y = 0; y < height; y++)
  {
    for (int x = 0; x < width; x++) {
      uint16_t pixelX = reinterpret_cast<uint16_t *>(dstp)[x];
      if (bits_per_pixel != 16) pixelX = min(pixelX, max_pixel_value); // clamp input below 16 bit
      uint16_t pixelY = reinterpret_cast<const uint16_t *>(srcp)[x];
      if (bits_per_pixel != 16) pixelY = min(pixelY, max_pixel_value); // clamp input below 16 bit
      uint16_t pixelZ = reinterpret_cast<const uint16_t *>(srcp2)[x];
      if (bits_per_pixel != 16) pixelZ = min(pixelZ, max_pixel_value); // clamp input below 16 bit
      uint16_t pixelA = reinterpret_cast<const uint16_t *>(srcp3)[x];
      if (bits_per_pixel != 16) pixelA = min(pixelA, max_pixel_value); // clamp input below 16 bit

      uint16_t result = ctx.compute_word_xyza<bits_per_pixel>(pixelX, pixelY, pixelZ, pixelA);

      reinterpret_cast<uint16_t *>(dstp)[x] = result;
    }
    dstp += dst_pitch;
    srcp += nSrcPitch;
    srcp2 += nSrc2Pitch;
    srcp3 += nSrc3Pitch;
  }
}

void Filtering::MaskTools::Filters::Lut::Quad::realtime32_c(uint8_t *dstp, ptrdiff_t dst_pitch, const uint8_t *srcp, ptrdiff_t nSrcPitch,
  const uint8_t *srcp2, ptrdiff_t nSrc2Pitch, const uint8_t *srcp3, ptrdiff_t nSrc3Pitch, int width, int height, bool chroma, Parser::Context &ctx)
{
  for (int y = 0; y < height; y++)
  {
    for (int x = 0; x < width; x++) {
      reinterpret_cast<float *>(dstp)[x] = ctx.compute_float_xyza(reinterpret_cast<float *>(dstp)[x],
        reinterpret_cast<const float *>(srcp)[x],
        reinterpret_cast<const float *>(srcp2)[x],
        reinterpret_cast<const float *>(srcp3)[x],
        chroma);
    }
    dstp += dst_pitch;
    srcp += nSrcPitch;
    srcp2 += nSrc2Pitch;
    srcp3 += nSrc3Pitch;
  }
}

template void realtime16_t_c<10>(uint8_t *dstp, ptrdiff_t dst_pitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, const uint8_t *srcp2, ptrdiff_t nSrc2Pitch, const uint8_t *srcp3, ptrdiff_t nSrc3Pitch, int width, int height, Parser::Context &ctx);
template void realtime16_t_c<12>(uint8_t *dstp, ptrdiff_t dst_pitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, const uint8_t *srcp2, ptrdiff_t nSrc2Pitch, const uint8_t *srcp3, ptrdiff_t nSrc3Pitch, int width, int height, Parser::Context &ctx);
template void realtime16_t_c<14>(uint8_t *dstp, ptrdiff_t dst_pitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, const uint8_t *srcp2, ptrdiff_t nSrc2Pitch, const uint8_t *srcp3, ptrdiff_t nSrc3Pitch, int width, int height, Parser::Context &ctx);
template void realtime16_t_c<16>(uint8_t *dstp, ptrdiff_t dst_pitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, const uint8_t *srcp2, ptrdiff_t nSrc2Pitch, const uint8_t *srcp3, ptrdiff_t nSrc3Pitch, int width, int height, Parser::Context &ctx);

namespace Filtering {
  namespace MaskTools {
    namespace Filters {
      namespace Lut {
        namespace Quad {
          ProcessorCtx *realtime10_c = &realtime16_t_c<10>;
          ProcessorCtx *realtime12_c = &realtime16_t_c<12>;
          ProcessorCtx *realtime14_c = &realtime16_t_c<14>;
          ProcessorCtx *realtime16_c = &realtime16_t_c<16>;
        }
      }
    }
  }
}


