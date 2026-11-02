#ifndef __Common_Plane_H__
#define __Common_Plane_H__

#include "../utils/utils.h"
#include <cstddef>

namespace Filtering {

template<typename T>
class Plane {

protected:

   T *pPixel;
   ptrdiff_t nPitch;
   int nWidth;
   int nHeight;
   int nPixelSize; // 1 for 8 bit, 2 for 10-16 bit, 4 for float
   int nOrigHeight; // the frame height, kept by offset(): stacked.h finds the LSB half at origheight / 2

public:

   Plane() : pPixel(NULL), nPitch(0), nWidth(0), nHeight(0), nPixelSize(1), nOrigHeight(0) {}
   Plane(T *pPixel, ptrdiff_t nPitch, int nWidth, int nHeight, int nPixelSize, int nOrigHeight) : pPixel(pPixel), nPitch(nPitch), nWidth(nWidth), nHeight(nHeight), nPixelSize(nPixelSize), nOrigHeight(nOrigHeight) { }

   Plane(const Plane<const T> &plane) : pPixel(plane.data()), nPitch(plane.pitch()), nWidth(plane.width()), nHeight(plane.height()), nPixelSize(plane.pixelsize()), nOrigHeight(plane.origheight())
   {
   }

   T* data() const { return pPixel; }
   ptrdiff_t pitch() const { return nPitch; }
   int width() const { return nWidth; }
   int height() const { return nHeight; }
   int pixelsize() const { return nPixelSize; }
   int origheight() const { return nOrigHeight; }

   Plane<T> offset(int x = 0, int y = 0, int w = 0, int h = 0) const 
   { 
     return Plane<T>( pPixel + x * nPixelSize + y * nPitch, nPitch, w, h, nPixelSize, nOrigHeight ); // byte pointers->x*pixelsize
   }
   void print() const
   { 
#ifdef _WIN32
     Filtering::print( LOG_DEBUG, "plane 0x%x (%ix%i:%i) t%i\n", pPixel, nWidth, nHeight, nPitch, GetCurrentThreadId() ); 
#else
     Filtering::print(LOG_DEBUG, "plane 0x%x (%ix%i:%i)\n", pPixel, nWidth, nHeight, nPitch);
#endif
   }

};

} // namespace Filtering

#endif

