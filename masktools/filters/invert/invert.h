#ifndef __Mt_Invert_H__
#define __Mt_Invert_H__

#include "../../common/base/filter.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Invert {

typedef void(Processor)(uint8_t *pDst, ptrdiff_t nDstPitch, int nWidth, int nHeight);

Processor invert_c;
Processor invert_sse2;
/* 10-16 */
extern Processor *invert10_c;
extern Processor *invert10_sse2;
extern Processor *invert12_c;
extern Processor *invert12_sse2;
extern Processor *invert14_c;
extern Processor *invert14_sse2;
extern Processor *invert16_c;
extern Processor *invert16_sse2;

Processor invert32_c;
Processor invert32_sse2;

#ifdef NEON_INTRINSICS
extern Processor *invert_neon;
extern Processor *invert10_neon;
extern Processor *invert12_neon;
extern Processor *invert14_neon;
extern Processor *invert16_neon;
extern Processor *invert32_neon;
#endif

class Invert : public MaskTools::Filter
{
   ProcessorList<Processor> processors;

protected:
    virtual void process(int n, const Plane<uint8_t> &dst, int nPlane, const Frame<const uint8_t> frames[4], const Constraint constraints[4]) override
    {
        UNUSED(n); UNUSED(frames);
        processors.best_processor(constraints[nPlane])(dst.data(), dst.pitch(), dst.width(), dst.height());
    }

public:
  Invert(const Parameters &parameters, CpuFlags cpuFlags) : MaskTools::Filter(parameters, FilterProcessingType::INPLACE, (CpuFlags)cpuFlags)
  {
    int bits_per_pixel = bit_depths[C];

    /* add the processors */
    switch (bits_per_pixel) {
    case 8:
      processors.push_back(Filtering::Processor<Processor>(invert_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0));
#ifdef NEON_INTRINSICS
      processors.push_back(Filtering::Processor<Processor>(invert_neon, Constraint(CPU_NEON, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 1), 1));
#endif
#ifdef INTEL_INTRINSICS
      processors.push_back(Filtering::Processor<Processor>(invert_sse2, Constraint(CPU_SSE2, 16, 1, ALIGNMENT_NONE, 16), 1));
#endif
      break;
    case 10:
      processors.push_back(Filtering::Processor<Processor>(invert10_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0));
#ifdef NEON_INTRINSICS
      processors.push_back(Filtering::Processor<Processor>(invert10_neon, Constraint(CPU_NEON, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 1), 1));
#endif
#ifdef INTEL_INTRINSICS
      processors.push_back(Filtering::Processor<Processor>(invert10_sse2, Constraint(CPU_SSE2, 16, 1, ALIGNMENT_NONE, 16), 1));
#endif
      break;
    case 12:
      processors.push_back(Filtering::Processor<Processor>(invert12_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0));
#ifdef NEON_INTRINSICS
      processors.push_back(Filtering::Processor<Processor>(invert12_neon, Constraint(CPU_NEON, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 1), 1));
#endif
#ifdef INTEL_INTRINSICS
      processors.push_back(Filtering::Processor<Processor>(invert12_sse2, Constraint(CPU_SSE2, 16, 1, ALIGNMENT_NONE, 16), 1));
#endif
      break;
    case 14:
      processors.push_back(Filtering::Processor<Processor>(invert14_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0));
#ifdef NEON_INTRINSICS
      processors.push_back(Filtering::Processor<Processor>(invert14_neon, Constraint(CPU_NEON, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 1), 1));
#endif
#ifdef INTEL_INTRINSICS
      processors.push_back(Filtering::Processor<Processor>(invert14_sse2, Constraint(CPU_SSE2, 16, 1, ALIGNMENT_NONE, 16), 1));
#endif
      break;
    case 16:
      processors.push_back(Filtering::Processor<Processor>(invert16_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0));
#ifdef NEON_INTRINSICS
      processors.push_back(Filtering::Processor<Processor>(invert16_neon, Constraint(CPU_NEON, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 1), 1));
#endif
#ifdef INTEL_INTRINSICS
      processors.push_back(Filtering::Processor<Processor>(invert16_sse2, Constraint(CPU_SSE2, 16, 1, ALIGNMENT_NONE, 16), 1));
#endif
      break;
    case 32:
      processors.push_back(Filtering::Processor<Processor>(invert32_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0));
#ifdef NEON_INTRINSICS
      processors.push_back(Filtering::Processor<Processor>(invert32_neon, Constraint(CPU_NEON, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 1), 1));
#endif
#ifdef INTEL_INTRINSICS
      processors.push_back(Filtering::Processor<Processor>(invert32_sse2, Constraint(CPU_SSE2, 16, 1, ALIGNMENT_NONE, 16), 1));
#endif
      break;
    }
  }

   InputConfiguration &input_configuration() const override { return InPlaceOneFrame(); }

   static Signature filter_signature()
   {
      Signature signature = "mt_invert";

      signature.add( Parameter( TYPE_CLIP, "", false) );

      return add_defaults( signature );
   }
};


} } } } // namespace Invert, Filters, MaskTools, Filtering

#endif