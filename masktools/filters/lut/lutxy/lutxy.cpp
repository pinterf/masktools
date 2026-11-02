#include "lutxy.h"

using namespace Filtering;

void Filtering::MaskTools::Filters::Lut::Dual::lut_c(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int nWidth, int nHeight, const uint8_t lut[65536])
{
   for ( int y = 0; y < nHeight; y++ )
   {
      for ( int x = 0; x < nWidth; x++ )
         pDst[x] = lut[(pDst[x]<<8) + pSrc[x]];
      pDst += nDstPitch;
      pSrc += nSrcPitch;
   }
}


void Filtering::MaskTools::Filters::Lut::Dual::lut16_c(uint8_t *dstp, ptrdiff_t nDstPitch, const uint8_t *srcp, ptrdiff_t nSrcPitch, int nWidth, int nHeight, const uint16_t *lut, int bits_per_pixel)
{
  // 16 bit: min is a no-op, the lut covers every input
  const int max_pixel_value = (1 << bits_per_pixel) - 1;
  for (int y = 0; y < nHeight; y++)
  {
    for (int x = 0; x < nWidth; x++) {
      const int pixelX = min((int)reinterpret_cast<uint16_t *>(dstp)[x], max_pixel_value);
      const int pixelY = min((int)reinterpret_cast<const uint16_t *>(srcp)[x], max_pixel_value);
      reinterpret_cast<uint16_t *>(dstp)[x] = lut[((size_t)pixelX << bits_per_pixel) + pixelY];
    }
    dstp += nDstPitch;
    srcp += nSrcPitch;
  }
}

void Filtering::MaskTools::Filters::Lut::Dual::realtime8_c(uint8_t *dstp, ptrdiff_t dst_pitch, const uint8_t *srcp, ptrdiff_t nSrcPitch, int width, int height, Parser::Context &ctx)
{
  for (int y = 0; y < height; y++)
  {
    for (int x = 0; x < width; x++) {
      dstp[x] = ctx.compute_byte_xy(dstp[x], srcp[x]);
    }
    dstp += dst_pitch;
    srcp += nSrcPitch;
  }
}

void Filtering::MaskTools::Filters::Lut::Dual::realtime16_c(uint8_t *dstp, ptrdiff_t dst_pitch, const uint8_t *srcp, ptrdiff_t nSrcPitch, int width, int height, Parser::Context &ctx, int bits_per_pixel)
{
  const int max_pixel_value = (1 << bits_per_pixel) - 1;
  for (int y = 0; y < height; y++)
  {
    for (int x = 0; x < width; x++) {
      const int pixelX = min((int)reinterpret_cast<uint16_t *>(dstp)[x], max_pixel_value); // clamp input below 16 bit
      const int pixelY = min((int)reinterpret_cast<const uint16_t *>(srcp)[x], max_pixel_value);
      reinterpret_cast<uint16_t *>(dstp)[x] = ctx.compute_word_xy(pixelX, pixelY, bits_per_pixel);
    }
    dstp += dst_pitch;
    srcp += nSrcPitch;
  }
}

void Filtering::MaskTools::Filters::Lut::Dual::realtime32_c(uint8_t *dstp, ptrdiff_t dst_pitch, const uint8_t *srcp, ptrdiff_t nSrcPitch, int width, int height, bool chroma, Parser::Context &ctx)
{
  for (int y = 0; y < height; y++)
  {
    for (int x = 0; x < width; x++) {
      reinterpret_cast<float *>(dstp)[x] = ctx.compute_float_xy(reinterpret_cast<float *>(dstp)[x], reinterpret_cast<const float *>(srcp)[x], chroma);
    }
    dstp += dst_pitch;
    srcp += nSrcPitch;
  }
}
