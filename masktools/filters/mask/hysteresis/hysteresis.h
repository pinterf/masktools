#ifndef __Mt_Hysteresis_H__
#define __Mt_Hysteresis_H__

#include "../../../common/base/filter.h"
#include <vector>

namespace Filtering { namespace MaskTools { namespace Filters { namespace Mask { namespace Hysteresis {

typedef void(Processor)(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch,
                        const uint8_t *pSrc2, ptrdiff_t nSrc2Pitch, uint8_t *pTemp, int nWidth, int nHeight, int bits_per_pixel);

extern Processor *hysteresis_8_c;
extern Processor *hysteresis_16_c;
extern Processor *hysteresis_32_c;
extern Processor* hysteresis_8_nocorner_c;
extern Processor* hysteresis_16_nocorner_c;
extern Processor* hysteresis_32_nocorner_c;

class Hysteresis : public MaskTools::Filter
{
    Processor *processor;
    int bits_per_pixel;

protected:
    virtual void process(int n, const Plane<uint8_t> &dst, int nPlane, const Filtering::Frame<const uint8_t> frames[4], const Constraint constraints[4]) override
    {
        UNUSED(n);
        UNUSED(constraints);
        // per call: the filter is MT_NICE, a shared buffer would be a data race
        std::vector<uint8_t> stack(size_t(dst.width()) * dst.height());
        processor(dst.data(), dst.pitch(),
            frames[0].plane(nPlane).data(), frames[0].plane(nPlane).pitch(),
            frames[1].plane(nPlane).data(), frames[1].plane(nPlane).pitch(),
            stack.data(), dst.width(), dst.height(), bits_per_pixel);
    }

public:
    Hysteresis(const Parameters &parameters, CpuFlags cpuFlags) : MaskTools::Filter(parameters, FilterProcessingType::CHILD, (CpuFlags)cpuFlags) {
      bits_per_pixel = bit_depths[C];
      const bool corners = parameters["corners"].toBool();

      switch (bits_per_pixel) {
      case 8: processor = corners ? hysteresis_8_c : hysteresis_8_nocorner_c; break;
      case 10: case 12: case 14: case 16: processor = corners ? hysteresis_16_c : hysteresis_16_nocorner_c; break;
      case 32: processor = corners ? hysteresis_32_c : hysteresis_32_nocorner_c; break;
      }
    }

    InputConfiguration &input_configuration() const override { return TwoFrame(); }

    static Signature filter_signature()
    {
        Signature signature = "mt_hysteresis";

        signature.add(Parameter(TYPE_CLIP, "", false));
        signature.add(Parameter(TYPE_CLIP, "", false));
        signature.add(Parameter(true, "corners", false)); // default true

        return add_defaults(signature);
    }
};


} } } } }

#endif