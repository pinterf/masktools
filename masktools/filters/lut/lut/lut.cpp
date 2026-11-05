#include "lut.h"

using namespace Filtering;

void Filtering::MaskTools::Filters::Lut::Single::lut_c(uint8_t * dstp, ptrdiff_t dst_pitch, int width, int height, const uint8_t * lut)
{
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++) {
            dstp[x] = lut[dstp[x]];
        }
        dstp += dst_pitch;
    }
}

void Filtering::MaskTools::Filters::Lut::Single::realtime8_c(uint8_t *dstp, ptrdiff_t dst_pitch, int width, int height, Parser::Context &ctx)
{
  for (int y = 0; y < height; y++)
  {
    ctx.set_row(y);
    for (int x = 0; x < width; x++) {
      ctx.set_column(x);
      dstp[x] = ctx.compute_byte_x(dstp[x]);
    }
    dstp += dst_pitch;
  }
}


void Filtering::MaskTools::Filters::Lut::Single::realtime32_c(uint8_t *dstp, ptrdiff_t dst_pitch, int width, int height, bool chroma, Parser::Context &ctx)
{
  for (int y = 0; y < height; y++)
  {
    ctx.set_row(y);
    for (int x = 0; x < width; x++) {
      ctx.set_column(x);
      reinterpret_cast<float *>(dstp)[x] = ctx.compute_float_x(reinterpret_cast<float *>(dstp)[x], chroma);
    }
    dstp += dst_pitch;
  }
}

