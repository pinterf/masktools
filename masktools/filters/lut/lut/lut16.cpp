#include "lut.h"

using namespace Filtering;


void Filtering::MaskTools::Filters::Lut::Single::lut16_c(uint8_t * pDst, ptrdiff_t nDstPitch, int nWidth, int nHeight, const uint16_t *lut, int bits_per_pixel)
{
    // 16 bit: min is a no-op, the lut covers every input
    const int max_pixel_value = (1 << bits_per_pixel) - 1;
    for ( int y = 0; y < nHeight; y++ )
    {
        auto pDst16 = reinterpret_cast<uint16_t*>(pDst);
        int x;
        const int wmod2 = nWidth / 2 * 2;
        for (x = 0; x < wmod2; x += 2) {
            const int pixel0 = min((int)pDst16[x], max_pixel_value);
            const int pixel1 = min((int)pDst16[x + 1], max_pixel_value);
            pDst16[x] = lut[pixel0];
            pDst16[x + 1] = lut[pixel1];
        }
        if (wmod2 != nWidth)
            pDst16[x] = lut[min((int)pDst16[x], max_pixel_value)];
        pDst += nDstPitch;
    }
}

void Filtering::MaskTools::Filters::Lut::Single::realtime16_c(uint8_t* dstp, ptrdiff_t dst_pitch, int width, int height, Parser::Context& ctx, int bits_per_pixel)
{
  const int max_pixel_value = (1 << bits_per_pixel) - 1;
  for (int y = 0; y < height; y++)
  {
    ctx.set_row(y);
    for (int x = 0; x < width; x++) {
      ctx.set_column(x);
      const int pixel = min((int)reinterpret_cast<uint16_t*>(dstp)[x], max_pixel_value);
      reinterpret_cast<uint16_t*>(dstp)[x] = ctx.compute_word_x(pixel, bits_per_pixel);
    }
    dstp += dst_pitch;
  }
}
