#ifndef __Common_BaseFunctions_H__
#define __Common_BaseFunctions_H__

#include "../utils/utils.h"
#include "../constraints/constraints.h"

namespace Filtering { namespace Functions {
 
void memset_plane(uint8_t *ptr, ptrdiff_t pitch, int width, int height, uint8_t value);
void memset_plane_16(uint8_t *ptr, ptrdiff_t pitch, int width, int height, uint16_t value);
void memset_plane_32(uint8_t *ptr, ptrdiff_t pitch, int width, int height, float value);

void copy_plane(uint8_t *pDst, ptrdiff_t dst_pitch, const uint8_t *pSrc, ptrdiff_t src_pitch, int rowsize, int height);

CpuFlags get_cpu_flags();


}
}

#endif
