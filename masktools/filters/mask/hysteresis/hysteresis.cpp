#include "hysteresis.h"

using namespace Filtering;


struct Coordinates {
    int x;
    int y;

    Coordinates(int x_, int y_) : x(x_), y(y_) {}
};

typedef std::vector<Coordinates> CoordinatesList;

template<typename pixel_t, int bits_per_pixel, bool corners>
static void expand_mask(pixel_t* pDst, ptrdiff_t nDstPitch, const pixel_t* pSrc2, ptrdiff_t nSrc2Pitch, uint8_t* pTemp, int x, int y, int nWidth, int nHeight, CoordinatesList& coordinates)
{
    //CoordinatesList coordinates;
    coordinates.clear();
    
    const pixel_t mask_value = sizeof(pixel_t) == 4 ? (pixel_t)1.0f : (pixel_t)((1 << bits_per_pixel) - 1);
    nDstPitch /= sizeof(pixel_t);
    nSrc2Pitch /= sizeof(pixel_t);

    pTemp[0] = 1;
    pDst[0] = mask_value;

    coordinates.emplace_back(0, 0);

    while (!coordinates.empty())
    {
        /* pop last coordinates */
        Coordinates current = coordinates.back();
        coordinates.pop_back();

        /* check surrounding positions */
        int x_min = current.x == -x ? current.x : current.x - 1;
        int x_max = current.x == nWidth - x - 1 ? current.x : current.x + 1;
        int y_min = current.y == -y ? current.y : current.y - 1;
        int y_max = current.y == nHeight - y - 1 ? current.y : current.y + 1;

        for (int i = y_min; i <= y_max; i++) {
          for (int j = x_min; j <= x_max; j++) {
            // corners:
            // false   true
            //   +     + + +
            // + O +   + O +
            //   +     + + +
            if (corners || (i == current.y || j == current.x))
            {
              if (!pTemp[j + i * nWidth] && pSrc2[j + i * nSrc2Pitch]) {
                coordinates.emplace_back(j, i);
                pTemp[j + i * nWidth] = 1;
                pDst[j + i * nDstPitch] = mask_value;
              }
            }
          }
        }
    }
}

namespace Filtering { namespace MaskTools { namespace Filters { namespace Mask { namespace Hysteresis {
template<typename pixel_t, int bits_per_pixel, bool corners>
void hysteresis_c(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch,
  const uint8_t *pSrc2, ptrdiff_t nSrc2Pitch, uint8_t *pTemp, int width, int height)
{

  for (int y = 0; y < height; y++) // the plane can be a window: clear only its rows
    memset(pDst + y * nDstPitch, 0, width * sizeof(pixel_t));
  memset(pTemp, 0, width * height);
  CoordinatesList coordinates;

  for (int y = 0; y < height; y++)
  {
    for (int x = 0; x < width; x++) {
      if (!pTemp[x] && ((pixel_t *)pSrc1)[x] && ((pixel_t *)pSrc2)[x]) {
        expand_mask<pixel_t, bits_per_pixel, corners>((pixel_t *)pDst + x, nDstPitch, (pixel_t *)pSrc2 + x, nSrc2Pitch, pTemp + x, x, y, width, height, coordinates);
      }
      
    }
    pTemp += width;
    pSrc1 += nSrc1Pitch;
    pSrc2 += nSrc2Pitch;
    pDst += nDstPitch;
  }
}

template void hysteresis_c<uint8_t, 8, true>(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch, const uint8_t *pSrc2, ptrdiff_t nSrc2Pitch, uint8_t *pTemp, int width, int height);
template void hysteresis_c<uint16_t, 10, true>(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch, const uint8_t *pSrc2, ptrdiff_t nSrc2Pitch, uint8_t *pTemp, int width, int height);
template void hysteresis_c<uint16_t, 12, true>(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch, const uint8_t *pSrc2, ptrdiff_t nSrc2Pitch, uint8_t *pTemp, int width, int height);
template void hysteresis_c<uint16_t, 14, true>(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch, const uint8_t *pSrc2, ptrdiff_t nSrc2Pitch, uint8_t *pTemp, int width, int height);
template void hysteresis_c<uint16_t, 16, true>(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch, const uint8_t *pSrc2, ptrdiff_t nSrc2Pitch, uint8_t *pTemp, int width, int height);
template void hysteresis_c<float, 0, true>(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch, const uint8_t *pSrc2, ptrdiff_t nSrc2Pitch, uint8_t *pTemp, int width, int height);
template void hysteresis_c<uint8_t, 8, false>(uint8_t* pDst, ptrdiff_t nDstPitch, const uint8_t* pSrc1, ptrdiff_t nSrc1Pitch, const uint8_t* pSrc2, ptrdiff_t nSrc2Pitch, uint8_t* pTemp, int width, int height);
template void hysteresis_c<uint16_t, 10, false>(uint8_t* pDst, ptrdiff_t nDstPitch, const uint8_t* pSrc1, ptrdiff_t nSrc1Pitch, const uint8_t* pSrc2, ptrdiff_t nSrc2Pitch, uint8_t* pTemp, int width, int height);
template void hysteresis_c<uint16_t, 12, false>(uint8_t* pDst, ptrdiff_t nDstPitch, const uint8_t* pSrc1, ptrdiff_t nSrc1Pitch, const uint8_t* pSrc2, ptrdiff_t nSrc2Pitch, uint8_t* pTemp, int width, int height);
template void hysteresis_c<uint16_t, 14, false>(uint8_t* pDst, ptrdiff_t nDstPitch, const uint8_t* pSrc1, ptrdiff_t nSrc1Pitch, const uint8_t* pSrc2, ptrdiff_t nSrc2Pitch, uint8_t* pTemp, int width, int height);
template void hysteresis_c<uint16_t, 16, false>(uint8_t* pDst, ptrdiff_t nDstPitch, const uint8_t* pSrc1, ptrdiff_t nSrc1Pitch, const uint8_t* pSrc2, ptrdiff_t nSrc2Pitch, uint8_t* pTemp, int width, int height);
template void hysteresis_c<float, 0, false>(uint8_t* pDst, ptrdiff_t nDstPitch, const uint8_t* pSrc1, ptrdiff_t nSrc1Pitch, const uint8_t* pSrc2, ptrdiff_t nSrc2Pitch, uint8_t* pTemp, int width, int height);

Processor *hysteresis_8_c = &hysteresis_c<uint8_t, 8, true>;
Processor *hysteresis_10_c = &hysteresis_c<uint16_t, 10, true>;
Processor *hysteresis_12_c = &hysteresis_c<uint16_t, 12, true>;
Processor *hysteresis_14_c = &hysteresis_c<uint16_t, 14, true>;
Processor *hysteresis_16_c = &hysteresis_c<uint16_t, 16, true>;
Processor *hysteresis_32_c = &hysteresis_c<float, 0, true>; // float: bit_per_pixel n/a
Processor* hysteresis_8_nocorner_c = &hysteresis_c<uint8_t, 8, false>;
Processor* hysteresis_10_nocorner_c = &hysteresis_c<uint16_t, 10, false>;
Processor* hysteresis_12_nocorner_c = &hysteresis_c<uint16_t, 12, false>;
Processor* hysteresis_14_nocorner_c = &hysteresis_c<uint16_t, 14, false>;
Processor* hysteresis_16_nocorner_c = &hysteresis_c<uint16_t, 16, false>;
Processor* hysteresis_32_nocorner_c = &hysteresis_c<float, 0, false>; // float: bit_per_pixel n/a

} } } } }
