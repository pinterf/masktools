#include "lutxyz.h"

using namespace Filtering;

void Filtering::MaskTools::Filters::Lut::Trial::lut_c(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch, const uint8_t *pSrc2, ptrdiff_t nSrc2Pitch, int nWidth, int nHeight, const uint8_t *lut)
{
   for ( int y = 0; y < nHeight; y++ )
   {
      for ( int x = 0; x < nWidth; x++ )
         pDst[x] = lut[(pDst[x]<<16) + (pSrc1[x]<<8) + (pSrc2[x])];
      pDst += nDstPitch;
      pSrc1 += nSrc1Pitch;
      pSrc2 += nSrc2Pitch;
   }
}

void Filtering::MaskTools::Filters::Lut::Trial::realtime8_c(uint8_t *dstp, ptrdiff_t dst_pitch, const uint8_t *srcp, ptrdiff_t nSrcPitch, const uint8_t *srcp2, ptrdiff_t nSrc2Pitch, int width, int height, Parser::Context &ctx)
{
  for (int y = 0; y < height; y++)
  {
    ctx.set_row(y);
    for (int x = 0; x < width; x++) {
      ctx.set_column(x);
      dstp[x] = ctx.compute_byte_xyz(dstp[x], srcp[x], srcp2[x]);
    }
    dstp += dst_pitch;
    srcp += nSrcPitch;
    srcp2 += nSrc2Pitch;
  }
}

void Filtering::MaskTools::Filters::Lut::Trial::realtime16_c(uint8_t *dstp, ptrdiff_t dst_pitch, const uint8_t *srcp, ptrdiff_t nSrcPitch, const uint8_t *srcp2, ptrdiff_t nSrc2Pitch, int width, int height, Parser::Context &ctx, int bits_per_pixel)
{
  const int max_pixel_value = (1 << bits_per_pixel) - 1;
  for (int y = 0; y < height; y++)
  {
    ctx.set_row(y);
    for (int x = 0; x < width; x++) {
      ctx.set_column(x);
      const int pixelX = min((int)reinterpret_cast<uint16_t *>(dstp)[x], max_pixel_value); // clamp input below 16 bit
      const int pixelY = min((int)reinterpret_cast<const uint16_t *>(srcp)[x], max_pixel_value); // clamp input below 16 bit
      const int pixelZ = min((int)reinterpret_cast<const uint16_t *>(srcp2)[x], max_pixel_value); // clamp input below 16 bit

      uint16_t result = ctx.compute_word_xyz(pixelX, pixelY, pixelZ, bits_per_pixel);

      reinterpret_cast<uint16_t *>(dstp)[x] = result;
    }
    dstp += dst_pitch;
    srcp += nSrcPitch;
    srcp2 += nSrc2Pitch;
  }
}

void Filtering::MaskTools::Filters::Lut::Trial::realtime32_c(uint8_t *dstp, ptrdiff_t dst_pitch, const uint8_t *srcp, ptrdiff_t nSrcPitch, const uint8_t *srcp2, ptrdiff_t nSrc2Pitch, int width, int height, bool chroma, Parser::Context &ctx)
{
  for (int y = 0; y < height; y++)
  {
    ctx.set_row(y);
    for (int x = 0; x < width; x++) {
      ctx.set_column(x);
      reinterpret_cast<float *>(dstp)[x] = ctx.compute_float_xyz(reinterpret_cast<float *>(dstp)[x], reinterpret_cast<const float *>(srcp)[x], reinterpret_cast<const float *>(srcp2)[x], chroma);
    }
    dstp += dst_pitch;
    srcp += nSrcPitch;
    srcp2 += nSrc2Pitch;
  }
}
