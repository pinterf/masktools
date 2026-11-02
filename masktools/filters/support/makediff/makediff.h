#ifndef __Mt_MakeDiff_H__
#define __Mt_MakeDiff_H__

#include "../../../common/base/filter.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Support { namespace MakeDiff {

/* 8 bit */
typedef void(Processor)(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int nWidth, int nHeight);
typedef void(Processor16)(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int nWidth, int nHeight, int bits_per_pixel);
typedef void(Processor32)(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int nWidth, int nHeight, bool chroma);

Processor makediff_c;
extern Processor *makediff_sse2;
extern Processor *makediff_avx2;

/* 16 bit */


Processor16 makediff16_native_c;
extern Processor16 *makediff16_native_sse2;
extern Processor16 *makediff16_native_avx2;

/* 32 bit */
Processor32 makediff32_c;
extern Processor32 *makediff32_sse2;
extern Processor32 *makediff32_avx2;

#ifdef NEON_INTRINSICS
extern Processor *makediff_neon;
extern Processor16 *makediff16_native_neon;
extern Processor32 *makediff32_neon;
#endif

class MakeDiff : public MaskTools::Filter
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
            dst.width(), dst.height(), bits_per_pixel);
        else {
          const bool chroma = ((nPlane == 1 || nPlane == 2) && !planes_isRGB[C]); // chroma center is 0.0 others are 0.5
          processors32.best_processor(constraints[nPlane])(dst.data(), dst.pitch(),
            frames[0].plane(nPlane).data(), frames[0].plane(nPlane).pitch(),
            dst.width(), dst.height(), chroma);
        }
    }

public:
    MakeDiff(const Parameters &parameters, CpuFlags cpuFlags) : MaskTools::Filter(parameters, FilterProcessingType::INPLACE, (CpuFlags)cpuFlags)
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
      switch (bits_per_pixel) {
      case 8:
        processors.push_back(Filtering::Processor<Processor>(&makediff_c, Constraint(CPU_NONE), 0));
#ifdef INTEL_INTRINSICS
        processors.push_back(Filtering::Processor<Processor>(makediff_sse2, Constraint(CPU_SSE2), 1));
        processors.push_back(Filtering::Processor<Processor>(makediff_avx2, Constraint(CPU_AVX2), 3));
#endif
#ifdef NEON_INTRINSICS
        processors.push_back(Filtering::Processor<Processor>(makediff_neon, Constraint(CPU_NEON), 1));
#endif
        break;
      case 10: case 12: case 14: case 16:
        processors16.push_back(Filtering::Processor<Processor16>(&makediff16_native_c, Constraint(CPU_NONE), 0));
#ifdef INTEL_INTRINSICS
        processors16.push_back(Filtering::Processor<Processor16>(makediff16_native_sse2, Constraint(CPU_SSE2), 1));
        processors16.push_back(Filtering::Processor<Processor16>(makediff16_native_avx2, Constraint(CPU_AVX2), 3));
#endif
#ifdef NEON_INTRINSICS
        processors16.push_back(Filtering::Processor<Processor16>(makediff16_native_neon, Constraint(CPU_NEON), 1));
#endif
        break;
      case 32:
        processors32.push_back(Filtering::Processor<Processor32>(&makediff32_c, Constraint(CPU_NONE), 0));
#ifdef INTEL_INTRINSICS
        processors32.push_back(Filtering::Processor<Processor32>(makediff32_sse2, Constraint(CPU_SSE2), 1));
        processors32.push_back(Filtering::Processor<Processor32>(makediff32_avx2, Constraint(CPU_AVX2), 3));
#endif
#ifdef NEON_INTRINSICS
        processors32.push_back(Filtering::Processor<Processor32>(makediff32_neon, Constraint(CPU_NEON), 1));
#endif
        break;
      }
    }

    InputConfiguration &input_configuration() const override { return InPlaceTwoFrame(); }

    static Signature filter_signature()
    {
        Signature signature = "mt_makediff";

        signature.add(Parameter(TYPE_CLIP, "", false));
        signature.add(Parameter(TYPE_CLIP, "", false));

        add_defaults(signature);

        signature.add(Parameter(false, "stacked", false));
        return signature;
    }

};

} } } } }

#endif