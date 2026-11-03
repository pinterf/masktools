#ifndef __Common_Clip_H__
#define __Common_Clip_H__

#include "../frame/frame.h"
#include <memory>

#ifdef _WIN32
#include <windows.h>
#pragma warning( push )
#pragma warning(disable:4100) // disable possible loss of data conversion
#include <avisynth.h>
#pragma warning( pop )
#else
#include <avisynth.h>
#endif
#include <avs/config.h>
namespace Filtering { 

class Clip {

protected:

   int nFrames;
   int nWidth, nHeight;
   int nPlanes = 0; // 0: unsupported format
   int nBitDepth = 0; // 8, 10, 12, 14, 16, 32 (float)
   bool isRGB = false; // planar RGB(A)
   // plane size ratios to the luma plane (chroma subsampling: 1, 2 or 4); 0 for the planes the format lacks
   int width_ratios[4] = { 0, 0, 0, 0 };
   int height_ratios[4] = { 0, 0, 0, 0 };

public:

   Clip() : nFrames(-1), nWidth(-1), nHeight(-1) { }
   Clip(const Clip *clip) : nFrames(clip ? clip->frame_count() : -1), nWidth(clip ? clip->width() : -1), nHeight(clip ? clip->height() : -1)
   {
     nPlanes = clip ? clip->plane_count() : 0;
     nBitDepth = clip ? clip->bit_depth() : 0;
     isRGB = clip ? clip->is_rgb() : false;
     for (int i = 0; clip && i < 4; i++) {
       width_ratios[i] = clip->width_ratio(i);
       height_ratios[i] = clip->height_ratio(i);
     }
   }
   int width() const { return nWidth; }
   int height() const { return nHeight; }
   int frame_count() const { return nFrames; }
   int plane_count() const { return nPlanes; }
   int plane_index(int ordinal) const { return Filtering::plane_index(nPlanes, ordinal); }
   bool has_plane(int slot) const { return Filtering::has_plane(nPlanes, slot); }
   int bit_depth() const { return nBitDepth; }
   bool is_rgb() const { return isRGB; }
   int width_ratio(int nPlane) const { return width_ratios[nPlane]; }
   int height_ratio(int nPlane) const { return height_ratios[nPlane]; }
   const int *width_ratios_ptr() const { return width_ratios; }
   const int *height_ratios_ptr() const { return height_ratios; }

   // not used: virtual Frame<uint8_t> get_frame(int n, PVideoFrame &current_frame, IScriptEnvironment *env) = 0;
   virtual Frame<const uint8_t> get_const_frame(int n, PVideoFrame &current_frame, IScriptEnvironment *env) = 0;
   virtual ::PClip get_avs_clip() = 0;

};

typedef std::shared_ptr<Clip> PClip;
typedef std::vector<PClip> ClipArray;

} // namespace Filtering

#endif
