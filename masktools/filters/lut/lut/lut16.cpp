#include "lut.h"

using namespace Filtering;


template<int bits_per_pixel>
static void lut16_t_c(uint8_t * pDst, ptrdiff_t nDstPitch, int nWidth, int nHeight, const uint16_t *lut, int)
{
    constexpr int max_pixel_value = (1 << bits_per_pixel) - 1;
    for ( int y = 0; y < nHeight; y++ )
    {
        auto pDst16 = reinterpret_cast<uint16_t*>(pDst);
        int x;
        const int wmod2 = nWidth / 2 * 2;
        for (x = 0; x < wmod2; x+=2 ) {
            int pixelX = pDst16[x];
            if constexpr (bits_per_pixel != 16)
              pixelX = min(pixelX, max_pixel_value);

            pDst16[x] = lut[pixelX];

            pixelX = pDst16[x + 1];
            if constexpr (bits_per_pixel != 16)
              pixelX = min(pixelX, max_pixel_value);

            pDst16[x + 1] = lut[pixelX];

        }
        if (wmod2 != nWidth) {
          int pixelX = pDst16[x];
          if constexpr (bits_per_pixel != 16)
            pixelX = min(pixelX, max_pixel_value);

          pDst16[x] = lut[pixelX];
        }

        pDst += nDstPitch;
    }
}

template<int bits_per_pixel>
static void realtime16_t_c(uint8_t* dstp, ptrdiff_t dst_pitch, int width, int height, Parser::Context& ctx)
{
  constexpr int max_pixel_value = (1 << bits_per_pixel) - 1;
  for (int y = 0; y < height; y++)
  {
    for (int x = 0; x < width; x++) {
      int pixel = reinterpret_cast<uint16_t*>(dstp)[x];
      if constexpr(bits_per_pixel != 16) 
        pixel = min(pixel, max_pixel_value);

      const auto result = ctx.compute_word_x<bits_per_pixel>(pixel);

      reinterpret_cast<uint16_t*>(dstp)[x] = result;
    }
    dstp += dst_pitch;
  }
}

static void lut16_t_stacked_c(uint8_t* pDst, ptrdiff_t nDstPitch, int nWidth, int nHeight, const uint16_t* lut, int nOrigHeightForStacked)
{
  auto pLsb = pDst + nDstPitch * nOrigHeightForStacked / 2;

  for (int y = 0; y < nHeight / 2; y++)
  {
    for (int x = 0; x < nWidth; x++) {
      uint16_t value = lut[(pDst[x] << 8) + pLsb[x]];
      pDst[x] = value >> 8;
      pLsb[x] = value & 0xFF;
    }
    pDst += nDstPitch;
    pLsb += nDstPitch;
  }
}

template void realtime16_t_c<10>(uint8_t* dstp, ptrdiff_t dst_pitch, int width, int height, Parser::Context& ctx);
template void realtime16_t_c<12>(uint8_t* dstp, ptrdiff_t dst_pitch, int width, int height, Parser::Context& ctx);
template void realtime16_t_c<14>(uint8_t* dstp, ptrdiff_t dst_pitch, int width, int height, Parser::Context& ctx);
template void realtime16_t_c<16>(uint8_t* dstp, ptrdiff_t dst_pitch, int width, int height, Parser::Context& ctx);

template void lut16_t_c<10>(uint8_t* dstp, ptrdiff_t dst_pitch, int width, int height, const uint16_t* lut, int);
template void lut16_t_c<12>(uint8_t* dstp, ptrdiff_t dst_pitch, int width, int height, const uint16_t* lut, int);
template void lut16_t_c<14>(uint8_t* dstp, ptrdiff_t dst_pitch, int width, int height, const uint16_t* lut, int);
template void lut16_t_c<16>(uint8_t* dstp, ptrdiff_t dst_pitch, int width, int height, const uint16_t* lut, int);

namespace Filtering {
  namespace MaskTools {
    namespace Filters {
      namespace Lut {
        namespace Single {
          Processor16* lut10_c = &lut16_t_c<10>;
          Processor16* lut12_c = &lut16_t_c<12>;
          Processor16* lut14_c = &lut16_t_c<14>;
          Processor16* lut16_c = &lut16_t_c<16>;
          Processor16* lut16_stacked_c = &lut16_t_stacked_c;


          ProcessorCtx* realtime10_c = &realtime16_t_c<10>;
          ProcessorCtx* realtime12_c = &realtime16_t_c<12>;
          ProcessorCtx* realtime14_c = &realtime16_t_c<14>;
          ProcessorCtx* realtime16_c = &realtime16_t_c<16>;
        }
      }
    }
  }
}

