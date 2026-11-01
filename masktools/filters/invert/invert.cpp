#include "invert.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Invert {

void invert16_c(uint8_t *pDst, ptrdiff_t nDstPitch, int nWidth, int nHeight, int bits_per_pixel)
{
  const uint16_t max_pixel_value_mask = uint16_t((1 << bits_per_pixel) - 1);
  for (int j = 0; j < nHeight; j++)
  {
    for (int i = 0; i < nWidth; i++)
      reinterpret_cast<uint16_t *>(pDst)[i] = max_pixel_value_mask - reinterpret_cast<uint16_t *>(pDst)[i];
    pDst += nDstPitch;
  }
}

void invert_c(uint8_t *pDst, ptrdiff_t nDstPitch, int nWidth, int nHeight, int)
{
   for ( int j = 0; j < nHeight; j++ )
   {
      for ( int i = 0; i < nWidth; i++ )
         pDst[i] = 255 - pDst[i];
      pDst += nDstPitch;
   }
}

void invert32_c(uint8_t *pDst, ptrdiff_t nDstPitch, int nWidth, int nHeight, int)
{
  for (int j = 0; j < nHeight; j++)
  {
    for (int i = 0; i < nWidth; i++)
      reinterpret_cast<float *>(pDst)[i] = 1.0f - reinterpret_cast<float *>(pDst)[i];
    pDst += nDstPitch;
  }
}

} } } }
