#ifndef __Mt_Logic_H__
#define __Mt_Logic_H__

#include "../../common/base/filter.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Logic {

typedef void(Processor)(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch, int nWidth, int nHeight, uint8_t nThresholdDestination, uint8_t nThresholdSource);
typedef void(Processor16)(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int nWidth, int nHeight, int bits_per_pixel, uint16_t nThresholdDestination, uint16_t nThresholdSource);
typedef void(Processor32)(float *pDst, ptrdiff_t nDstPitch, const float *pSrc1, ptrdiff_t nSrc1Pitch, int nWidth, int nHeight, float nThresholdDestination, float nThresholdSource);

#define DEFINE_PROCESSOR(name) \
   extern Processor *name##_c; \
   extern Processor *name##_sse2; \
   extern Processor *name##_avx2; \
   extern Processor *name##_neon;

DEFINE_PROCESSOR(and);
DEFINE_PROCESSOR(or);
DEFINE_PROCESSOR(andn);
DEFINE_PROCESSOR(xor);

#define DEFINE_TRIPLE(mode) \
DEFINE_PROCESSOR(mode); \
DEFINE_PROCESSOR(mode##add); \
DEFINE_PROCESSOR(mode##sub)

#define DEFINE_NINE(mode) \
DEFINE_TRIPLE(mode); \
DEFINE_TRIPLE(add##mode); \
DEFINE_TRIPLE(sub##mode)

DEFINE_NINE(min);
DEFINE_NINE(max);

#undef DEFINE_TRIPLE
#undef DEFINE_NINE
#undef DEFINE_PROCESSOR

/* 16 bit */
#define DEFINE_PROCESSOR(name) \
   extern Processor16 *name##_native_c; \
   extern Processor16 *name##_native_sse2; \
   extern Processor16 *name##_native_avx2; \
   extern Processor16 *name##_native_neon;

DEFINE_PROCESSOR(and16);
DEFINE_PROCESSOR(or16 );
DEFINE_PROCESSOR(andn16);
DEFINE_PROCESSOR(xor16);

#define DEFINE_TRIPLE(mode) \
DEFINE_PROCESSOR(mode); \
DEFINE_PROCESSOR(mode##add); \
DEFINE_PROCESSOR(mode##sub)

#define DEFINE_NINE(mode) \
DEFINE_TRIPLE(mode); \
DEFINE_TRIPLE(add##mode); \
DEFINE_TRIPLE(sub##mode)

DEFINE_NINE(min);
DEFINE_NINE(max);

#undef DEFINE_TRIPLE
#undef DEFINE_NINE
#undef DEFINE_PROCESSOR

/* 32 bit */
#define DEFINE_PROCESSOR(name) \
   extern Processor32 *name##_32_c; \
   extern Processor32 *name##_32_sse2; \
   extern Processor32 *name##_32_avx; \
   extern Processor32 *name##_32_neon;

DEFINE_PROCESSOR(and);
DEFINE_PROCESSOR(or );
DEFINE_PROCESSOR(andn);
DEFINE_PROCESSOR(xor);

#define DEFINE_TRIPLE(mode) \
DEFINE_PROCESSOR(mode); \
DEFINE_PROCESSOR(mode##add); \
DEFINE_PROCESSOR(mode##sub)

#define DEFINE_NINE(mode) \
DEFINE_TRIPLE(mode); \
DEFINE_TRIPLE(add##mode); \
DEFINE_TRIPLE(sub##mode)

DEFINE_NINE(min);
DEFINE_NINE(max);

#undef DEFINE_TRIPLE
#undef DEFINE_NINE
#undef DEFINE_PROCESSOR


class Logic : public MaskTools::Filter
{

  ProcessorList<Processor> processors;
  ProcessorList<Processor16> processors16;
  ProcessorList<Processor32> processors32;
  int nThresholdDestination, nThresholdSource;
  float nThresholdDestination_f, nThresholdSource_f;

  int bits_per_pixel;
  int isStacked;

protected:

  virtual void process(int n, const Plane<uint8_t> &dst, int nPlane, const Frame<const uint8_t> frames[4], const Constraint constraints[4]) override
  {
    UNUSED(n);
    if (bits_per_pixel == 8) {
      processors.best_processor(constraints[nPlane])(dst.data(), dst.pitch(),
        frames[0].plane(nPlane).data(), frames[0].plane(nPlane).pitch(),
        dst.width(), dst.height(), (uint8_t)nThresholdDestination, (uint8_t)nThresholdSource);
    }
    else if (bits_per_pixel <= 16) {
      processors16.best_processor(constraints[nPlane])(dst.data(), dst.pitch(),
        frames[0].plane(nPlane).data(), frames[0].plane(nPlane).pitch(),
        dst.width(), dst.height(), bits_per_pixel, (uint16_t)nThresholdDestination, (uint16_t)nThresholdSource);
    }
    else {
      processors32.best_processor(constraints[nPlane])((float *)dst.data(), dst.pitch(),
        (float *)frames[0].plane(nPlane).data(), frames[0].plane(nPlane).pitch(),
        dst.width(), dst.height(), nThresholdDestination_f, nThresholdSource_f);
    }
  }

public:
  Logic(const Parameters &parameters, CpuFlags cpuFlags) : MaskTools::Filter(parameters, FilterProcessingType::INPLACE, (CpuFlags)cpuFlags)
  {
    isStacked = parameters["stacked"].toBool();
    bits_per_pixel = bit_depths[C];

    if (isStacked && bits_per_pixel != 8) {
      error = "Stacked specified for a non-8 bit clip";
      return;
    }

    if (isStacked)
      bits_per_pixel = 16;

    bool fullscale = planes_isRGB[C];
    String scalemode = parameters["paramscale"].toString();
    const char *errortxt = "invalid parameter: paramscale. Use i8, i10, i12, i14, i16, f32 for scale or none/empty to disable scaling";
    
    int nTh1, nTh2;
    float nTh1_f, nTh2_f;

    // defaults
    nTh1 = nTh2 = 0;
    nTh1_f = nTh2_f = 0.0f;
    // threshold 1
    if (parameters["th1"].is_defined()) {
      nTh1_f = (float)parameters["th1"].toFloat();
      if (!ScaleParam(scalemode, nTh1_f, bits_per_pixel, nTh1_f, nTh1, fullscale, true, false)) // allow negatives
      {
        error = errortxt;
        return;
      }
    }
    // threshold 2
    if (parameters["th2"].is_defined()) {
      nTh2_f = (float)parameters["th2"].toFloat();
      if (!ScaleParam(scalemode, nTh2_f, bits_per_pixel, nTh2_f, nTh2, fullscale, true, false)) // allow negatives
      {
        error = errortxt;
        return;
      }
    }

    if (bits_per_pixel == 8) {
#define SET_MODE(mode) \
   do { \
      processors.push_back( Filtering::Processor<Processor>( mode##_c, Constraint(CPU_NONE), 0 ) ); \
      MT_X86(processors.push_back( Filtering::Processor<Processor>( mode##_sse2, Constraint(CPU_SSE2), 1 ) )); \
      MT_X86(processors.push_back( Filtering::Processor<Processor>( mode##_avx2, Constraint(CPU_AVX2), 3 ) )); \
      MT_NEON(processors.push_back( Filtering::Processor<Processor>( mode##_neon, Constraint(CPU_NEON), 1 ) )); \
   } while(0)
      
      if (parameters["mode"].toString() == "and")
        SET_MODE(and);
      else if (parameters["mode"].toString() == "or")
        SET_MODE(or );
      else if (parameters["mode"].toString() == "xor")
        SET_MODE(xor);
      else if (parameters["mode"].toString() == "andn")
        SET_MODE(andn);
      else
      {
        bool isDstSub = nTh1 < 0;
        bool isDstAdd = nTh1 > 0;
        bool isSrcSub = nTh2 < 0;
        bool isSrcAdd = nTh2 > 0;

        nThresholdDestination = convert<uint8_t, int>(abs<int>(nTh1));
        nThresholdSource = convert<uint8_t, int>(abs<int>(nTh2));

        if (parameters["mode"].toString() == "min")
        {
          if (isDstAdd && isSrcAdd) SET_MODE(addminadd);
          else if (isDstAdd && isSrcSub) SET_MODE(addminsub);
          else if (isDstSub && isSrcAdd) SET_MODE(subminadd);
          else if (isDstSub && isSrcSub) SET_MODE(subminsub);
          else if (isDstAdd) SET_MODE(addmin);
          else if (isSrcAdd) SET_MODE(minadd);
          else if (isDstSub) SET_MODE(submin);
          else if (isSrcSub) SET_MODE(minsub);
          else SET_MODE(min);
        }
        else if (parameters["mode"].toString() == "max")
        {
          if (isDstAdd && isSrcAdd) SET_MODE(addmaxadd);
          else if (isDstAdd && isSrcSub) SET_MODE(addmaxsub);
          else if (isDstSub && isSrcAdd) SET_MODE(submaxadd);
          else if (isDstSub && isSrcSub) SET_MODE(submaxsub);
          else if (isDstAdd) SET_MODE(addmax);
          else if (isSrcAdd) SET_MODE(maxadd);
          else if (isDstSub) SET_MODE(submax);
          else if (isSrcSub) SET_MODE(maxsub);
          else SET_MODE(max);
        }
        else
          error = "\"mode\" must be either \"and\", \"or\", \"xor\", \"andn\", \"min\" or \"max\"";
      }
#undef SET_MODE
    }
    else if (bits_per_pixel <= 16) {
      /* 16 bit */
#define SET_MODE(mode, needSSE4) \
    do { \
        processors16.push_back( Filtering::Processor<Processor16>( mode##_native_c, Constraint(CPU_NONE), 0 ) ); \
        MT_X86(processors16.push_back( Filtering::Processor<Processor16>( mode##_native_sse2, Constraint((needSSE4) ? CPU_SSE4_1 : CPU_SSE2), 1 ) )); \
        MT_X86(processors16.push_back( Filtering::Processor<Processor16>( mode##_native_avx2, Constraint(CPU_AVX2), 3 ) )); \
        MT_NEON(processors16.push_back( Filtering::Processor<Processor16>( mode##_native_neon, Constraint(CPU_NEON), 1 ) )); \
    } while(0)

      // modes containing min, max, add require SSE4

      if (parameters["mode"].toString() == "and") {
        SET_MODE(and16, false);
      }
      else if (parameters["mode"].toString() == "or") {
        SET_MODE(or16, false);
      }
      else if (parameters["mode"].toString() == "xor") {
        SET_MODE(xor16, false);
      }
      else if (parameters["mode"].toString() == "andn") {
        SET_MODE(andn16, false);
      }
      else {
        bool isDstSub = nTh1 < 0;
        bool isDstAdd = nTh1 > 0;
        bool isSrcSub = nTh2 < 0;
        bool isSrcAdd = nTh2 > 0;

        int max_pixel_value = (1 << bits_per_pixel) - 1;
        nThresholdDestination = min(convert<uint16_t, int>(abs<int>(nTh1)), (uint16_t)max_pixel_value);
        nThresholdSource = min(convert<uint16_t, int>(abs<int>(nTh2)), (uint16_t)max_pixel_value);

        if (parameters["mode"].toString() == "min")
        {
          if (isDstAdd && isSrcAdd) SET_MODE(addminadd, true);
          else if (isDstAdd && isSrcSub) SET_MODE(addminsub, true);
          else if (isDstSub && isSrcAdd) SET_MODE(subminadd, true);
          else if (isDstSub && isSrcSub) SET_MODE(subminsub, true);
          else if (isDstAdd) SET_MODE(addmin, true);
          else if (isSrcAdd) SET_MODE(minadd, true);
          else if (isDstSub) SET_MODE(submin, true);
          else if (isSrcSub) SET_MODE(minsub, true);
          else SET_MODE(min, true);
        }
        else if (parameters["mode"].toString() == "max")
        {
          if (isDstAdd && isSrcAdd) SET_MODE(addmaxadd, true);
          else if (isDstAdd && isSrcSub) SET_MODE(addmaxsub, true);
          else if (isDstSub && isSrcAdd) SET_MODE(submaxadd, true);
          else if (isDstSub && isSrcSub) SET_MODE(submaxsub, true);
          else if (isDstAdd) SET_MODE(addmax, true);
          else if (isSrcAdd) SET_MODE(maxadd, true);
          else if (isDstSub) SET_MODE(submax, true);
          else if (isSrcSub) SET_MODE(maxsub, true);
          else SET_MODE(max, true);
        }
        else
          error = "\"mode\" must be either \"and\", \"or\", \"xor\", \"andn\", \"min\" or \"max\"";
      }
#undef SET_MODE

    }
    else { // float
#define SET_MODE(mode) \
   do { \
      processors32.push_back( Filtering::Processor<Processor32>( mode##_32_c, Constraint(CPU_NONE), 0 ) ); \
      MT_X86(processors32.push_back( Filtering::Processor<Processor32>( mode##_32_sse2, Constraint(CPU_SSE2), 1 ) )); \
      MT_X86(processors32.push_back( Filtering::Processor<Processor32>( mode##_32_avx, Constraint(CPU_AVX), 3 ) )); \
      MT_NEON(processors32.push_back( Filtering::Processor<Processor32>( mode##_32_neon, Constraint(CPU_NEON), 1 ) )); \
   } while(0)

      if (parameters["mode"].toString() == "and")
        SET_MODE(and);
      else if (parameters["mode"].toString() == "or")
        SET_MODE(or );
      else if (parameters["mode"].toString() == "xor")
        SET_MODE(xor);
      else if (parameters["mode"].toString() == "andn")
        SET_MODE(andn);
      else
      {
        bool isDstSub = nTh1_f < 0;
        bool isDstAdd = nTh1_f > 0;
        bool isSrcSub = nTh2_f < 0;
        bool isSrcAdd = nTh2_f > 0;

        nThresholdDestination_f = abs<float>(nTh1_f);
        nThresholdSource_f = abs<float>(nTh2_f);

        if (parameters["mode"].toString() == "min")
        {
          if (isDstAdd && isSrcAdd) SET_MODE(addminadd);
          else if (isDstAdd && isSrcSub) SET_MODE(addminsub);
          else if (isDstSub && isSrcAdd) SET_MODE(subminadd);
          else if (isDstSub && isSrcSub) SET_MODE(subminsub);
          else if (isDstAdd) SET_MODE(addmin);
          else if (isSrcAdd) SET_MODE(minadd);
          else if (isDstSub) SET_MODE(submin);
          else if (isSrcSub) SET_MODE(minsub);
          else SET_MODE(min);
        }
        else if (parameters["mode"].toString() == "max")
        {
          if (isDstAdd && isSrcAdd) SET_MODE(addmaxadd);
          else if (isDstAdd && isSrcSub) SET_MODE(addmaxsub);
          else if (isDstSub && isSrcAdd) SET_MODE(submaxadd);
          else if (isDstSub && isSrcSub) SET_MODE(submaxsub);
          else if (isDstAdd) SET_MODE(addmax);
          else if (isSrcAdd) SET_MODE(maxadd);
          else if (isDstSub) SET_MODE(submax);
          else if (isSrcSub) SET_MODE(maxsub);
          else SET_MODE(max);
        }
        else
          error = "\"mode\" must be either \"and\", \"or\", \"xor\", \"andn\", \"min\" or \"max\"";
      }
#undef SET_MODE
    }
  }

  InputConfiguration &input_configuration() const override { return InPlaceTwoFrame(); }

  static Signature filter_signature()
  {
    Signature signature = "mt_logic";

    signature.add(Parameter(TYPE_CLIP, "", false));
    signature.add(Parameter(TYPE_CLIP, "", false));
    signature.add(Parameter(String(""), "mode", false));
    signature.add(Parameter(0.0, "th1", true));
    signature.add(Parameter(0.0, "th2", true));

    add_defaults(signature);

    signature.add(Parameter(false, "stacked", false));
    return signature;

  }

};


} } } }

#endif