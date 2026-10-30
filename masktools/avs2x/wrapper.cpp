#include "../filters/binarize/binarize.h"
#include "../filters/invert/invert.h"
#include "../filters/logic/logic.h"
#include "../filters/merge/merge.h"
#include "../filters/convolution/convolution.h"
#include "../filters/lut/lut/lut.h"
#include "../filters/lut/luts/luts.h"
#include "../filters/lut/lutf/lutf.h"
#include "../filters/lut/lutxy/lutxy.h"
#include "../filters/lut/lutxyz/lutxyz.h"
#include "../filters/lut/lutxyza/lutxyza.h"
#include "../filters/lut/lutsx/lutsx.h"
#include "../filters/lut/lutspa/lutspa.h"
#include "../filters/mask/edge/edgemask.h"
#include "../filters/mask/motion/motionmask.h"
#include "../filters/mask/hysteresis/hysteresis.h"
#include "../filters/morphologic/expand/expand.h"
#include "../filters/morphologic/inpand/inpand.h"
#include "../filters/morphologic/inflate/inflate.h"
#include "../filters/morphologic/deflate/deflate.h"
#include "../filters/blur/mappedblur.h"
#include "../filters/gradient/gradient.h"
//
#include "../filters/support/adddiff/adddiff.h"
#include "../filters/support/makediff/makediff.h"
#include "../filters/support/average/average.h"
#include "../filters/support/clamp/clamp.h"

#include "../helpers/avs2x/helpers_avs2x.h"
#include "../../avs2x/filter.h" // Filtering::Avisynth2x namespace

using namespace Filtering;
using namespace Filtering::MaskTools::Filters;

const AVS_Linkage *AVS_linkage = nullptr;

extern "C" __declspec(dllexport) const char* __stdcall AvisynthPluginInit3(IScriptEnvironment* env, const AVS_Linkage* const vectors) {
    AVS_linkage = vectors;

   Avisynth2x::Filter<Invert::Invert>::create( env ); // 8-32
   Avisynth2x::Filter<Binarize::Binarize>::create( env ); // 8-32, stacked
   Avisynth2x::Filter<Morphologic::Inflate::Inflate>::create( env ); // 8-32, stacked
   Avisynth2x::Filter<Morphologic::Deflate::Deflate>::create( env ); // 8-32, stacked
   Avisynth2x::Filter<Morphologic::Inpand::Inpand>::create( env ); // 8-32, stacked
   Avisynth2x::Filter<Morphologic::Expand::Expand>::create( env ); // 8-32, stacked
   Avisynth2x::Filter<Lut::Single::Lutx>::create( env ); // 8-32, stacked, lut for bits<=16, realtime otherwise
   Avisynth2x::Filter<Lut::Dual::Lutxy>::create( env ); // 8-32, lut for bits<=12, realtime otherwise, may ask realtime=false for 14,16
   Avisynth2x::Filter<Lut::Trial::Lutxyz>::create( env ); // 8-32, lut only for 8 bits, 10+ bits realtime
   Avisynth2x::Filter<Lut::Quad::Lutxyza>::create(env); // 8-32, four clips, realtime only, realtime=false accepted on 8 bit (4GBytes LUT!)
   Avisynth2x::Filter<Lut::Spatial::Luts>::create( env ); // 8-32 lut for bits<=12, realtime otherwise, may ask realtime=false for 14,16
   Avisynth2x::Filter<Lut::Frame::Lutf>::create( env ); // 8-32, lut for bits<=12, realtime otherwise, may ask realtime=false for 14,16
   Avisynth2x::Filter<Lut::SpatialExtended::Lutsx>::create( env ); // 8-32, lut only for 8 bits, 10+ bits realtime
   Avisynth2x::Filter<Lut::Coordinate::Lutspa>::create( env ); // 8-32
   Avisynth2x::Filter<Merge::Merge>::create( env ); // 8-32, stacked
   Avisynth2x::Filter<Logic::Logic>::create( env ); // 8-32, stacked
   Avisynth2x::Filter<Convolution::Convolution>::create( env ); // 8-32
   Avisynth2x::Filter<Blur::MappedBlur>::create( env ); // 8-32
   Avisynth2x::Filter<Gradient::Gradient>::create( env ); // 8-32 bit only
   Avisynth2x::Filter<Support::MakeDiff::MakeDiff>::create( env ); // 8-32, stacked
   Avisynth2x::Filter<Support::Average::Average>::create( env ); // 8-32, stacked
   Avisynth2x::Filter<Support::AddDiff::AddDiff>::create( env ); // 8-32, stacked
   Avisynth2x::Filter<Support::Clamp::Clamp>::create( env ); // 8-32, stacked
   Avisynth2x::Filter<Mask::Motion::MotionMask>::create( env ); // 8-32
   Avisynth2x::Filter<Mask::Edge::EdgeMask>::create( env ); // 8-32
   Avisynth2x::Filter<Mask::Hysteresis::Hysteresis>::create( env ); // 8-32
   MaskTools::Avs2x::Helpers::DeclareHelpers(env);

   return("MaskTools: a set of tools to work with masks");
}
