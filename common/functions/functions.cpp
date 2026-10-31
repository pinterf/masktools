#include "functions.h"
#include <cstring>
#include <stdint.h>

using namespace Filtering;

void Functions::memset_plane(uint8_t *ptr, ptrdiff_t pitch, int width, int height, uint8_t value)
{
    if (pitch == width) {
        memset(ptr, value, width*height);
    } else {
        for (int y = 0; y < height; ++y) {
            memset(ptr, value, width);
            ptr += pitch;
        }
    }
}

void Functions::memset_plane_16(uint8_t *ptr, ptrdiff_t pitch, int width, int height, uint16_t value)
{
  if ((size_t)pitch == width * sizeof(uint16_t)) {
    std::fill_n((uint16_t *)ptr, width*height, value);
  }
  else {
    for (int y = 0; y < height; ++y) {
      std::fill_n((uint16_t *)ptr, width, value);
      ptr += pitch;
    }
  }
}

void Functions::memset_plane_32(uint8_t *ptr, ptrdiff_t pitch, int width, int height, float value)
{
  if ((size_t)pitch == width * sizeof(float)) {
    std::fill_n((float *)ptr, width*height, value);
  }
  else {
    for (int y = 0; y < height; ++y) {
      std::fill_n((float *)ptr, width, value);
      ptr += pitch;
    }
  }
}

void Functions::copy_plane(uint8_t *pDst, ptrdiff_t dst_pitch, const uint8_t *pSrc, ptrdiff_t src_pitch, int rowsize, int height)
{
    if (dst_pitch == rowsize && src_pitch == rowsize) {
        memcpy(pDst, pSrc, rowsize*height);
    } else {
        for (int y = 0; y < height; ++y) {
            memcpy(pDst, pSrc, rowsize);
            pDst += dst_pitch;
            pSrc += src_pitch;
        }
    }
}

