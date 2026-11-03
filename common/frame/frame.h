#ifndef __Common_Frame_H__
#define __Common_Frame_H__

#include "../plane/plane.h"

namespace Filtering {

template<typename T>
class Frame {

   Plane<T> planes[4];
   Colorspace C;

public:

   Frame() {}
   Frame(const Plane<T> &plane, Colorspace C) : C(C)
   {
       planes[0] = plane;
   }

   Frame(const Plane<T> planes[4], Colorspace C) : C(C)
   {
       assert(plane_counts[C] == 1 || plane_counts[C] == 3 || plane_counts[C] == 4);
       for (int i = 0; i < plane_counts[C]; i++) {
           this->planes[i] = planes[i];
       }
   }

   Frame(const Plane<T> &y, const Plane<T> &u, const Plane<T> &v, Colorspace C) : C(C)
   {
       assert(plane_counts[C] == 3);
       planes[0] = y;
       planes[1] = u;
       planes[2] = v;
   }

   Frame(const Plane<T> &y, const Plane<T> &u, const Plane<T> &v, const Plane<T> &a, Colorspace C) : C(C)
   {
     assert(plane_counts[C] == 4);
     planes[0] = y;
     planes[1] = u;
     planes[2] = v;
     planes[3] = a;
   }

   template<class U>
   Frame(const Frame<U> &frame) : C(frame.colorspace())
   {
       for (int i = 0; i < plane_counts[C]; i++) {
           planes[i] = Plane<T>(frame.plane(i));
       }
   }

   Plane<T> plane(int nPlane) const {
       assert(nPlane < plane_counts[C]);
       return planes[nPlane];
   }

   // width_ratios / height_ratios: the clip's plane size ratios (Clip::width_ratio)
   Frame<T> offset(int x, int y, int w, int h, const int width_ratios[4], const int height_ratios[4]) const
   {
       if (plane_counts[C] > 1)
       {
           w = ((x + w) / width_ratios[2]) * width_ratios[2];
           h = ((y + h) / height_ratios[2]) * height_ratios[2];
           x = (x / width_ratios[2]) * width_ratios[2];
           y = (y / height_ratios[2]) * height_ratios[2];
           w -= x;
           h -= y;
           if (plane_counts[C] == 3) {
             return Frame<T>(planes[0].offset(x, y, w, h),
               planes[1].offset(x / width_ratios[1], y / height_ratios[1], w / width_ratios[1], h / height_ratios[1]),
               planes[2].offset(x / width_ratios[2], y / height_ratios[2], w / width_ratios[2], h / height_ratios[2]),
               C);
           }
           else {
             return Frame<T>(planes[0].offset(x, y, w, h),
               planes[1].offset(x / width_ratios[1], y / height_ratios[1], w / width_ratios[1], h / height_ratios[1]),
               planes[2].offset(x / width_ratios[2], y / height_ratios[2], w / width_ratios[2], h / height_ratios[2]),
               planes[3].offset(x, y, w, h), // alpha
               C);
           }
       } else {
           return Frame<T>(planes[0].offset(x, y, w, h), 
             C);
       }
   }

   Frame<T> select(int paddx, int paddy) const
   {
     if (plane_counts[C] == 4)
       return Frame<T>(planes[0].offset(paddx, paddy, width(0) - 2 * paddx, height(0) - 2 * paddy),
         planes[1].offset(paddx, paddy, width(1) - 2 * paddx, height(1) - 2 * paddy),
         planes[2].offset(paddx, paddy, width(2) - 2 * paddx, height(2) - 2 * paddy),
         planes[3].offset(paddx, paddy, width(3) - 2 * paddx, height(3) - 2 * paddy),
         C);
     else if(plane_counts[C] > 1)
       return Frame<T>(planes[0].offset(paddx, paddy, width(0) - 2 * paddx, height(0) - 2 * paddy),
         planes[1].offset(paddx, paddy, width(1) - 2 * paddx, height(1) - 2 * paddy),
         planes[2].offset(paddx, paddy, width(2) - 2 * paddx, height(2) - 2 * paddy),
         C);
     else
       return Frame<T>(planes[0].offset(paddx, paddy, width(0) - 2 * paddx, height(0) - 2 * paddy),
         C);
   }

   int width(int nPlane) const { assert( nPlane < plane_counts[C] ); return planes[nPlane].width(); }
   int height(int nPlane) const { assert( nPlane < plane_counts[C] ); return planes[nPlane].height(); }
   Colorspace colorspace() const { return C; }
};

} // namespace Filtering

#endif
