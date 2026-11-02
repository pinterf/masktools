#include "gradient.h"
#include <type_traits>

using namespace Filtering;

//template<typename pixel_t, typename sad_t>
//using Distortion sad_t (*)(const pixel_t *pSrc, ptrdiff_t nSrcPitch, const pixel_t *pRef, ptrdiff_t nRefPitch, int nPrecision);
typedef int (Distorsion)(const uint8_t *pSrc, ptrdiff_t nSrcPitch, const uint8_t *pRef, ptrdiff_t nRefPitch, int nPrecision);
typedef int (Distorsion16)(const uint16_t *pSrc, ptrdiff_t nSrcPitch, const uint16_t *pRef, ptrdiff_t nRefPitch, int nPrecision);
typedef float (Distorsion_f)(const float *pSrc, ptrdiff_t nSrcPitch, const float *pRef, ptrdiff_t nRefPitch, int nPrecision);

template<int nBlockSizeX, int nBlockSizeY> 
int sad(const uint8_t *pSrc, ptrdiff_t nSrcPitch, const uint8_t *pRef, ptrdiff_t nRefPitch, int nPrecision)
{
  // pitches are already bit_depth corrected
   int nSad = 0;

   for ( int y = 0; y < nBlockSizeY; y++, pSrc += nSrcPitch * nPrecision, pRef += nRefPitch * nPrecision )
      for ( int x = 0; x < nBlockSizeX; x++ )
         nSad += abs<int>( pSrc[x * nPrecision] - pRef[x * nPrecision] );

   return nSad;
}

template<int nBlockSizeX, int nBlockSizeY>
int sad16(const uint16_t *pSrc, ptrdiff_t nSrcPitch, const uint16_t *pRef, ptrdiff_t nRefPitch, int nPrecision)
{
  // pitches are already bit_depth corrected
  int nSad = 0;

  for (int y = 0; y < nBlockSizeY; y++, pSrc += nSrcPitch * nPrecision, pRef += nRefPitch * nPrecision)
    for (int x = 0; x < nBlockSizeX; x++)
      nSad += abs<int>(pSrc[x * nPrecision] - pRef[x * nPrecision]);

  return nSad;
}

template<int nBlockSizeX, int nBlockSizeY>
float sad32(const float *pSrc, ptrdiff_t nSrcPitch, const float *pRef, ptrdiff_t nRefPitch, int nPrecision)
{
  // pitches are already bit_depth corrected
  float nSad = 0;

  for (int y = 0; y < nBlockSizeY; y++, pSrc += nSrcPitch * nPrecision, pRef += nRefPitch * nPrecision)
    for (int x = 0; x < nBlockSizeX; x++)
      nSad += abs<float>(pSrc[x * nPrecision] - pRef[x * nPrecision]);

  return nSad;
}

template<Distorsion distorsion, int nBlockSizeX, int nBlockSizeY> 
void generic_c(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, const uint8_t *pRef, ptrdiff_t nRefPitch, int nWidth, int nHeight, int nX, int nY, int nMinimum, int nMaximum, int nPrecision)
{
 
   int x, y;

   // the source block spans (size - 1) * precision + 1 pixels: it must fit in the plane, else no position is valid
   const int nBlockWidth = (nBlockSizeX - 1) * nPrecision + 1;
   const int nBlockHeight = (nBlockSizeY - 1) * nPrecision + 1;
   if (nWidth < nBlockWidth || nHeight < nBlockHeight) {
      for (y = 0; y < nHeight; y++)
         memset(&pDst[y * nDstPitch], 0, nWidth);
      return;
   }
   if (nX > nWidth - nBlockWidth) nX = nWidth - nBlockWidth;
   if (nY > nHeight - nBlockHeight) nY = nHeight - nBlockHeight;

   pSrc += nX + nY * nSrcPitch;

   const int max_pixel_value = 255;

   for ( y = 0; y < nHeight - nBlockSizeY * nPrecision + nPrecision; y++, pRef += nRefPitch, pDst += nDstPitch )
   {
      for ( x = 0; x < nWidth - nBlockSizeX * nPrecision + nPrecision; x++ )
         pDst[x] = clip<uint8_t, int>( (distorsion( pSrc, nSrcPitch, &pRef[x], nRefPitch, nPrecision ) - nMinimum) * max_pixel_value / (nMaximum - nMinimum) );

      for ( ; x < nWidth; x++ )
         pDst[x] = pDst[nWidth - nBlockSizeX * nPrecision + nPrecision - 1];
   }
   for ( y = 0; y < nBlockSizeY * nPrecision - nPrecision; y++ )
      memcpy( &pDst[y * nDstPitch], pDst - nDstPitch, nWidth );
}

template<Distorsion16 distorsion, int nBlockSizeX, int nBlockSizeY>
void generic16_c(uint16_t *pDst, ptrdiff_t nDstPitch, const uint16_t *pSrc, ptrdiff_t nSrcPitch, const uint16_t *pRef, ptrdiff_t nRefPitch, int nWidth, int nHeight, int nX, int nY, int nMinimum, int nMaximum, int nPrecision, int bits_per_pixel)
{
  nSrcPitch /= sizeof(uint16_t);
  nDstPitch /= sizeof(uint16_t);
  nRefPitch /= sizeof(uint16_t);

  int x, y;

  // the source block spans (size - 1) * precision + 1 pixels: it must fit in the plane, else no position is valid
  const int nBlockWidth = (nBlockSizeX - 1) * nPrecision + 1;
  const int nBlockHeight = (nBlockSizeY - 1) * nPrecision + 1;
  if (nWidth < nBlockWidth || nHeight < nBlockHeight) {
    for (y = 0; y < nHeight; y++)
      memset(&pDst[y * nDstPitch], 0, nWidth * sizeof(uint16_t));
    return;
  }
  if (nX > nWidth - nBlockWidth) nX = nWidth - nBlockWidth;
  if (nY > nHeight - nBlockHeight) nY = nHeight - nBlockHeight;

  pSrc += nX + nY * nSrcPitch;

  const int max_pixel_value = (1 << bits_per_pixel) - 1;

  for (y = 0; y < nHeight - nBlockSizeY * nPrecision + nPrecision; y++, pRef += nRefPitch, pDst += nDstPitch)
  {
    for (x = 0; x < nWidth - nBlockSizeX * nPrecision + nPrecision; x++)
      pDst[x] = clip<uint16_t, int>((distorsion(pSrc, nSrcPitch, &pRef[x], nRefPitch, nPrecision) - nMinimum) * max_pixel_value / (nMaximum - nMinimum));

    for (; x < nWidth; x++)
      pDst[x] = pDst[nWidth - nBlockSizeX * nPrecision + nPrecision - 1];
  }
  for (y = 0; y < nBlockSizeY * nPrecision - nPrecision; y++)
    memcpy(&pDst[y * nDstPitch], pDst - nDstPitch, nWidth * sizeof(uint16_t));
}

template<Distorsion_f distorsion, int nBlockSizeX, int nBlockSizeY>
void generic_f_c(float *pDst, ptrdiff_t nDstPitch, const float *pSrc, ptrdiff_t nSrcPitch, const float *pRef, ptrdiff_t nRefPitch, int nWidth, int nHeight, int nX, int nY, float nMinimum, float nMaximum, int nPrecision)
{
  nSrcPitch /= sizeof(float);
  nDstPitch /= sizeof(float);
  nRefPitch /= sizeof(float);

  int x, y;

  // the source block spans (size - 1) * precision + 1 pixels: it must fit in the plane, else no position is valid
  const int nBlockWidth = (nBlockSizeX - 1) * nPrecision + 1;
  const int nBlockHeight = (nBlockSizeY - 1) * nPrecision + 1;
  if (nWidth < nBlockWidth || nHeight < nBlockHeight) {
    for (y = 0; y < nHeight; y++)
      memset(&pDst[y * nDstPitch], 0, nWidth * sizeof(float));
    return;
  }
  if (nX > nWidth - nBlockWidth) nX = nWidth - nBlockWidth;
  if (nY > nHeight - nBlockHeight) nY = nHeight - nBlockHeight;

  pSrc += nX + nY * nSrcPitch;

  for (y = 0; y < nHeight - nBlockSizeY * nPrecision + nPrecision; y++, pRef += nRefPitch, pDst += nDstPitch)
  {
    for (x = 0; x < nWidth - nBlockSizeX * nPrecision + nPrecision; x++)
      pDst[x] = (distorsion(pSrc, nSrcPitch, &pRef[x], nRefPitch, nPrecision) - nMinimum) / (nMaximum - nMinimum);

    for (; x < nWidth; x++)
      pDst[x] = pDst[nWidth - nBlockSizeX * nPrecision + nPrecision - 1];
  }
  for (y = 0; y < nBlockSizeY * nPrecision - nPrecision; y++)
    memcpy(&pDst[y * nDstPitch], pDst - nDstPitch, nWidth * sizeof(float));
}

namespace Filtering { namespace MaskTools { namespace Filters { namespace Gradient {

Processor *sad_c  = &generic_c<sad<16, 16>, 16, 16>;
Processor16 *sad_16_c = &generic16_c<sad16<16, 16>, 16, 16>;
Processor32 *sad_32_c = &generic_f_c<sad32<16, 16>, 16, 16>;

} } } }
