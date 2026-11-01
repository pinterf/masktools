#ifndef __Mt_Deflate_H__
#define __Mt_Deflate_H__

#include "../morphologic.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Morphologic { namespace Deflate {

extern Processor *deflate_c;
extern Processor *deflate_sse2;

/* 16 bit */
extern Processor16 *deflate_16_c;

extern Processor16 *deflate_sse4_16;

#ifdef NEON_INTRINSICS
extern Processor *deflate_neon;
extern Processor16 *deflate_16_neon;
#endif

/* 32 bit */
extern Processor32 *deflate32_c;

class Deflate : public Morphologic::MorphologicFilter
{
public:
  Deflate(const Parameters &parameters, CpuFlags cpuFlags) : Morphologic::MorphologicFilter(parameters, (CpuFlags)cpuFlags)
  {
    int _bits_per_pixel = bit_depths[C];
    bool _isStacked = parameters["stacked"].toBool();
    if (_isStacked)
      _bits_per_pixel = 16;
    /* add the processors */
    if (_bits_per_pixel == 8) {
      processors.push_back(Filtering::Processor<Processor>(deflate_c, Constraint(CPU_NONE), 0));
#ifdef NEON_INTRINSICS
      processors.push_back(Filtering::Processor<Processor>(deflate_neon, Constraint(CPU_NEON, 17), 1));
#endif
#ifdef INTEL_INTRINSICS
      processors.push_back(Filtering::Processor<Processor>(deflate_sse2, Constraint(CPU_SSE2, 17), 1));
#endif
    }
    else if (_bits_per_pixel <= 16) {
      processors16.push_back(Filtering::Processor<Processor16>(deflate_16_c, Constraint(CPU_NONE), 0));
#ifdef NEON_INTRINSICS
      processors16.push_back(Filtering::Processor<Processor16>(deflate_16_neon, Constraint(CPU_NEON, 9), 1));
#endif
#ifdef INTEL_INTRINSICS
      processors16.push_back(Filtering::Processor<Processor16>(deflate_sse4_16, Constraint(CPU_SSE4_1, 9), 1));
#endif
    }
    else {
      processors32.push_back(Filtering::Processor<Processor32>(deflate32_c, Constraint(CPU_NONE), 0));
    }
  }

  static Signature filter_signature()
  {
    Signature signature = "mt_deflate";

    signature.add(Parameter(TYPE_CLIP, "", false));
    signature.add(Parameter(TYPE_FLOAT, "thY", true)); // overwritten to default 255..65535 in morphologic.h
    signature.add(Parameter(TYPE_FLOAT, "thC", true));

    add_defaults(signature);

    signature.add(Parameter(false, "stacked", false));
    return signature;
  }
};

} } } } } // namespace Deflate, Morphologic, Filter, MaskTools, Filtering

#endif