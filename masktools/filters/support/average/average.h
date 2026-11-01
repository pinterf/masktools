#ifndef __Mt_Average_H__
#define __Mt_Average_H__

#include "../../../common/base/filter.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Support { namespace Average {

/* 8 bits */
typedef void(Processor)(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int nWidth, int nHeight);
typedef void(Processor16)(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int nWidth, int nHeight, int nOrigHeight);
typedef void(Processor32)(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int nWidth, int nHeight);

Processor average_c;
extern Processor *average_sse2;

/* 16 bits */

Processor16 average16_native_c;
Processor16 average16_native_sse2;

Processor32 average32_c;
extern Processor32 *average32_sse2;


#ifdef NEON_INTRINSICS
extern Processor *average_neon;
extern Processor16 *average16_native_neon;
extern Processor32 *average32_neon;
#endif

class Average : public MaskTools::Filter
{
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
            dst.width(), dst.height());
        else if (bits_per_pixel <= 16)
          processors16.best_processor(constraints[nPlane])(dst.data(), dst.pitch(),
            frames[0].plane(nPlane).data(), frames[0].plane(nPlane).pitch(),
            dst.width(), dst.height(), dst.origheight());
        else
          processors32.best_processor(constraints[nPlane])(dst.data(), dst.pitch(),
            frames[0].plane(nPlane).data(), frames[0].plane(nPlane).pitch(),
            dst.width(), dst.height());
    }

public:
    Average(const Parameters &parameters, CpuFlags cpuFlags) : MaskTools::Filter(parameters, FilterProcessingType::INPLACE, (CpuFlags)cpuFlags)
    {
      bool isStacked = parameters["stacked"].toBool();
      bits_per_pixel = bit_depths[C];

      if (isStacked && bits_per_pixel != 8) {
        error = "Stacked specified for a non-8 bit clip";
        return;
      }
      if (isStacked)
        bits_per_pixel = 16;

      /* add the processors */
      if (bits_per_pixel == 8) {
        processors.push_back(Filtering::Processor<Processor>(&average_c, Constraint(CPU_NONE), 0));
#ifdef NEON_INTRINSICS
        processors.push_back(Filtering::Processor<Processor>(average_neon, Constraint(CPU_NEON), 1));
#endif
#ifdef INTEL_INTRINSICS
        processors.push_back(Filtering::Processor<Processor>(average_sse2, Constraint(CPU_SSE2), 1));
#endif
      }else if(bits_per_pixel <= 16)
      {
        processors16.push_back(Filtering::Processor<Processor16>(average16_native_c, Constraint(CPU_NONE), 0));
#ifdef NEON_INTRINSICS
        processors16.push_back(Filtering::Processor<Processor16>(average16_native_neon, Constraint(CPU_NEON), 1));
#endif
#ifdef INTEL_INTRINSICS
        processors16.push_back(Filtering::Processor<Processor16>(average16_native_sse2, Constraint(CPU_SSE2), 1));
#endif
      }
      else {
        processors32.push_back(Filtering::Processor<Processor32>(&average32_c, Constraint(CPU_NONE), 0));
#ifdef NEON_INTRINSICS
        processors32.push_back(Filtering::Processor<Processor32>(average32_neon, Constraint(CPU_NEON), 1));
#endif
#ifdef INTEL_INTRINSICS
        processors32.push_back(Filtering::Processor<Processor32>(average32_sse2, Constraint(CPU_SSE2), 1));
#endif
      }
    }

    InputConfiguration &input_configuration() const override { return InPlaceTwoFrame(); }

    static Signature filter_signature()
    {
        Signature signature = "mt_average";

        signature.add(Parameter(TYPE_CLIP, "", false));
        signature.add(Parameter(TYPE_CLIP, "", false));

        add_defaults(signature);

        signature.add(Parameter(false, "stacked", false));
        return signature;
    }

};

} } } } }

#endif