#ifndef __Mt_Convolution_H__
#define __Mt_Convolution_H__

#include "../../common/base/filter.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Convolution {

// void* parameters can be either int* or float*
typedef void(Processor)(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, void *, void *, void *, int nHorizontal, int nVertical, int nWidth, int nHeight);
typedef void(Processor16)(uint16_t *pDst, ptrdiff_t nDstPitch, const uint16_t *pSrc, ptrdiff_t nSrcPitch, void *, void *, void *, int nHorizontal, int nVertical, int nWidth, int nHeight);
typedef void(Processor32)(float *pDst, ptrdiff_t nDstPitch, const float *pSrc, ptrdiff_t nSrcPitch, void *, void *, void *, int nHorizontal, int nVertical, int nWidth, int nHeight);

extern Processor *convolution_f_s_c;
extern Processor *convolution_i_s_c;
extern Processor *convolution_f_m_c;
extern Processor *convolution_i_m_c;

extern Processor16 *convolution_f_s_10_c;
extern Processor16 *convolution_i_s_10_c;
extern Processor16 *convolution_f_m_10_c;
extern Processor16 *convolution_i_m_10_c;

extern Processor16 *convolution_f_s_12_c;
extern Processor16 *convolution_i_s_12_c;
extern Processor16 *convolution_f_m_12_c;
extern Processor16 *convolution_i_m_12_c;

extern Processor16 *convolution_f_s_14_c;
extern Processor16 *convolution_i_s_14_c;
extern Processor16 *convolution_f_m_14_c;
extern Processor16 *convolution_i_m_14_c;

extern Processor16 *convolution_f_s_16_c;
extern Processor16 *convolution_i_s_16_c;
extern Processor16 *convolution_f_m_16_c;
extern Processor16 *convolution_i_m_16_c;

extern Processor32 *convolution_f_s_32_c;
extern Processor32 *convolution_f_m_32_c;

class Convolution : public MaskTools::Filter
{
   int *i_horizontal, *i_vertical;
   float *f_horizontal, *f_vertical;
   int i_total;
   float f_total;
   void *horizontal, *vertical;
   void *total;

   int nHorizontal;
   int nVertical;

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
        horizontal, vertical, total, nHorizontal, nVertical, dst.width(), dst.height());
    }
    else if (bits_per_pixel <= 16) {
      processors16.best_processor(constraints[nPlane])((uint16_t *)dst.data(), dst.pitch(),
        (uint16_t *)frames[0].plane(nPlane).data(), frames[0].plane(nPlane).pitch(),
        horizontal, vertical, total, nHorizontal, nVertical, dst.width(), dst.height());
    }
    else
    {
      processors32.best_processor(constraints[nPlane])((float *)dst.data(), dst.pitch(),
        (float *)frames[0].plane(nPlane).data(), frames[0].plane(nPlane).pitch(),
        horizontal, vertical, total, nHorizontal, nVertical, dst.width(), dst.height());
    }
  }
public:
   Convolution(const Parameters &parameters, CpuFlags cpuFlags) : MaskTools::Filter( parameters, FilterProcessingType::CHILD, (CpuFlags)cpuFlags)
   {
      bits_per_pixel = bit_depths[C];
     
      i_vertical = i_horizontal = NULL;
      f_vertical = f_horizontal = NULL;
      vertical = horizontal = NULL;
      auto hcoeffs = Parser::getDefaultParser().parse(parameters["horizontal"].toString(), Parser::SYMBOL_SEPARATORS).getExpression();
      auto vcoeffs = Parser::getDefaultParser().parse(parameters["vertical"].toString(), Parser::SYMBOL_SEPARATORS).getExpression();
      nHorizontal = hcoeffs.size();
      nVertical = vcoeffs.size();
      
      /* search for float values */
      bool isFloat = false;
      isFloat |= parameters["horizontal"].toString().find(".", 0) != String::npos;
      isFloat |= parameters["vertical"].toString().find(".", 0) != String::npos;
      isFloat |= (bits_per_pixel == 32); // float clip is always using float

      /* create the two arrays */
      if ( isFloat )
      {
         f_vertical = new float[nVertical];
         f_horizontal = new float[nHorizontal];
         f_total = float(parameters["total"].toFloat());
         vertical = f_vertical;
         horizontal = f_horizontal;
         total = parameters["total"].is_defined() ? &f_total : NULL;
      }
      else
      {
         i_vertical = new int[nVertical];
         i_horizontal = new int[nHorizontal];
         i_total = convert<int, double>(parameters["total"].toFloat());
         if ( i_total == 0 )
            i_total = 1;
         vertical = i_vertical;
         horizontal = i_horizontal;
         total = parameters["total"].is_defined() ? &i_total : NULL;
      }

      /* fill them */
      for ( int i = 0; i < nHorizontal; i++ )
      {
         if ( isFloat )
            f_horizontal[i] = float(hcoeffs.front().getValue(0,0,0));
         else
            i_horizontal[i] = convert<int, double>(hcoeffs.front().getValue(0,0,0));

         hcoeffs.pop_front();
      }
      for ( int i = 0; i < nVertical; i++ )
      {
         if ( isFloat )
            f_vertical[i] = float(vcoeffs.front().getValue(0,0,0));
         else
            i_vertical[i] = convert<int, double>(vcoeffs.front().getValue(0,0,0));

         vcoeffs.pop_front();
      }

      for (int i = 0; i < 4; i++)
      {
        if (operators[i] == PROCESS) {
          if (nWidth / width_ratios[i][C] < nHorizontal)
          {
            error = "Plane width should be at least the horizontal element count";
            return;
          }
          if (nHeight / height_ratios[i][C] < nVertical)
          {
            error = "Plane height should be at least the vertical element count";
            return;
          }
        }
      }

      /* adds the processor */
      bool isSaturate = parameters["saturate"].toBool();
      if ( isFloat )
         if ( isSaturate )
           switch (bits_per_pixel) {
           case 8: processors.push_back(Filtering::Processor<Processor>(convolution_f_s_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0)); break;
           case 10: processors16.push_back(Filtering::Processor<Processor16>(convolution_f_s_10_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0)); break;
           case 12: processors16.push_back(Filtering::Processor<Processor16>(convolution_f_s_12_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0)); break;
           case 14: processors16.push_back(Filtering::Processor<Processor16>(convolution_f_s_14_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0)); break;
           case 16: processors16.push_back(Filtering::Processor<Processor16>(convolution_f_s_16_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0)); break;
           case 32: processors32.push_back(Filtering::Processor<Processor32>(convolution_f_s_32_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0)); break;
           }
         else
           switch (bits_per_pixel) {
           case 8:  processors.push_back(Filtering::Processor<Processor>(convolution_f_m_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0)); break;
           case 10:  processors16.push_back(Filtering::Processor<Processor16>(convolution_f_m_10_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0)); break;
           case 12:  processors16.push_back(Filtering::Processor<Processor16>(convolution_f_m_12_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0)); break;
           case 14:  processors16.push_back(Filtering::Processor<Processor16>(convolution_f_m_14_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0)); break;
           case 16:  processors16.push_back(Filtering::Processor<Processor16>(convolution_f_m_16_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0)); break;
           case 32:  processors32.push_back(Filtering::Processor<Processor32>(convolution_f_m_32_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0)); break;
           }
      else
         if ( isSaturate )
           switch (bits_per_pixel) {
           case 8: processors.push_back(Filtering::Processor<Processor>(convolution_i_s_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0)); break;
           case 10: processors16.push_back(Filtering::Processor<Processor16>(convolution_i_s_10_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0)); break;
           case 12: processors16.push_back(Filtering::Processor<Processor16>(convolution_i_s_12_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0)); break;
           case 14: processors16.push_back(Filtering::Processor<Processor16>(convolution_i_s_14_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0)); break;
           case 16: processors16.push_back(Filtering::Processor<Processor16>(convolution_i_s_16_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0)); break;
           }
         else
           switch (bits_per_pixel) {
           case 8: processors.push_back(Filtering::Processor<Processor>(convolution_i_m_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0)); break;
           case 10: processors16.push_back(Filtering::Processor<Processor16>(convolution_i_m_10_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0)); break;
           case 12: processors16.push_back(Filtering::Processor<Processor16>(convolution_i_m_12_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0)); break;
           case 14: processors16.push_back(Filtering::Processor<Processor16>(convolution_i_m_14_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0)); break;
           case 16: processors16.push_back(Filtering::Processor<Processor16>(convolution_i_m_16_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0)); break;
           }
   }

   ~Convolution()
   {
      if ( i_horizontal )
         delete[] i_horizontal;
      if ( i_vertical )
         delete[] i_vertical;
      if ( f_horizontal )
         delete[] f_horizontal;
      if ( f_vertical )
         delete[] f_vertical;
   }

   InputConfiguration &input_configuration() const override { return OneFrame(); }

   static Signature filter_signature()
   {
      Signature signature = "mt_convolution";

      signature.add(Parameter(TYPE_CLIP, "", false));
      signature.add(Parameter(String("1 1 1"), "horizontal", false));
      signature.add(Parameter(String("1 1 1"), "vertical", false));
      signature.add(Parameter(true, "saturate", false));
      signature.add(Parameter(1.0f, "total", false));

      return add_defaults( signature );
   }
};

} } } }

#endif
