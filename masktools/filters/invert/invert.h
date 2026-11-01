#ifndef __Mt_Invert_H__
#define __Mt_Invert_H__

#include "../../common/base/filter.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Invert {

typedef void(Processor)(uint8_t *pDst, ptrdiff_t nDstPitch, int nWidth, int nHeight, int bits_per_pixel); // bits_per_pixel: 16 bit only

Processor invert_c;
Processor invert_sse2;
Processor invert16_c;
Processor invert16_sse2;

Processor invert32_c;
Processor invert32_sse2;

#ifdef NEON_INTRINSICS
extern Processor *invert_neon;
extern Processor *invert16_neon;
extern Processor *invert32_neon;
#endif

// mt_invert: max_pixel_value - x on every processed plane (8 bit: 255 - x, 10-16 bit: (1 << bits) - 1 - x,
// float: 1.0 - x). Masks are full range on every plane, U and V too: 0..max_pixel_value, float 0..1.0 (not the
// signed -0.5..0.5 of float chroma). On a picture no chroma logic (chroma is signed around the neutral value;
// AviSynth's Invert handles that) and no limited range (16-235 luma or RGB is not mapped onto itself;
// AviSynth's Invert does not do it either).
class Invert : public MaskTools::Filter
{
   ProcessorList<Processor> processors;
   int bits_per_pixel;

protected:
    virtual void process(int n, const Plane<uint8_t> &dst, int nPlane, const Frame<const uint8_t> frames[4], const Constraint constraints[4]) override
    {
        UNUSED(n); UNUSED(frames);
        processors.best_processor(constraints[nPlane])(dst.data(), dst.pitch(), dst.width(), dst.height(), bits_per_pixel);
    }

public:
  Invert(const Parameters &parameters, CpuFlags cpuFlags) : MaskTools::Filter(parameters, FilterProcessingType::INPLACE, (CpuFlags)cpuFlags)
  {
    bits_per_pixel = bit_depths[C];

    /* add the processors */
    switch (bits_per_pixel) {
    case 8:
      processors.push_back(Filtering::Processor<Processor>(invert_c, Constraint(CPU_NONE), 0));
#ifdef NEON_INTRINSICS
      processors.push_back(Filtering::Processor<Processor>(invert_neon, Constraint(CPU_NEON), 1));
#endif
#ifdef INTEL_INTRINSICS
      processors.push_back(Filtering::Processor<Processor>(invert_sse2, Constraint(CPU_SSE2, MODULO_16, 16), 1));
#endif
      break;
    case 10: case 12: case 14: case 16:
      processors.push_back(Filtering::Processor<Processor>(invert16_c, Constraint(CPU_NONE), 0));
#ifdef NEON_INTRINSICS
      processors.push_back(Filtering::Processor<Processor>(invert16_neon, Constraint(CPU_NEON), 1));
#endif
#ifdef INTEL_INTRINSICS
      processors.push_back(Filtering::Processor<Processor>(invert16_sse2, Constraint(CPU_SSE2, MODULO_16, 16), 1));
#endif
      break;
    case 32:
      processors.push_back(Filtering::Processor<Processor>(invert32_c, Constraint(CPU_NONE), 0));
#ifdef NEON_INTRINSICS
      processors.push_back(Filtering::Processor<Processor>(invert32_neon, Constraint(CPU_NEON), 1));
#endif
#ifdef INTEL_INTRINSICS
      processors.push_back(Filtering::Processor<Processor>(invert32_sse2, Constraint(CPU_SSE2, MODULO_16, 16), 1));
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