#include "functions.h"
#include <immintrin.h>
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

#if 0
// v2.2.3: using avisynth's GetCpuFlags
CpuFlags Functions::get_cpu_flags()
{
    int CPUInfo[4]; //eax, ebx, ecx, edx
    CpuFlags flags = CPU_NONE;

    __cpuid(CPUInfo, 1);

    if (CPUInfo[3] & 0x00800000) flags |= CPU_MMX;
    if (CPUInfo[3] & 0x02000000) flags |= CPU_ISSE;
    if (CPUInfo[3] & 0x04000000) flags |= CPU_SSE2;
    if (CPUInfo[2] & 0x00000001) flags |= CPU_SSE3;
    if (CPUInfo[2] & 0x00000200) flags |= CPU_SSSE3;
    if (CPUInfo[2] & 0x00080000) flags |= CPU_SSE4_1;
    if (CPUInfo[2] & 0x00100000) flags |= CPU_SSE4_2;

    return flags;
}
#endif
