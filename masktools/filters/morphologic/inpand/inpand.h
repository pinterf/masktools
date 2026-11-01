#ifndef __Mt_Inpand_H__
#define __Mt_Inpand_H__

#include "../morphologic.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Morphologic { namespace Inpand {


extern Processor *inpand_square_c;
extern Processor *inpand_square_sse2;

extern Processor *inpand_horizontal_c;
extern Processor *inpand_horizontal_sse2;

extern Processor *inpand_vertical_c;
extern Processor *inpand_vertical_sse2;

extern Processor *inpand_both_c;
extern Processor *inpand_both_sse2;

extern Processor *inpand_custom_c;

/* 16 bit */

extern Processor16 *inpand_square_16_c;
extern Processor16 *inpand_horizontal_16_c;
extern Processor16 *inpand_vertical_16_c;
extern Processor16 *inpand_both_16_c;
extern Processor16 *inpand_custom_16_c;
extern Processor16* inpand_custom_16_avx2_c;

extern Processor16 *inpand_square_sse4_16;
extern Processor16 *inpand_horizontal_sse4_16;
extern Processor16 *inpand_vertical_sse4_16;
extern Processor16 *inpand_both_sse4_16;

extern Processor16* inpand_square_avx2_16;
extern Processor16* inpand_horizontal_avx2_16;
extern Processor16* inpand_vertical_avx2_16;
extern Processor16* inpand_both_avx2_16;

#ifdef NEON_INTRINSICS
extern Processor *inpand_square_neon;
extern Processor16 *inpand_square_16_neon;
extern Processor *inpand_both_neon;
extern Processor16 *inpand_both_16_neon;
extern Processor *inpand_horizontal_neon;
extern Processor16 *inpand_horizontal_16_neon;
extern Processor *inpand_vertical_neon;
extern Processor16 *inpand_vertical_16_neon;
#endif

/* 32 bit */
extern Processor32 *inpand32_square_c;
extern Processor32 *inpand32_horizontal_c;
extern Processor32 *inpand32_vertical_c;
extern Processor32 *inpand32_both_c;
extern Processor32 *inpand32_custom_c;

class Inpand : public Morphologic::MorphologicFilter
{
public:
  Inpand(const Parameters&parameters, CpuFlags cpuFlags) : Morphologic::MorphologicFilter(parameters, (CpuFlags)cpuFlags)
  {
    int _bits_per_pixel = bit_depths[C];
    bool _isStacked = parameters["stacked"].toBool();
    if (_isStacked)
      _bits_per_pixel = 16;
    /* add the processors */
    if (parameters["mode"].toString() == "square")
    {
      if (_bits_per_pixel == 8) {
        processors.push_back(Filtering::Processor<Processor>(inpand_square_c, Constraint(CPU_NONE), 0));
#ifdef NEON_INTRINSICS
        processors.push_back(Filtering::Processor<Processor>(inpand_square_neon, Constraint(CPU_NEON, 17), 1));
#endif
#ifdef INTEL_INTRINSICS
        processors.push_back(Filtering::Processor<Processor>(inpand_square_sse2, Constraint(CPU_SSE2, 17), 3));
#endif
      }
      else if (_bits_per_pixel <= 16) {
        processors16.push_back(Filtering::Processor<Processor16>(inpand_square_16_c, Constraint(CPU_NONE), 0));
#ifdef NEON_INTRINSICS
        processors16.push_back(Filtering::Processor<Processor16>(inpand_square_16_neon, Constraint(CPU_NEON, 9), 1));
#endif
#ifdef INTEL_INTRINSICS
        processors16.push_back(Filtering::Processor<Processor16>(inpand_square_sse4_16, Constraint(CPU_SSE4_1, 16), 1));
        processors16.push_back(Filtering::Processor<Processor16>(inpand_square_avx2_16, Constraint(CPU_AVX2, 32), 3));
#endif
      }
      else {
        processors32.push_back(Filtering::Processor<Processor32>(inpand32_square_c, Constraint(CPU_NONE), 0));
      }
    }
    else if (parameters["mode"].toString() == "horizontal")
    {
      if (_bits_per_pixel == 8) {
        processors.push_back(Filtering::Processor<Processor>(inpand_horizontal_c, Constraint(CPU_NONE), 0));
#ifdef NEON_INTRINSICS
        processors.push_back(Filtering::Processor<Processor>(inpand_horizontal_neon, Constraint(CPU_NEON, 17), 1));
#endif
#ifdef INTEL_INTRINSICS
        processors.push_back(Filtering::Processor<Processor>(inpand_horizontal_sse2, Constraint(CPU_SSE2, 17), 3));
#endif
      }
      else if (_bits_per_pixel <= 16) {
        processors16.push_back(Filtering::Processor<Processor16>(inpand_horizontal_16_c, Constraint(CPU_NONE), 0));
#ifdef NEON_INTRINSICS
        processors16.push_back(Filtering::Processor<Processor16>(inpand_horizontal_16_neon, Constraint(CPU_NEON, 9), 1));
#endif
#ifdef INTEL_INTRINSICS
        processors16.push_back(Filtering::Processor<Processor16>(inpand_horizontal_sse4_16, Constraint(CPU_SSE4_1, 16), 1));
        processors16.push_back(Filtering::Processor<Processor16>(inpand_horizontal_avx2_16, Constraint(CPU_AVX2, 32), 3));
#endif
      }
      else {
        processors32.push_back(Filtering::Processor<Processor32>(inpand32_horizontal_c, Constraint(CPU_NONE), 0));
      }
    }
    else if (parameters["mode"].toString() == "vertical")
    {
      if (_bits_per_pixel == 8) {
        processors.push_back(Filtering::Processor<Processor>(inpand_vertical_c, Constraint(CPU_NONE), 0));
#ifdef NEON_INTRINSICS
        processors.push_back(Filtering::Processor<Processor>(inpand_vertical_neon, Constraint(CPU_NEON, 17), 1));
#endif
#ifdef INTEL_INTRINSICS
        processors.push_back(Filtering::Processor<Processor>(inpand_vertical_sse2, Constraint(CPU_SSE2, 16), 3));
#endif
      }
      else if (_bits_per_pixel <= 16) {
        processors16.push_back(Filtering::Processor<Processor16>(inpand_vertical_16_c, Constraint(CPU_NONE), 0));
#ifdef NEON_INTRINSICS
        processors16.push_back(Filtering::Processor<Processor16>(inpand_vertical_16_neon, Constraint(CPU_NEON, 9), 1));
#endif
#ifdef INTEL_INTRINSICS
        processors16.push_back(Filtering::Processor<Processor16>(inpand_vertical_sse4_16, Constraint(CPU_SSE4_1, 16), 1));
        processors16.push_back(Filtering::Processor<Processor16>(inpand_vertical_avx2_16, Constraint(CPU_AVX2, 32), 3));
#endif
      }
      else {
        processors32.push_back(Filtering::Processor<Processor32>(inpand32_vertical_c, Constraint(CPU_NONE), 0));
      }
    }
    else if (parameters["mode"].toString() == "both")
    {
      if (_bits_per_pixel == 8) {
        processors.push_back(Filtering::Processor<Processor>(inpand_both_c, Constraint(CPU_NONE), 0));
#ifdef NEON_INTRINSICS
        processors.push_back(Filtering::Processor<Processor>(inpand_both_neon, Constraint(CPU_NEON, 17), 1));
#endif
#ifdef INTEL_INTRINSICS
        processors.push_back(Filtering::Processor<Processor>(inpand_both_sse2, Constraint(CPU_SSE2, 17), 3));
#endif
      }
      else if (_bits_per_pixel <= 16) {
        processors16.push_back(Filtering::Processor<Processor16>(inpand_both_16_c, Constraint(CPU_NONE), 0));
#ifdef NEON_INTRINSICS
        processors16.push_back(Filtering::Processor<Processor16>(inpand_both_16_neon, Constraint(CPU_NEON, 9), 1));
#endif
#ifdef INTEL_INTRINSICS
        processors16.push_back(Filtering::Processor<Processor16>(inpand_both_sse4_16, Constraint(CPU_SSE4_1, 16), 1));
        processors16.push_back(Filtering::Processor<Processor16>(inpand_both_avx2_16, Constraint(CPU_AVX2, 32), 3));
#endif
      }
      else {
        processors32.push_back(Filtering::Processor<Processor32>(inpand32_both_c, Constraint(CPU_NONE), 0));
      }
    }
    else
    {
      if (_bits_per_pixel == 8) {
        processors.push_back(Filtering::Processor<Processor>(inpand_custom_c, Constraint(CPU_NONE), 0));
      }
      else if (_bits_per_pixel <= 16) {
        processors16.push_back(Filtering::Processor<Processor16>(inpand_custom_16_c, Constraint(CPU_NONE), 0));
#ifdef INTEL_INTRINSICS
        processors16.push_back(Filtering::Processor<Processor16>(inpand_custom_16_avx2_c, Constraint(CPU_AVX2), 1));
#endif
      }
      else {
        processors32.push_back(Filtering::Processor<Processor32>(inpand32_custom_c, Constraint(CPU_NONE), 0));
      }
      FillCoordinates(parameters["mode"].toString());
    }
  }

  static Signature filter_signature()
  {
    Signature signature = "mt_inpand";

    signature.add(Parameter(TYPE_CLIP, "", false));
    signature.add(Parameter(TYPE_FLOAT, "thY", true)); // overwritten to default 255..65535 in morphologic.h
    signature.add(Parameter(TYPE_FLOAT, "thC", true));
    signature.add(Parameter(String("square"), "mode", false));

    add_defaults(signature);

    signature.add(Parameter(false, "stacked", false));
    return signature;
  }
};

} } } } } // namespace Inpand, Morphologic, Filter, MaskTools, Filtering

#endif