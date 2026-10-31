#ifndef __Mt_Stacked_H__
#define __Mt_Stacked_H__

// Stacked 16 bit (Dither era, stacked=true): an 8 bit clip of double height, the most significant bytes in the
// upper half of each plane, the least significant bytes in the lower half (at origheight / 2). The filters work
// on a native 16 bit copy of the planes; no 16 bit pixel type is needed in the host (Avisynth 2.6).

#include "../../../common/plane/plane.h"
#include <cstdint>
#include <cstdlib>

namespace Filtering { namespace MaskTools {

class NativePlaneBuffer {
  uint8_t *buffer = nullptr;
  NativePlaneBuffer(const NativePlaneBuffer &) = delete;
  NativePlaneBuffer &operator=(const NativePlaneBuffer &) = delete;
public:
  NativePlaneBuffer() {}
  ~NativePlaneBuffer() { if (buffer) _aligned_free(buffer); }

  // native 16 bit plane of the stacked plane's size (height / 2), the content converted from it
  Plane<uint8_t> from_stacked(const Plane<const uint8_t> &stacked)
  {
    const int width = stacked.width();
    const int height = stacked.height() / 2;
    const ptrdiff_t pitch = ((ptrdiff_t)width * sizeof(uint16_t) + 63) & ~(ptrdiff_t)63;
    if (buffer) _aligned_free(buffer);
    buffer = static_cast<uint8_t *>(_aligned_malloc(pitch * (height > 0 ? height : 1), 64));
    const uint8_t *msb = stacked.data();
    const uint8_t *lsb = stacked.data() + stacked.pitch() * (stacked.origheight() / 2);
    uint8_t *dst = buffer;
    for (int y = 0; y < height; y++) {
      uint16_t *dst16 = reinterpret_cast<uint16_t *>(dst);
      for (int x = 0; x < width; x++)
        dst16[x] = (uint16_t)((msb[x] << 8) | lsb[x]);
      msb += stacked.pitch();
      lsb += stacked.pitch();
      dst += pitch;
    }
    return Plane<uint8_t>(buffer, pitch, width, height, sizeof(uint16_t), height);
  }

  // the native plane back to the stacked plane
  static void to_stacked(const Plane<uint8_t> &native, const Plane<uint8_t> &stacked)
  {
    uint8_t *msb = stacked.data();
    uint8_t *lsb = stacked.data() + stacked.pitch() * (stacked.origheight() / 2);
    const uint8_t *src = native.data();
    for (int y = 0; y < native.height(); y++) {
      const uint16_t *src16 = reinterpret_cast<const uint16_t *>(src);
      for (int x = 0; x < native.width(); x++) {
        msb[x] = (uint8_t)(src16[x] >> 8);
        lsb[x] = (uint8_t)(src16[x] & 0xFF);
      }
      msb += stacked.pitch();
      lsb += stacked.pitch();
      src += native.pitch();
    }
  }
};

} } // namespace MaskTools, Filtering

#endif // __Mt_Stacked_H__
