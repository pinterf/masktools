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
   Colorspace C;
   // plane size ratios to the luma plane (chroma subsampling: 1, 2 or 4); 0 for the planes the format lacks
   int width_ratios[4] = { 0, 0, 0, 0 };
   int height_ratios[4] = { 0, 0, 0, 0 };

public:

   Clip() : nFrames(-1), nWidth(-1), nHeight(-1), C(COLORSPACE_NONE) { }
   Clip(int nWidth, int nHeight, int nFrames, Colorspace colorspace) : nFrames(nFrames), nWidth(nWidth), nHeight(nHeight), C(colorspace) { }
   Clip(const Clip *clip) : nFrames(clip ? clip->frame_count() : -1), nWidth(clip ? clip->width() : -1), nHeight(clip ? clip->height() : -1), C(clip ? clip->colorspace() : COLORSPACE_NONE)
   {
     for (int i = 0; clip && i < 4; i++) {
       width_ratios[i] = clip->width_ratio(i);
       height_ratios[i] = clip->height_ratio(i);
     }
   }
   int width() const { return nWidth; }
   int height() const { return nHeight; }
   int frame_count() const { return nFrames; }
   Colorspace colorspace() const { return C; }
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
