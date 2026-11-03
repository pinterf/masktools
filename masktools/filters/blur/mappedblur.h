#ifndef __Mt_Blur_H__
#define __Mt_Blur_H__

#include "../../common/base/filter.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Blur { 

typedef void(Processor)(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, const uint8_t *pMask, ptrdiff_t nMaskPitch, const int16_t *matrix, int nWidth, int nHeight);
typedef void(Processor16)(uint16_t *pDst, ptrdiff_t nDstPitch, const uint16_t *pSrc, ptrdiff_t nSrcPitch, const uint16_t *pMask, ptrdiff_t nMaskPitch, const int16_t *matrix, int nWidth, int nHeight, int bits_per_pixel);
typedef void(Processor32)(float *pDst, ptrdiff_t nDstPitch, const float *pSrc, ptrdiff_t nSrcPitch, const float *pMask, ptrdiff_t nMaskPitch, const float *matrix, int nWidth, int nHeight);

extern Processor *mapped_below_c;
extern Processor *mapped_all_c;
extern Processor16 *mapped_below_16_c;
extern Processor16 *mapped_all_16_c;
extern Processor32 *mapped_below_32_c;
extern Processor32 *mapped_all_32_c;

class MappedBlur : public MaskTools::Filter
{
   int16_t matrix[10];
   float matrix_f[10];

   int bits_per_pixel;

   ProcessorList<Processor> processors;
   ProcessorList<Processor16> processors16;
   ProcessorList<Processor32> processors32;

protected:
  virtual void process(int n, const Plane<uint8_t> &dst, int nPlane, const Frame<const uint8_t> frames[4], const Constraint constraints[4]) override
  {
    UNUSED(n);
    if (bits_per_pixel == 8) {
      processors.best_processor(constraints[nPlane])(dst.data(), dst.pitch(),
        frames[0].plane(nPlane).data(), frames[0].plane(nPlane).pitch(),
        frames[1].plane(nPlane).data(), frames[1].plane(nPlane).pitch(),
        matrix, dst.width(), dst.height());
    }
    else if (bits_per_pixel <= 16) {
      processors16.best_processor(constraints[nPlane])((uint16_t *)dst.data(), dst.pitch(),
        (uint16_t *)frames[0].plane(nPlane).data(), frames[0].plane(nPlane).pitch(),
        (uint16_t *)frames[1].plane(nPlane).data(), frames[1].plane(nPlane).pitch(),
        matrix, dst.width(), dst.height(), bits_per_pixel);
    }
    else {
      processors32.best_processor(constraints[nPlane])((float *)dst.data(), dst.pitch(),
        (float *)frames[0].plane(nPlane).data(), frames[0].plane(nPlane).pitch(),
        (float *)frames[1].plane(nPlane).data(), frames[1].plane(nPlane).pitch(),
        matrix_f, dst.width(), dst.height());
    }
  }

public:
   MappedBlur(const Parameters &parameters, CpuFlags cpuFlags) : MaskTools::Filter( parameters, FilterProcessingType::CHILD, (CpuFlags)cpuFlags)
   {
      bits_per_pixel = nBitDepth;
      bool isFloat = bits_per_pixel == 32;

      auto coeffs = Parser::getDefaultParser().parse(parameters["kernel"].toString(), Parser::SYMBOL_SEPARATORS).getExpression();

      if (isFloat) {
        memset(matrix_f, 0, sizeof(matrix_f));
        for (int i = 0; i < 9; i++)
        {
          if (!coeffs.size())
          {
            error = "invalid kernel";
            return;
          }

          matrix_f[9] += abs(matrix_f[i] = float(coeffs.front().getValue(0, 0, 0)));
          coeffs.pop_front();
        }

        if (coeffs.size())
          matrix_f[9] = float(coeffs.front().getValue(0, 0, 0));

        if (!matrix_f[9])
          matrix_f[9] = 1.0f;
      }
      else {
        memset(matrix, 0, sizeof(matrix));
        for (int i = 0; i < 9; i++)
        {
          if (!coeffs.size())
          {
            error = "invalid kernel";
            return;
          }

          matrix[9] += abs(matrix[i] = int16_t(coeffs.front().getValue(0, 0, 0)));
          coeffs.pop_front();
        }

        if (coeffs.size())
          matrix[9] = int16_t(coeffs.front().getValue(0, 0, 0));

        if (!matrix[9])
          matrix[9] = 1;
      }
      /* add the processors */
      if (parameters["mode"].toString() == "replace") {
        switch (bits_per_pixel) {
        case 8:
          processors.push_back(Filtering::Processor<Processor>(mapped_all_c, Constraint(CPU_NONE), 0));
          break;
        case 10: case 12: case 14: case 16:
          processors16.push_back(Filtering::Processor<Processor16>(mapped_all_16_c, Constraint(CPU_NONE), 0));
          break;
        case 32:
          processors32.push_back(Filtering::Processor<Processor32>(mapped_all_32_c, Constraint(CPU_NONE), 0));
          break;
        }
      }
      else if (parameters["mode"].toString() == "dump") {
        switch (bits_per_pixel) {
        case 8:
          processors.push_back(Filtering::Processor<Processor>(mapped_below_c, Constraint(CPU_NONE), 0));
          break;
        case 10: case 12: case 14: case 16:
          processors16.push_back(Filtering::Processor<Processor16>(mapped_below_16_c, Constraint(CPU_NONE), 0));
          break;
        case 32:
          processors32.push_back(Filtering::Processor<Processor32>(mapped_below_32_c, Constraint(CPU_NONE), 0));
          break;
        }
      }
      else
      {
         error = "unknown mode";
         return;
      }
   }

   InputConfiguration &input_configuration() const override { return TwoFrame(); }

   static Signature filter_signature()
   {
      Signature signature = "mt_mappedblur";

      signature.add(Parameter(TYPE_CLIP, "", false));
      signature.add(Parameter(TYPE_CLIP, "", false));
      signature.add(Parameter(String("1 1 1 1 1 1 1 1 1"), "kernel", false));
      signature.add(Parameter(String("replace"), "mode", false));

      return add_defaults( signature );
   }
};

} } } }

#endif