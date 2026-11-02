#ifndef __Mt_Clamp_H__
#define __Mt_Clamp_H__

#include "../../../common/base/filter.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Support { namespace Clamp {

typedef void(Processor)(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch, const uint8_t *pSrc2, ptrdiff_t nSrc2Pitch, int nWidth, int nHeight, int nOvershoot, int nUndershoot);
typedef void(Processor16)(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pUpLimit, ptrdiff_t nUpLimitPitch, const uint8_t *pLowLimit, ptrdiff_t nLowLimitPitch, int nWidth, int nHeight, int bits_per_pixel, int nOvershoot, int nUndershoot);
typedef void(Processor32)(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pUpLimit, ptrdiff_t nUpLimitPitch, const uint8_t *pLowLimit, ptrdiff_t nLowLimitPitch, int nWidth, int nHeight, float nOvershoot, float nUndershoot);

Processor clamp_c;
extern Processor *clamp_sse2;

/* 16 bit */

Processor16 clamp16_native_c;
extern Processor16 *clamp16_native_sse2;
extern Processor16 *clamp16_native_sse4_1;

/* 32 bit */
Processor32 clamp32_c;
extern Processor32 *clamp32_sse2;


#ifdef NEON_INTRINSICS
extern Processor *clamp_neon;
extern Processor16 *clamp16_native_neon;
extern Processor32 *clamp32_neon;
#endif

class Clamp : public MaskTools::Filter
{
    int nOvershoot, nUndershoot;
    float nOvershoot_f, nUndershoot_f;
    ProcessorList<Processor> processors;
    ProcessorList<Processor16> processors16;
    ProcessorList<Processor32> processors32;
    int bits_per_pixel;

protected:
  virtual void process(int n, const Plane<uint8_t> &dst, int nPlane, const Filtering::Frame<const uint8_t> frames[4], const Constraint constraints[4]) override
  {
    UNUSED(n);
    if (bits_per_pixel == 8)
      processors.best_processor(constraints[nPlane])(dst.data(), dst.pitch(),
        frames[0].plane(nPlane).data(), frames[0].plane(nPlane).pitch(),
        frames[1].plane(nPlane).data(), frames[1].plane(nPlane).pitch(),
        dst.width(), dst.height(), nOvershoot, nUndershoot);
    else if (bits_per_pixel <= 16)
      processors16.best_processor(constraints[nPlane])(dst.data(), dst.pitch(),
        frames[0].plane(nPlane).data(), frames[0].plane(nPlane).pitch(),
        frames[1].plane(nPlane).data(), frames[1].plane(nPlane).pitch(),
        dst.width(), dst.height(), bits_per_pixel, nOvershoot, nUndershoot);
    else
      processors32.best_processor(constraints[nPlane])(dst.data(), dst.pitch(),
        frames[0].plane(nPlane).data(), frames[0].plane(nPlane).pitch(),
        frames[1].plane(nPlane).data(), frames[1].plane(nPlane).pitch(),
        dst.width(), dst.height(), nOvershoot_f, nUndershoot_f);
  }

public:
    Clamp(const Parameters &parameters, CpuFlags cpuFlags) : MaskTools::Filter(parameters, FilterProcessingType::INPLACE, (CpuFlags)cpuFlags)
    {
        bool isStacked = parameters["stacked"].toBool();
        bits_per_pixel = bit_depths[C];

        if (isStacked && bits_per_pixel != 8) {
          error = "Stacked specified for a non-8 bit clip";
          return;
        }
        if (isStacked)
          bits_per_pixel = 16;

        bool fullscale = planes_isRGB[C];
        String scalemode = parameters["paramscale"].toString();

        // defaults
        nUndershoot_f = nOvershoot_f = 0.0f;
        nUndershoot = nOvershoot = 0;

        const char *errortxt = "invalid parameter: paramscale. Use i8, i10, i12, i14, i16, f32 for scale or none/empty to disable scaling";

        if (parameters["undershoot"].is_defined()) {
          nUndershoot_f = (float)parameters["undershoot"].toFloat();
          if (!ScaleParam(scalemode, nUndershoot_f, bits_per_pixel, nUndershoot_f, nUndershoot, fullscale, false, false))
          {
            error = errortxt;
            return;
          }
        }

        if (parameters["overshoot"].is_defined()) {
          nOvershoot_f = (float)parameters["overshoot"].toFloat();
          if (!ScaleParam(scalemode, nOvershoot_f, bits_per_pixel, nOvershoot_f, nOvershoot, fullscale, false, false))
          {
            error = errortxt;
            return;
          }
        }

        /* add the processors */
        switch (bits_per_pixel) {
        case 8:
          processors.push_back(Filtering::Processor<Processor>(&clamp_c, Constraint(CPU_NONE), 0));
#ifdef NEON_INTRINSICS
          processors.push_back(Filtering::Processor<Processor>(clamp_neon, Constraint(CPU_NEON), 1));
#endif
#ifdef INTEL_INTRINSICS
          processors.push_back(Filtering::Processor<Processor>(clamp_sse2, Constraint(CPU_SSE2), 1));
#endif
          break;
        case 10: case 12: case 14: case 16:
          processors16.push_back(Filtering::Processor<Processor16>(&clamp16_native_c, Constraint(CPU_NONE), 0));
#ifdef NEON_INTRINSICS
          processors16.push_back(Filtering::Processor<Processor16>(clamp16_native_neon, Constraint(CPU_NEON), 1));
#endif
#ifdef INTEL_INTRINSICS
          processors16.push_back(Filtering::Processor<Processor16>(clamp16_native_sse2, Constraint(CPU_SSE2), 1));
          processors16.push_back(Filtering::Processor<Processor16>(clamp16_native_sse4_1, Constraint(CPU_SSE4_1), 2));
#endif
          break;
        case 32:
          processors32.push_back(Filtering::Processor<Processor32>(clamp32_c, Constraint(CPU_NONE), 0));
#ifdef NEON_INTRINSICS
          processors32.push_back(Filtering::Processor<Processor32>(clamp32_neon, Constraint(CPU_NEON), 1));
#endif
#ifdef INTEL_INTRINSICS
          processors32.push_back(Filtering::Processor<Processor32>(clamp32_sse2, Constraint(CPU_SSE2), 1));
#endif
          break;
        }
    }

    InputConfiguration &input_configuration() const override { return InPlaceThreeFrame(); }

    static Signature filter_signature()
    {
        Signature signature = "mt_clamp";

        signature.add(Parameter(TYPE_CLIP, "", false));
        signature.add(Parameter(TYPE_CLIP, "", false));
        signature.add(Parameter(TYPE_CLIP, "", false));
        signature.add(Parameter(0.0f, "overshoot", true));
        signature.add(Parameter(0.0f, "undershoot", true));

        add_defaults(signature);

        signature.add(Parameter(false, "stacked", false));
        return signature;
    }
};

} } } } }

#endif
