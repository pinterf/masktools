#ifndef __Mt_Binarize_H__
#define __Mt_Binarize_H__

#include "../../common/base/filter.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Binarize {

typedef void(Processor)(uint8_t *pDst, ptrdiff_t nDstPitch, uint8_t nThreshold, int nWidth, int nHeight);
typedef void(Processor16)(uint8_t *pDst, ptrdiff_t nDstPitch, uint16_t nThreshold, int nWidth, int nHeight, int bits_per_pixel);
typedef void(Processor32)(uint8_t *pDst, ptrdiff_t nDstPitch, float nThreshold, int nWidth, int nHeight);

#define DEFINE_PROCESSOR(name) \
   extern Processor *binarize_##name##_c; \
   extern Processor *binarize_##name##_sse2; \
   extern Processor *binarize_##name##_avx2; \
   extern Processor *binarize_##name##_neon

DEFINE_PROCESSOR(upper);
DEFINE_PROCESSOR(lower);
DEFINE_PROCESSOR(0_x);
DEFINE_PROCESSOR(t_x);
DEFINE_PROCESSOR(x_0);
DEFINE_PROCESSOR(x_t);
DEFINE_PROCESSOR(t_0);
DEFINE_PROCESSOR(0_t);
DEFINE_PROCESSOR(x_255);
DEFINE_PROCESSOR(t_255);
DEFINE_PROCESSOR(255_x);
DEFINE_PROCESSOR(255_t);

#undef DEFINE_PROCESSOR

/* 16 bits */

#define DEFINE_PROCESSOR(name) \
extern Processor16 *binarize_##name##_native_c; \
extern Processor16 *binarize_##name##_native_sse2; \
extern Processor16 *binarize_##name##_native_avx2; \
extern Processor16 *binarize_##name##_native_neon;

DEFINE_PROCESSOR(upper);
DEFINE_PROCESSOR(lower);
DEFINE_PROCESSOR(0_x);
DEFINE_PROCESSOR(t_x);
DEFINE_PROCESSOR(x_0);
DEFINE_PROCESSOR(x_t);
DEFINE_PROCESSOR(t_0);
DEFINE_PROCESSOR(0_t);
DEFINE_PROCESSOR(x_255);
DEFINE_PROCESSOR(t_255);
DEFINE_PROCESSOR(255_x);
DEFINE_PROCESSOR(255_t);

#undef DEFINE_PROCESSOR

/* 32 bits */
#define DEFINE_PROCESSOR(name) \
   extern Processor32 *binarize32_##name##_c; \
   extern Processor32 *binarize32_##name##_sse2; \
   extern Processor32 *binarize32_##name##_avx2; \
   extern Processor32 *binarize32_##name##_neon

DEFINE_PROCESSOR(upper);
DEFINE_PROCESSOR(lower);
DEFINE_PROCESSOR(0_x);
DEFINE_PROCESSOR(t_x);
DEFINE_PROCESSOR(x_0);
DEFINE_PROCESSOR(x_t);
DEFINE_PROCESSOR(t_0);
DEFINE_PROCESSOR(0_t);
DEFINE_PROCESSOR(x_255);
DEFINE_PROCESSOR(t_255);
DEFINE_PROCESSOR(255_x);
DEFINE_PROCESSOR(255_t);

#undef DEFINE_PROCESSOR


class Binarize : public MaskTools::Filter
{
   ProcessorList<Processor> processors;
   ProcessorList<Processor16> processors16;
   ProcessorList<Processor32> processors32;

   int nThreshold;
   float nThreshold_f;

   int bits_per_pixel;
protected:
    virtual void process(int n, const Plane<uint8_t> &dst, int nPlane, const Frame<const uint8_t> frames[4], const Constraint constraints[4]) override
    {
        UNUSED(n); UNUSED(frames);
        if(bits_per_pixel == 8)
          processors.best_processor(constraints[nPlane])(dst.data(), dst.pitch(), (uint8_t)nThreshold, dst.width(), dst.height());
        else if(bits_per_pixel <= 16)
          processors16.best_processor(constraints[nPlane])(dst.data(), dst.pitch(),(uint16_t)nThreshold, dst.width(), dst.height(), bits_per_pixel);
        else
          processors32.best_processor(constraints[nPlane])(dst.data(), dst.pitch(), nThreshold_f, dst.width(), dst.height());
    }

   bool isMode(const char *mode) {
       return parameters["mode"].is_defined() && parameters["mode"].toString() == mode;
   }

public:
   Binarize(const Parameters &parameters, CpuFlags cpuFlags) : MaskTools::Filter(parameters, FilterProcessingType::INPLACE, (CpuFlags)cpuFlags)
   {
     bool isStacked = parameters["stacked"].toBool();
     bits_per_pixel = bit_depths[C];

     if (isStacked && bits_per_pixel != 8) {
       error = "Stacked specified for a non-8 bit clip";
       return;
     }

     if (isStacked)
       bits_per_pixel = 16;

     bool isFloat = bits_per_pixel == 32;
     
     if (parameters["threshold"].is_defined()) {
       nThreshold_f = (float)parameters["threshold"].toFloat();
       nThreshold = parameters["threshold"].toInt();
       String scalemode = parameters["paramscale"].toString();
       bool fullscale = planes_isRGB[C];
       if (!ScaleParam(scalemode, nThreshold_f, bits_per_pixel, nThreshold_f, nThreshold, fullscale, false, false))
       {
         error = "invalid parameter: paramscale. Use i8, i10, i12, i14, i16, f32 for scale or none/empty to disable scaling";
         return;
       }
     }
     else {
       if(isFloat)
         nThreshold_f = 0.5f; 
       else 
         nThreshold = (1 << (bits_per_pixel - 1)); // bit-depth adaptive default half value
     }

     if (bits_per_pixel == 8) {
#define SET_MODE(mode) \
      processors.push_back(Filtering::Processor<Processor>(binarize_##mode##_c, Constraint(CPU_NONE), 0)); \
      MT_X86(processors.push_back(Filtering::Processor<Processor>(binarize_##mode##_sse2, Constraint(CPU_SSE2), 1))); \
      MT_X86(processors.push_back(Filtering::Processor<Processor>(binarize_##mode##_avx2, Constraint(CPU_AVX2), 3))); \
      MT_NEON(processors.push_back(Filtering::Processor<Processor>(binarize_##mode##_neon, Constraint(CPU_NEON), 1)));

       if (isMode("0 x")) { SET_MODE(0_x); }
       else if (isMode("t x")) { SET_MODE(t_x); }
       else if (isMode("x 0")) { SET_MODE(x_0); }
       else if (isMode("x t")) { SET_MODE(x_t); }
       else if (isMode("t 0")) { SET_MODE(t_0); }
       else if (isMode("0 t")) { SET_MODE(0_t); }
       else if (isMode("x 255")) { SET_MODE(x_255); }
       else if (isMode("t 255")) { SET_MODE(t_255); }
       else if (isMode("255 x")) { SET_MODE(255_x); }
       else if (isMode("255 t")) { SET_MODE(255_t); }
       else if ((parameters["mode"].is_defined() && (parameters["mode"].toString() == "upper" || parameters["mode"].toString() == "0 255")) || (!parameters["mode"].is_defined() && parameters["upper"].toBool())) {
         SET_MODE(upper);
       }
       else {
         SET_MODE(lower);
       }
#undef SET_MODE
     
     }
     else if (bits_per_pixel <= 16) {

#define SET_MODE(mode) \
    processors16.push_back(Filtering::Processor<Processor16>(binarize_##mode##_native_c, Constraint(CPU_NONE), 0)); \
    MT_X86(processors16.push_back(Filtering::Processor<Processor16>(binarize_##mode##_native_sse2, Constraint(CPU_SSE2), 1))); \
    MT_X86(processors16.push_back(Filtering::Processor<Processor16>(binarize_##mode##_native_avx2, Constraint(CPU_AVX2), 3))); \
    MT_NEON(processors16.push_back(Filtering::Processor<Processor16>(binarize_##mode##_native_neon, Constraint(CPU_NEON), 1)));
       if (isMode("0 x")) { SET_MODE(0_x); }
       else if (isMode("t x")) { SET_MODE(t_x); }
       else if (isMode("x 0")) { SET_MODE(x_0); }
       else if (isMode("x t")) { SET_MODE(x_t); }
       else if (isMode("t 0")) { SET_MODE(t_0); }
       else if (isMode("0 t")) { SET_MODE(0_t); }
       else if (isMode("x 255")) { SET_MODE(x_255); }
       else if (isMode("t 255")) { SET_MODE(t_255); }
       else if (isMode("255 x")) { SET_MODE(255_x); }
       else if (isMode("255 t")) { SET_MODE(255_t); }
       else if ((parameters["mode"].is_defined() && (parameters["mode"].toString() == "upper" || parameters["mode"].toString() == "0 255")) || (!parameters["mode"].is_defined() && parameters["upper"].toBool())) {
         SET_MODE(upper);
       }
       else {
         SET_MODE(lower);
       }
#undef SET_MODE

     }
     else { // float
#define SET_MODE(mode) \
      processors32.push_back(Filtering::Processor<Processor32>(binarize32_##mode##_c, Constraint(CPU_NONE), 0)); \
      MT_X86(processors32.push_back(Filtering::Processor<Processor32>(binarize32_##mode##_sse2, Constraint(CPU_SSE2), 1))); \
      MT_X86(processors32.push_back(Filtering::Processor<Processor32>(binarize32_##mode##_avx2, Constraint(CPU_AVX2), 3))); \
      MT_NEON(processors32.push_back(Filtering::Processor<Processor32>(binarize32_##mode##_neon, Constraint(CPU_NEON), 1)));

       if (isMode("0 x")) { SET_MODE(0_x); }
       else if (isMode("t x")) { SET_MODE(t_x); }
       else if (isMode("x 0")) { SET_MODE(x_0); }
       else if (isMode("x t")) { SET_MODE(x_t); }
       else if (isMode("t 0")) { SET_MODE(t_0); }
       else if (isMode("0 t")) { SET_MODE(0_t); }
       else if (isMode("x 255")) { SET_MODE(x_255); }
       else if (isMode("t 255")) { SET_MODE(t_255); }
       else if (isMode("255 x")) { SET_MODE(255_x); }
       else if (isMode("255 t")) { SET_MODE(255_t); }
       else if ((parameters["mode"].is_defined() && (parameters["mode"].toString() == "upper" || parameters["mode"].toString() == "0 255")) || (!parameters["mode"].is_defined() && parameters["upper"].toBool())) {
         SET_MODE(upper);
       }
       else {
         SET_MODE(lower);
       }
#undef SET_MODE


     }
   }

   InputConfiguration &input_configuration() const override { return InPlaceOneFrame(); }

   static Signature filter_signature()
   {
      Signature signature = "mt_binarize";

      signature.add( Parameter( TYPE_CLIP, "", false) );
      signature.add( Parameter( 128.0f, "threshold", true) ); // bit depth dependent, see above.
      signature.add( Parameter( false, "upper", false));
      signature.add( Parameter( String("lower"), "mode", false) );

      add_defaults( signature );

      signature.add(Parameter(false, "stacked", false));
      return signature;
   }
};

} } } } // namespace Binarize, Filter, MaskTools, Filtering

#endif