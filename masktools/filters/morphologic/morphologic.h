#ifndef __Mt_MorphologicFilter_H__
#define __Mt_MorphologicFilter_H__

#include "../../common/base/filter.h"
#include "../../../common/parser/parser.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Morphologic {

typedef void (Processor)(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int nMaxDeviation, const int *pCoordinates, int nCoordinates, int nWidth, int nHeight);
typedef void (Processor16)(uint16_t *pDst, ptrdiff_t nDstPitch, const uint16_t *pSrc, ptrdiff_t nSrcPitch, int nMaxDeviation, const int *pCoordinates, int nCoordinates, int nWidth, int nHeight);
typedef void (Processor32)(float *pDst, ptrdiff_t nDstPitch, const float *pSrc, ptrdiff_t nSrcPitch, float nMaxDeviation, const int *pCoordinates, int nCoordinates, int nWidth, int nHeight);

class MorphologicFilter : public MaskTools::Filter
{
    int nMaxDeviations[4];
    float nMaxDeviations_f[4];
    int *coordinates_list;
    int coordinates_count;

    MorphologicFilter(const MorphologicFilter &filter);

    int bits_per_pixel;
    bool isStacked;

protected:

    ProcessorList<Processor> processors;
    ProcessorList<Processor16> processors16;
    ProcessorList<Processor32> processors32;

    virtual void process(int n, const Plane<uint8_t> &dst, int nPlane, const ::Filtering::Frame<const uint8_t> frames[4], const Constraint constraints[4]) override
    {
      UNUSED(n);
      if (bits_per_pixel == 8) {
        processors.best_processor(constraints[nPlane])(dst.data(), dst.pitch(),
          frames[0].plane(nPlane).data(), frames[0].plane(nPlane).pitch(),
          nMaxDeviations[nPlane], coordinates_list, coordinates_count, dst.width(), dst.height());
      }
      else if(bits_per_pixel <= 16) {
        processors16.best_processor(constraints[nPlane])(reinterpret_cast<uint16_t*>((uint8_t*)dst.data()), dst.pitch() / sizeof(uint16_t), /* /2: word sized */
          reinterpret_cast<const uint16_t*>((const uint8_t*)(frames[0].plane(nPlane).data())), frames[0].plane(nPlane).pitch() / sizeof(uint16_t),
          nMaxDeviations[nPlane], coordinates_list, coordinates_count, dst.width(), dst.height());
      }
      else {
        processors32.best_processor(constraints[nPlane])((float *)dst.data(), dst.pitch() / sizeof(float),
          (float *)frames[0].plane(nPlane).data(), frames[0].plane(nPlane).pitch() / sizeof(float),
          nMaxDeviations_f[nPlane], coordinates_list, coordinates_count, dst.width(), dst.height());
      }
    }

    void FillCoordinates(const String &coordinates)
    {
        auto coeffs = Parser::getDefaultParser().parse(coordinates, Parser::SYMBOL_SEPARATORS_COORD).getExpression();
        coordinates_count = coeffs.size();
        coordinates_list = new int[coordinates_count];
        int i = 0;

        while (!coeffs.empty())
        {
            coordinates_list[i++] = int(coeffs.front().getValue(0, 0, 0));
            coeffs.pop_front();
        }
    }

public:
    MorphologicFilter(const Parameters &parameters, CpuFlags cpuFlags) : MaskTools::Filter(parameters, FilterProcessingType::CHILD, (CpuFlags)cpuFlags), coordinates_list(NULL), coordinates_count(0)
    {
      isStacked = parameters["stacked"].toBool();
      bits_per_pixel = nBitDepth;

      if (isStacked && bits_per_pixel != 8) {
        error = "Stacked specified for a non-8 bit clip";
        return;
      }
      
      if (isStacked)
        bits_per_pixel = 16;

      bool isFloat = bits_per_pixel == 32;
      bool fullscale = isRGB;
      String scalemode = parameters["paramscale"].toString();

      int thY, thC;
      float thY_f, thC_f;

      const char *errortxt = "invalid parameter: paramscale. Use i8, i10, i12, i14, i16, f32 for scale or none/empty to disable scaling";

      // defaults
      thY_f = thC_f = 1.0;
      thY = thC = (1 << bits_per_pixel) - 1;

      // Y threshold
      if (parameters["thY"].is_defined()) {
        thY_f = (float)parameters["thY"].toFloat();
        if (!ScaleParam(scalemode, thY_f, bits_per_pixel, thY_f, thY, fullscale, false, false))
        {
          error = errortxt;
          return;
        }
      }

      // chroma threshold
      if (parameters["thC"].is_defined()) {
        thC_f = (float)parameters["thC"].toFloat();
        // ScaleParam chroma = false here intentionally: value is not a chroma value but a difference
        if (!ScaleParam(scalemode, thC_f, bits_per_pixel, thC_f, thC, fullscale, false, false))
        {
          error = errortxt;
          return;
        }
      }

      if (isFloat) {
        nMaxDeviations_f[0] = thY_f;
        nMaxDeviations_f[1] = nMaxDeviations_f[2] = thC_f;
        nMaxDeviations_f[3] = thY_f; // no separate parameter for alpha
      }
      else {
        nMaxDeviations[0] = thY;
        nMaxDeviations[1] = nMaxDeviations[2] = thC;
        nMaxDeviations[3] = thY; // no separate parameter for alpha
      }

    }

    ~MorphologicFilter()
    {
        delete[] coordinates_list;
        coordinates_list = NULL;
    }

    InputConfiguration &input_configuration() const override { return OneFrame(); }
};

} } } } // namespace Morphologic, Filters, MaskTools, Filtering

#endif