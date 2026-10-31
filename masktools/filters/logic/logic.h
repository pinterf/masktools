#ifndef __Mt_Logic_H__
#define __Mt_Logic_H__

#include "../../common/base/filter.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Logic {

typedef void(Processor)(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch, int nWidth, int nHeight, uint8_t nThresholdDestination, uint8_t nThresholdSource);
typedef void(Processor16)(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int nWidth, int nHeight, int nOrigHeight, uint16_t nThresholdDestination, uint16_t nThresholdSource);
typedef void(Processor32)(float *pDst, ptrdiff_t nDstPitch, const float *pSrc1, ptrdiff_t nSrc1Pitch, int nWidth, int nHeight, float nThresholdDestination, float nThresholdSource);

#define DEFINE_PROCESSOR(name) \
   extern Processor *name##_c; \
   extern Processor *name##_sse2; \
   extern Processor *name##_asse2; \
   extern Processor *name##_avx2; \
   extern Processor *name##_aavx2;

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
   extern Processor16 *name##_stacked_c; \
   extern Processor16 *name##_native_c; \
   extern Processor16 *name##_stacked_sse2; \
   extern Processor16 *name##_native_sse2; \
   extern Processor16 *name##_native_avx2;

DEFINE_PROCESSOR(and16);
DEFINE_PROCESSOR(or16 );
DEFINE_PROCESSOR(andn16);
DEFINE_PROCESSOR(xor16);

#define DEFINE_TRIPLE(mode, bits_per_pixel) \
DEFINE_PROCESSOR(mode); \
DEFINE_PROCESSOR(mode##add##bits_per_pixel); \
DEFINE_PROCESSOR(mode##sub)

// for 8 bit it was 3x3 (NINE), now it is 6x3
#define DEFINE_NINE(mode) \
DEFINE_TRIPLE(mode,10); \
DEFINE_TRIPLE(mode,12); \
DEFINE_TRIPLE(mode,14); \
DEFINE_TRIPLE(mode,16); \
DEFINE_TRIPLE(add10##mode,10); \
DEFINE_TRIPLE(add12##mode,12); \
DEFINE_TRIPLE(add14##mode,14); \
DEFINE_TRIPLE(add16##mode,16); \
DEFINE_TRIPLE(sub##mode,10) \
DEFINE_TRIPLE(sub##mode,12) \
DEFINE_TRIPLE(sub##mode,14) \
DEFINE_TRIPLE(sub##mode,16)

DEFINE_NINE(min);
DEFINE_NINE(max);

#undef DEFINE_TRIPLE
#undef DEFINE_NINE
#undef DEFINE_PROCESSOR

/* 32 bit */
#define DEFINE_PROCESSOR(name) \
   extern Processor32 *name##_32_c; \
   extern Processor32 *name##_32_sse2; \
   extern Processor32 *name##_32_asse2; \
   extern Processor32 *name##_32_avx; \
   extern Processor32 *name##_32_aavx;

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
        dst.width(), dst.height(), dst.origheight(), (uint16_t)nThresholdDestination, (uint16_t)nThresholdSource);
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
      processors.push_back( Filtering::Processor<Processor>( mode##_c, Constraint( CPU_NONE, 1, 1, 1, 1 ), 0 ) ); \
      processors.push_back( Filtering::Processor<Processor>( mode##_sse2, Constraint( CPU_SSE2 , 1, 1, 1, 1 ), 1 ) ); \
      processors.push_back( Filtering::Processor<Processor>( mode##_asse2, Constraint( CPU_SSE2 , 1, 1, 16, 16 ), 2 ) ); \
      processors.push_back( Filtering::Processor<Processor>( mode##_avx2, Constraint( CPU_AVX2 , 1, 1, 1, 1 ), 3 ) ); \
      processors.push_back( Filtering::Processor<Processor>( mode##_aavx2, Constraint( CPU_AVX2 , 1, 1, 32, 32 ), 4 ) ); \
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
    if (isStacked) { \
        processors16.push_back( Filtering::Processor<Processor16>( mode##_stacked_c, Constraint( CPU_NONE, 1, 1, 1, 1 ), 0 ) ); \
        if(needSSE4) \
          processors16.push_back( Filtering::Processor<Processor16>( mode##_stacked_sse2, Constraint( CPU_SSE4_1 , 1, 1, 1, 1 ), 1 ) ); \
        else \
          processors16.push_back( Filtering::Processor<Processor16>( mode##_stacked_sse2, Constraint( CPU_SSE2 , 1, 1, 1, 1 ), 1 ) ); \
    } else { \
        processors16.push_back( Filtering::Processor<Processor16>( mode##_native_c, Constraint( CPU_NONE, 1, 1, 1, 1 ), 0 ) ); \
        if(needSSE4) \
          processors16.push_back( Filtering::Processor<Processor16>( mode##_native_sse2, Constraint( CPU_SSE4_1 , 1, 1, 1, 1 ), 1 ) ); \
        else \
          processors16.push_back( Filtering::Processor<Processor16>( mode##_native_sse2, Constraint( CPU_SSE2 , 1, 1, 1, 1 ), 1 ) ); \
        processors16.push_back( Filtering::Processor<Processor16>( mode##_native_avx2, Constraint( CPU_AVX2 , 1, 1, 1, 1 ), 1 ) ); \
    }

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
          if (isDstAdd && isSrcAdd) { 
            switch (bits_per_pixel) {
            case 10: SET_MODE(add10minadd10, true); break;
            case 12: SET_MODE(add12minadd12, true); break;
            case 14: SET_MODE(add14minadd14, true); break;
            case 16: SET_MODE(add16minadd16, true); break;
            }
          }
          else if (isDstAdd && isSrcSub) {
            switch (bits_per_pixel) {
            case 10: SET_MODE(add10minsub, true); break;
            case 12: SET_MODE(add12minsub, true); break;
            case 14: SET_MODE(add14minsub, true); break;
            case 16: SET_MODE(add16minsub, true); break;
            }
          }
          else if (isDstSub && isSrcAdd) {
            switch (bits_per_pixel) {
            case 10: SET_MODE(subminadd10, true); break;
            case 12: SET_MODE(subminadd12, true); break;
            case 14: SET_MODE(subminadd14, true); break;
            case 16: SET_MODE(subminadd16, true); break;
            }
          }
          else if (isDstSub && isSrcSub) { SET_MODE(subminsub, true); }
          else if (isDstAdd) {
            switch (bits_per_pixel) {
            case 10: SET_MODE(add10min, true); break;
            case 12: SET_MODE(add12min, true); break;
            case 14: SET_MODE(add14min, true); break;
            case 16: SET_MODE(add16min, true); break;
            }
          }
          else if (isSrcAdd) {
            switch (bits_per_pixel) {
            case 10: SET_MODE(minadd10, true); break;
            case 12: SET_MODE(minadd12, true); break;
            case 14: SET_MODE(minadd14, true); break;
            case 16: SET_MODE(minadd16, true); break;
            }
          }
          else if (isDstSub) { SET_MODE(submin, true); }
          else if (isSrcSub) { SET_MODE(minsub, true); }
          else { SET_MODE(min, true); }
        }
        else if (parameters["mode"].toString() == "max")
        {
          if (isDstAdd && isSrcAdd) {
            switch (bits_per_pixel) {
            case 10: SET_MODE(add10maxadd10, true); break;
            case 12: SET_MODE(add12maxadd12, true); break;
            case 14: SET_MODE(add14maxadd14, true); break;
            case 16: SET_MODE(add16maxadd16, true); break;
            }
          }
          else if (isDstAdd && isSrcSub) {
            switch (bits_per_pixel) {
            case 10: SET_MODE(add10maxsub, true); break;
            case 12: SET_MODE(add12maxsub, true); break;
            case 14: SET_MODE(add14maxsub, true); break;
            case 16: SET_MODE(add16maxsub, true); break;
            }
          }
          else if (isDstSub && isSrcAdd) {
            switch (bits_per_pixel) {
            case 10: SET_MODE(submaxadd10, true); break;
            case 12: SET_MODE(submaxadd12, true); break;
            case 14: SET_MODE(submaxadd14, true); break;
            case 16: SET_MODE(submaxadd16, true); break;
            }
          }
          else if (isDstSub && isSrcSub) { SET_MODE(submaxsub, true); }
          else if (isDstAdd) {
            switch (bits_per_pixel) {
            case 10: SET_MODE(add10max, true); break;
            case 12: SET_MODE(add12max, true); break;
            case 14: SET_MODE(add14max, true); break;
            case 16: SET_MODE(add16max, true); break;
            }
          }
          else if (isSrcAdd) {
            switch (bits_per_pixel) {
            case 10: SET_MODE(maxadd10, true); break;
            case 12: SET_MODE(maxadd12, true); break;
            case 14: SET_MODE(maxadd14, true); break;
            case 16: SET_MODE(maxadd16, true); break;
            }
          }
          else if (isDstSub) { SET_MODE(submax, true); }
          else if (isSrcSub) { SET_MODE(maxsub, true); }
          else { SET_MODE(max, true); }
        }
        else
          error = "\"mode\" must be either \"and\", \"or\", \"xor\", \"andn\", \"min\" or \"max\"";
      }
#undef SET_MODE

    }
    else { // float
#define SET_MODE(mode) \
   do { \
      processors32.push_back( Filtering::Processor<Processor32>( mode##_32_c, Constraint( CPU_NONE, 1, 1, 1, 1 ), 0 ) ); \
      processors32.push_back( Filtering::Processor<Processor32>( mode##_32_sse2, Constraint( CPU_SSE2 , 1, 1, 1, 1 ), 1 ) ); \
      processors32.push_back( Filtering::Processor<Processor32>( mode##_32_asse2, Constraint( CPU_SSE2 , 1, 1, 16, 16 ), 2 ) ); \
      processors32.push_back( Filtering::Processor<Processor32>( mode##_32_avx, Constraint( CPU_AVX , 1, 1, 1, 1 ), 3 ) ); \
      processors32.push_back( Filtering::Processor<Processor32>( mode##_32_aavx, Constraint( CPU_AVX , 1, 1, 32, 32 ), 4 ) ); \
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