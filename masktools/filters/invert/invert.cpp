#include "invert.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Invert {

template<int bits_per_pixel>
void invert16_t_c(uint8_t *pDst, ptrdiff_t nDstPitch, int nWidth, int nHeight)
{
  uint16_t max_pixel_value_mask = (1 << bits_per_pixel) - 1;
  for (int j = 0; j < nHeight; j++)
  {
    for (int i = 0; i < nWidth; i++)
      reinterpret_cast<uint16_t *>(pDst)[i] = max_pixel_value_mask - reinterpret_cast<uint16_t *>(pDst)[i];
    pDst += nDstPitch;
  }
}

void invert_c(uint8_t *pDst, ptrdiff_t nDstPitch, int nWidth, int nHeight)
{
   for ( int j = 0; j < nHeight; j++ )
   {
      for ( int i = 0; i < nWidth; i++ )
         pDst[i] = 255 - pDst[i];
      pDst += nDstPitch;
   }
}

void invert32_c(uint8_t *pDst, ptrdiff_t nDstPitch, int nWidth, int nHeight)
{
  for (int j = 0; j < nHeight; j++)
  {
    for (int i = 0; i < nWidth; i++)
      reinterpret_cast<float *>(pDst)[i] = 1.0f - reinterpret_cast<float *>(pDst)[i];
    pDst += nDstPitch;
  }
}


template void invert16_t_c<10>(uint8_t *pDst, ptrdiff_t nDstPitch, int nWidth, int nHeight);
template void invert16_t_c<12>(uint8_t *pDst, ptrdiff_t nDstPitch, int nWidth, int nHeight);
template void invert16_t_c<14>(uint8_t *pDst, ptrdiff_t nDstPitch, int nWidth, int nHeight);
template void invert16_t_c<16>(uint8_t *pDst, ptrdiff_t nDstPitch, int nWidth, int nHeight);


Processor *invert10_c = &invert16_t_c<10>;
Processor *invert12_c = &invert16_t_c<12>;
Processor *invert14_c = &invert16_t_c<14>;
Processor *invert16_c = &invert16_t_c<16>;

} } } }
