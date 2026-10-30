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

public:

   Clip() : nFrames(-1), nWidth(-1), nHeight(-1), C(COLORSPACE_NONE) { }
   Clip(int nWidth, int nHeight, int nFrames, Colorspace colorspace) : nFrames(nFrames), nWidth(nWidth), nHeight(nHeight), C(colorspace) { }
   Clip(const Clip *clip) : nFrames(clip ? clip->frame_count() : -1), nWidth(clip ? clip->width() : -1), nHeight(clip ? clip->height() : -1), C(clip ? clip->colorspace() : COLORSPACE_NONE) { }
   int width() const { return nWidth; }
   int height() const { return nHeight; }
   int frame_count() const { return nFrames; }
   Colorspace colorspace() const { return C; }

   // not used: virtual Frame<Byte> get_frame(int n, PVideoFrame &current_frame, IScriptEnvironment *env) = 0;
   virtual Frame<const Byte> get_const_frame(int n, PVideoFrame &current_frame, IScriptEnvironment *env) = 0;
   virtual ::PClip get_avs_clip() = 0;

};

typedef std::shared_ptr<Clip> PClip;
typedef std::vector<PClip> ClipArray;

} // namespace Filtering

#endif
