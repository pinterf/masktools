#ifndef __Common_Frame_H__
#define __Common_Frame_H__

#include "../plane/plane.h"

namespace Filtering {

// Internal slots stay Y/U/V/A (R/G/B/A); YA uses slots 0 and 3, not 0 and 1.
inline int plane_index(int plane_count, int ordinal)
{
    return plane_count == 2 && ordinal == 1 ? 3 : ordinal;
}

inline bool has_plane(int plane_count, int slot)
{
    return slot >= 0 && slot < 4 &&
        (plane_count == 2 ? slot == 0 || slot == 3 : slot < plane_count);
}

template<typename T>
class Frame {

   Plane<T> planes[4];
   int nPlanes = 0;

public:

   Frame() {}
   explicit Frame(const Plane<T> &plane) : nPlanes(1)
   {
       planes[0] = plane;
   }

   // The array is indexed by internal slot, including slot 3 for YA alpha.
   Frame(const Plane<T> planes[4], int nPlanes) : nPlanes(nPlanes)
   {
       assert(nPlanes >= 1 && nPlanes <= 4);
       for (int i = 0; i < nPlanes; i++) {
           const int p = plane_index(i);
           this->planes[p] = planes[p];
       }
   }

   Frame(const Plane<T> &y, const Plane<T> &u, const Plane<T> &v) : nPlanes(3)
   {
       planes[0] = y;
       planes[1] = u;
       planes[2] = v;
   }

   Frame(const Plane<T> &y, const Plane<T> &u, const Plane<T> &v, const Plane<T> &a) : nPlanes(4)
   {
     planes[0] = y;
     planes[1] = u;
     planes[2] = v;
     planes[3] = a;
   }

   template<class U>
   Frame(const Frame<U> &frame) : nPlanes(frame.plane_count())
   {
       for (int i = 0; i < nPlanes; i++) {
           const int p = plane_index(i);
           planes[p] = Plane<T>(frame.plane(p));
       }
   }

   Plane<T> plane(int nPlane) const {
       assert(has_plane(nPlanes, nPlane));
       return planes[nPlane];
   }

   // width_ratios / height_ratios: the clip's plane size ratios (Clip::width_ratio)
   Frame<T> offset(int x, int y, int w, int h, const int width_ratios[4], const int height_ratios[4]) const
   {
       // Y and YA have no chroma grid; their absent U/V slots have zero ratios and must not be used.
       const int rw = nPlanes >= 3 ? width_ratios[2] : 1;
       const int rh = nPlanes >= 3 ? height_ratios[2] : 1;
       w = ((x + w) / rw) * rw;
       h = ((y + h) / rh) * rh;
       x = (x / rw) * rw;
       y = (y / rh) * rh;
       w -= x;
       h -= y;
       Plane<T> result[4];
       for (int i = 0; i < nPlanes; i++) {
           const int p = plane_index(i);
           result[p] = planes[p].offset(x / width_ratios[p], y / height_ratios[p],
               w / width_ratios[p], h / height_ratios[p]);
       }
       return Frame<T>(result, nPlanes);
   }

   Frame<T> select(int paddx, int paddy) const
   {
     Plane<T> result[4];
     for (int i = 0; i < nPlanes; i++) {
       const int p = plane_index(i);
       result[p] = planes[p].offset(paddx, paddy, width(p) - 2 * paddx, height(p) - 2 * paddy);
     }
     return Frame<T>(result, nPlanes);
   }

   int width(int nPlane) const { return plane(nPlane).width(); }
   int height(int nPlane) const { return plane(nPlane).height(); }
   int plane_count() const { return nPlanes; }
   int plane_index(int ordinal) const { return Filtering::plane_index(nPlanes, ordinal); }
};

} // namespace Filtering

#endif
