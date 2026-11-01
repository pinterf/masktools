#ifndef __Mt_Inflate_H__
#define __Mt_Inflate_H__

#include "../morphologic.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Morphologic { namespace Inflate {

extern Processor *inflate_c;
extern Processor *inflate_sse2;

/* 16 bit */
extern Processor16 *inflate_16_c;

extern Processor16 *inflate_sse4_16;

#ifdef NEON_INTRINSICS
extern Processor *inflate_neon;
extern Processor16 *inflate_16_neon;
#endif

/*32 bit */
extern Processor32 *inflate32_c;

class Inflate : public Morphologic::MorphologicFilter
{
public:
  Inflate(const Parameters &parameters, CpuFlags cpuFlags) : Morphologic::MorphologicFilter(parameters, (CpuFlags)cpuFlags)
  {
    int _bits_per_pixel = bit_depths[C];
    bool _isStacked = parameters["stacked"].toBool();
    if (_isStacked)
      _bits_per_pixel = 16;
    /* add the processors */
    if (_bits_per_pixel == 8) {
      processors.push_back(Filtering::Processor<Processor>(inflate_c, Constraint(CPU_NONE), 0));
#ifdef NEON_INTRINSICS
      processors.push_back(Filtering::Processor<Processor>(inflate_neon, Constraint(CPU_NEON, 17), 1));
#endif
#ifdef INTEL_INTRINSICS
      processors.push_back(Filtering::Processor<Processor>(inflate_sse2, Constraint(CPU_SSE2, 17), 1));
#endif
    }
    else if (_bits_per_pixel <= 16){
      processors16.push_back(Filtering::Processor<Processor16>(inflate_16_c, Constraint(CPU_NONE), 0));
#ifdef NEON_INTRINSICS
      processors16.push_back(Filtering::Processor<Processor16>(inflate_16_neon, Constraint(CPU_NEON, 9), 1));
#endif
#ifdef INTEL_INTRINSICS
      processors16.push_back(Filtering::Processor<Processor16>(inflate_sse4_16, Constraint(CPU_SSE4_1, 9), 1));
#endif
    }
    else {
      processors32.push_back(Filtering::Processor<Processor32>(inflate32_c, Constraint(CPU_NONE), 0));
    }
  }

  static Signature filter_signature()
  {
    Signature signature = "mt_inflate";

    signature.add(Parameter(TYPE_CLIP, "", false));
    signature.add(Parameter(TYPE_FLOAT, "thY", true)); // overwritten to default 255..65535 in morphologic.h
    signature.add(Parameter(TYPE_FLOAT, "thC", true));

    add_defaults(signature);

    signature.add(Parameter(false, "stacked", false));
    return signature;
  }
};

} } } } } // namespace Inflate, Morphologic, Filter, MaskTools, Filtering

#endif