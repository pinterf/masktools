#include "lutxy.h"
#include <type_traits>

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


template<int bits_per_pixel>
static void lut16_t_c(uint8_t *dstp, ptrdiff_t nDstPitch, const uint8_t *srcp, ptrdiff_t nSrcPitch, int nWidth, int nHeight, const uint16_t *lut)
{
  const uint16_t max_pixel_value = (1 << bits_per_pixel) - 1;
  for (int y = 0; y < nHeight; y++)
  {
    for (int x = 0; x < nWidth; x++) {
      uint16_t pixelX = reinterpret_cast<uint16_t *>(dstp)[x];
      if (bits_per_pixel != 16) pixelX = min(pixelX, max_pixel_value);
      uint16_t pixelY = reinterpret_cast<const uint16_t *>(srcp)[x];
      if (bits_per_pixel != 16) pixelY = min(pixelY, max_pixel_value);
      typedef typename std::conditional< bits_per_pixel == 16 , size_t , int>::type safe_index_t;
      reinterpret_cast<uint16_t *>(dstp)[x] = lut[((safe_index_t)pixelX << bits_per_pixel) + pixelY];
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

template<int bits_per_pixel>
static void realtime16_t_c(uint8_t *dstp, ptrdiff_t dst_pitch, const uint8_t *srcp, ptrdiff_t nSrcPitch, int width, int height, Parser::Context &ctx)
{
  const uint16_t max_pixel_value = (1 << bits_per_pixel) - 1;
  for (int y = 0; y < height; y++)
  {
    for (int x = 0; x < width; x++) {
      uint16_t pixelX = reinterpret_cast<uint16_t *>(dstp)[x];
      if (bits_per_pixel != 16) pixelX = min(pixelX, max_pixel_value); // clamp input below 16 bit
      uint16_t pixelY = reinterpret_cast<const uint16_t *>(srcp)[x];
      if (bits_per_pixel != 16) pixelY = min(pixelY, max_pixel_value); // clamp input below 16 bit

      uint16_t result = ctx.compute_word_xy<bits_per_pixel>(pixelX, pixelY);

      reinterpret_cast<uint16_t *>(dstp)[x] = result;
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

template void lut16_t_c<10>(uint8_t *dstp, ptrdiff_t dst_pitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int width, int height, const uint16_t* lut);
template void lut16_t_c<12>(uint8_t *dstp, ptrdiff_t dst_pitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int width, int height, const uint16_t* lut);
template void lut16_t_c<14>(uint8_t *dstp, ptrdiff_t dst_pitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int width, int height, const uint16_t* lut);
template void lut16_t_c<16>(uint8_t *dstp, ptrdiff_t dst_pitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int width, int height, const uint16_t* lut);

template void realtime16_t_c<10>(uint8_t *dstp, ptrdiff_t dst_pitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int width, int height, Parser::Context &ctx);
template void realtime16_t_c<12>(uint8_t *dstp, ptrdiff_t dst_pitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int width, int height, Parser::Context &ctx);
template void realtime16_t_c<14>(uint8_t *dstp, ptrdiff_t dst_pitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int width, int height, Parser::Context &ctx);
template void realtime16_t_c<16>(uint8_t *dstp, ptrdiff_t dst_pitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int width, int height, Parser::Context &ctx);

namespace Filtering {
  namespace MaskTools {
    namespace Filters {
      namespace Lut {
        namespace Dual {
          Processor16 *lut10_c = &lut16_t_c<10>;
          Processor16 *lut12_c = &lut16_t_c<12>;
          Processor16 *lut14_c = &lut16_t_c<14>;
          Processor16 *lut16_c = &lut16_t_c<16>;

          ProcessorCtx *realtime10_c = &realtime16_t_c<10>;
          ProcessorCtx *realtime12_c = &realtime16_t_c<12>;
          ProcessorCtx *realtime14_c = &realtime16_t_c<14>;
          ProcessorCtx *realtime16_c = &realtime16_t_c<16>;
        }
      }
    }
  }
}

