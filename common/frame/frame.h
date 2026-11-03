#ifndef __Common_Frame_H__
#define __Common_Frame_H__

#include "../plane/plane.h"

namespace Filtering {

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

   Frame(const Plane<T> planes[4], int nPlanes) : nPlanes(nPlanes)
   {
       assert(nPlanes == 1 || nPlanes == 3 || nPlanes == 4);
       for (int i = 0; i < nPlanes; i++) {
           this->planes[i] = planes[i];
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
           planes[i] = Plane<T>(frame.plane(i));
       }
   }

   Plane<T> plane(int nPlane) const {
       assert(nPlane < nPlanes);
       return planes[nPlane];
   }

   // width_ratios / height_ratios: the clip's plane size ratios (Clip::width_ratio)
   Frame<T> offset(int x, int y, int w, int h, const int width_ratios[4], const int height_ratios[4]) const
   {
       if (nPlanes > 1)
       {
           w = ((x + w) / width_ratios[2]) * width_ratios[2];
           h = ((y + h) / height_ratios[2]) * height_ratios[2];
           x = (x / width_ratios[2]) * width_ratios[2];
           y = (y / height_ratios[2]) * height_ratios[2];
           w -= x;
           h -= y;
           if (nPlanes == 3) {
             return Frame<T>(planes[0].offset(x, y, w, h),
               planes[1].offset(x / width_ratios[1], y / height_ratios[1], w / width_ratios[1], h / height_ratios[1]),
               planes[2].offset(x / width_ratios[2], y / height_ratios[2], w / width_ratios[2], h / height_ratios[2]));
           }
           else {
             return Frame<T>(planes[0].offset(x, y, w, h),
               planes[1].offset(x / width_ratios[1], y / height_ratios[1], w / width_ratios[1], h / height_ratios[1]),
               planes[2].offset(x / width_ratios[2], y / height_ratios[2], w / width_ratios[2], h / height_ratios[2]),
               planes[3].offset(x, y, w, h)); // alpha
           }
       } else {
           return Frame<T>(planes[0].offset(x, y, w, h));
       }
   }

   Frame<T> select(int paddx, int paddy) const
   {
     if (nPlanes == 4)
       return Frame<T>(planes[0].offset(paddx, paddy, width(0) - 2 * paddx, height(0) - 2 * paddy),
         planes[1].offset(paddx, paddy, width(1) - 2 * paddx, height(1) - 2 * paddy),
         planes[2].offset(paddx, paddy, width(2) - 2 * paddx, height(2) - 2 * paddy),
         planes[3].offset(paddx, paddy, width(3) - 2 * paddx, height(3) - 2 * paddy));
     else if(nPlanes > 1)
       return Frame<T>(planes[0].offset(paddx, paddy, width(0) - 2 * paddx, height(0) - 2 * paddy),
         planes[1].offset(paddx, paddy, width(1) - 2 * paddx, height(1) - 2 * paddy),
         planes[2].offset(paddx, paddy, width(2) - 2 * paddx, height(2) - 2 * paddy));
     else
       return Frame<T>(planes[0].offset(paddx, paddy, width(0) - 2 * paddx, height(0) - 2 * paddy));
   }

   int width(int nPlane) const { assert( nPlane < nPlanes ); return planes[nPlane].width(); }
   int height(int nPlane) const { assert( nPlane < nPlanes ); return planes[nPlane].height(); }
   int plane_count() const { return nPlanes; }
};

} // namespace Filtering

#endif
