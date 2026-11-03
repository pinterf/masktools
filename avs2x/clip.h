#ifndef __Common_Avs2x_Clip_H__
#define __Common_Avs2x_Clip_H__

#include "filter.h"
#include "../common/utils/utils.h"

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

namespace Filtering { namespace Avisynth2x {

/* plane conversion */
template<typename T> Plane<T> ConvertTo(const PVideoFrame& frame, int nPlane, int nPixelSize);

template<> inline Plane<uint8_t> ConvertTo<uint8_t>(const PVideoFrame &frame, int nPlane, int nPixelSize)
{
   return Plane<uint8_t>( frame->GetWritePtr( nPlane ), frame->GetPitch( nPlane ), frame->GetRowSize( nPlane ), frame->GetHeight( nPlane ), nPixelSize, frame->GetHeight(nPlane));
}
template<> inline Plane<const uint8_t> ConvertTo<const uint8_t>(const PVideoFrame &frame, int nPlane, int nPixelSize)
{
   return Plane<const uint8_t>( frame->GetReadPtr( nPlane ), frame->GetPitch( nPlane ), frame->GetRowSize( nPlane ), frame->GetHeight( nPlane ), nPixelSize, frame->GetHeight(nPlane));
}

/* the supported formats (planar Y(A), YUV(A), RGB(A)) */
static bool IsSupportedPixelType(int pixel_type)
{
   switch ( pixel_type )
   {
   case VideoInfo::CS_I420:
   case VideoInfo::CS_YV12:
   case VideoInfo::CS_YV16:
   case VideoInfo::CS_YV24:
   case VideoInfo::CS_Y8  :

   case VideoInfo::CS_YA8:
   case VideoInfo::CS_YA10:
   case VideoInfo::CS_YA12:
   case VideoInfo::CS_YA14:
   case VideoInfo::CS_YA16:
   case VideoInfo::CS_YAS:

   case VideoInfo::CS_YV411: // new from 2.2.1
   case VideoInfo::CS_YUV411P10:
   case VideoInfo::CS_YUV411P12:
   case VideoInfo::CS_YUV411P14:
   case VideoInfo::CS_YUV411P16:
   case VideoInfo::CS_YUV411PS:
   case VideoInfo::CS_YUVA411:
   case VideoInfo::CS_YUVA411P10:
   case VideoInfo::CS_YUVA411P12:
   case VideoInfo::CS_YUVA411P14:
   case VideoInfo::CS_YUVA411P16:
   case VideoInfo::CS_YUVA411PS:

   // new color spaces from v2.2.0
   case VideoInfo::CS_Y10:
   case VideoInfo::CS_YUV420P10:
   case VideoInfo::CS_YUV422P10:
   case VideoInfo::CS_YUV444P10:

   case VideoInfo::CS_Y12:
   case VideoInfo::CS_YUV420P12:
   case VideoInfo::CS_YUV422P12:
   case VideoInfo::CS_YUV444P12:

   case VideoInfo::CS_Y14:
   case VideoInfo::CS_YUV420P14:
   case VideoInfo::CS_YUV422P14:
   case VideoInfo::CS_YUV444P14:

   case VideoInfo::CS_Y16:
   case VideoInfo::CS_YUV420P16:
   case VideoInfo::CS_YUV422P16:
   case VideoInfo::CS_YUV444P16:

   case VideoInfo::CS_Y32:
   case VideoInfo::CS_YUV420PS:
   case VideoInfo::CS_YUV422PS:
   case VideoInfo::CS_YUV444PS:

   case VideoInfo::CS_RGBP:
   case VideoInfo::CS_RGBP10:
   case VideoInfo::CS_RGBP12:
   case VideoInfo::CS_RGBP14:
   case VideoInfo::CS_RGBP16:
   case VideoInfo::CS_RGBPS:

     // alpha support from v2.2.7
   case VideoInfo::CS_YUVA420:
   case VideoInfo::CS_YUVA422:
   case VideoInfo::CS_YUVA444:

   case VideoInfo::CS_YUVA420P10:
   case VideoInfo::CS_YUVA422P10:
   case VideoInfo::CS_YUVA444P10:

   case VideoInfo::CS_YUVA420P12:
   case VideoInfo::CS_YUVA422P12:
   case VideoInfo::CS_YUVA444P12:

   case VideoInfo::CS_YUVA420P14:
   case VideoInfo::CS_YUVA422P14:
   case VideoInfo::CS_YUVA444P14:

   case VideoInfo::CS_YUVA420P16:
   case VideoInfo::CS_YUVA422P16:
   case VideoInfo::CS_YUVA444P16:

   case VideoInfo::CS_YUVA420PS:
   case VideoInfo::CS_YUVA422PS:
   case VideoInfo::CS_YUVA444PS:

   case VideoInfo::CS_RGBAP:
   case VideoInfo::CS_RGBAP10:
   case VideoInfo::CS_RGBAP12:
   case VideoInfo::CS_RGBAP14:
   case VideoInfo::CS_RGBAP16:
   case VideoInfo::CS_RGBAPS:


     return true;
   default: return false;
   }
}

static const int PlaneOrder[] = { PLANAR_Y, PLANAR_U, PLANAR_V, PLANAR_A };
static const int PlaneOrderRGB[] = { PLANAR_R, PLANAR_G, PLANAR_B, PLANAR_A };

/* clip class, adapted to avs25 */
class Clip : public Filtering::Clip {

    ::PClip pclip;

public:

    Clip(const ::PClip &pclip) : pclip(pclip)
    {
        VideoInfo vi = pclip->GetVideoInfo();
        nWidth = vi.width;
        nHeight = vi.height;
        nFrames = vi.num_frames;
        const bool supported = IsSupportedPixelType(vi.pixel_type);
        nPlanes = supported ? vi.NumComponents() : 0;
        nBitDepth = supported ? vi.BitsPerComponent() : 0;
        isRGB = supported && (vi.IsPlanarRGB() || vi.IsPlanarRGBA());
        for (int i = 0; i < nPlanes; i++) {
            const int p = plane_index(i);
            // Y8-style formats have no chroma planes: GetPlane*Subsampling would throw for U/V there
            const bool chroma = !isRGB && (p == 1 || p == 2);
            width_ratios[p] = chroma ? 1 << vi.GetPlaneWidthSubsampling(PlaneOrder[p]) : 1;
            height_ratios[p] = chroma ? 1 << vi.GetPlaneHeightSubsampling(PlaneOrder[p]) : 1;
        }
    }
    
    virtual ~Clip()
    {
    }

    template<typename T> Frame<T> ConvertTo(const PVideoFrame& frame)
    {
        Plane<T> planes[4];
        {
            int nPixelSize = nBitDepth == 8 ? 1 : ((nBitDepth <= 16) ? 2 : 4); // uint8_t, uint16_t, float
            for (int i = 0; i < nPlanes; i++) {
                const int p = plane_index(i);
                planes[p] = Avisynth2x::ConvertTo<T>(frame, isRGB ? PlaneOrderRGB[p] : PlaneOrder[p], nPixelSize);
            }
        }
        return Frame<T>(planes, nPlanes);
    }



   virtual Frame<const uint8_t> get_const_frame(int n, PVideoFrame &frame_out, IScriptEnvironment *env)
   {
     // PF 170211: Issue: volatile pointers
     // return the frame, or else plane pointers stored in ConvertTo are not valid anymore
     // after function exit
     frame_out = pclip->GetFrame(clip<int>(n, 0, nFrames - 1), env);
     return ConvertTo<const uint8_t>(frame_out);
   }

   virtual ::PClip get_avs_clip()
   {
     return pclip;
   }

};

} } // namespace Avisynth2x, Common

#endif
