#ifndef __Mt_MorphologicFilter16_H__
#define __Mt_MorphologicFilter16_H__

#if 0
#include "../../common/base/filter.h"
#include "../../../common/parser/parser.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Morphologic16 {

typedef void (StackedProcessor)(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int nMaxDeviation, const int *pCoordinates, int nCoordinates, int nWidth, int nHeight, int nOrigHeight);
typedef void (Processor16)(uint16_t *pDst, ptrdiff_t nDstPitch, const uint16_t *pSrc, ptrdiff_t nSrcPitch, int nMaxDeviation, const int *pCoordinates, int nCoordinates, int nWidth, int nHeight, int nOrigHeight);

class MorphologicFilter16 : public MaskTools::Filter
{
   int nMaxDeviations[3];
   int *coordinates_list, coordinates_count;

   MorphologicFilter16(const MorphologicFilter16 &filter);

protected:

   ProcessorList<StackedProcessor> stackedProcessors;
   ProcessorList<Processor16> processors16;

   virtual void process(int n, const Plane<uint8_t> &dst, int nPlane, const ::Filtering::Frame<const uint8_t> frames[3], const Constraint constraints[3]) override
    {
        UNUSED(n);
        if (parameters["stacked"].toBool()) {
            stackedProcessors.best_processor(constraints[nPlane])(dst.data(), dst.pitch(), 
                frames[0].plane(nPlane).data(), frames[0].plane(nPlane).pitch(), 
                nMaxDeviations[nPlane], coordinates_list, coordinates_count, dst.width(), dst.height() / 2, dst.origheight()); // stacked: /2
        }
        else {
            processors16.best_processor(constraints[nPlane])(reinterpret_cast<uint16_t*>((uint8_t*)dst.data()), dst.pitch() / 2, /* /2: word sized */
                reinterpret_cast<const uint16_t*>((const uint8_t*)(frames[0].plane(nPlane).data())), frames[0].plane(nPlane).pitch() / 2, 
                nMaxDeviations[nPlane], coordinates_list, coordinates_count, dst.width(), dst.height(), dst.origheight());
        }
    }

   void FillCoordinates(const String &coordinates)
   {
      auto coeffs = Parser::getDefaultParser().parse( coordinates, Parser::SYMBOL_SEPARATORS_COORD).getExpression();
      coordinates_count = coeffs.size();
      coordinates_list = new int[coordinates_count];
      int i = 0;

      while ( !coeffs.empty() )
      {
         coordinates_list[i++] = int( coeffs.front().getValue(0, 0, 0) );
         coeffs.pop_front();
      }
   }

public:
   MorphologicFilter16(const Parameters &parameters) : MaskTools::Filter( parameters, FilterProcessingType::CHILD ), coordinates_list( NULL ), coordinates_count( 0 )
   {
     bool isStacked = parameters["stacked"].toBool();
     int bits_per_pixel = bit_depths[C];

     if (isStacked && bits_per_pixel != 8) {
       error = "Stacked specified for a non-8 bit clip";
       return;
     }
     if (!isStacked && bits_per_pixel == 8) {
       error = "8 bit clip needs stacked=true";
       return;
     }
     if (bits_per_pixel == 32) {
       error = "32 bit float clip is not supported yet";
       return;
     }
     
     int max_pixel_value = isStacked ? 65535 : ((1 << bits_per_pixel) - 1);

     nMaxDeviations[0] = clip<int, int>( parameters["thY"].toInt(), 0, max_pixel_value);
     nMaxDeviations[1] = clip<int, int>( parameters["thC"].toInt(), 0, max_pixel_value);
     nMaxDeviations[2] = clip<int, int>( parameters["thC"].toInt(), 0, max_pixel_value);
   }

   ~MorphologicFilter16()
   {
      if ( coordinates_list )
         delete[] coordinates_list;
      coordinates_list = NULL;
   }

   InputConfiguration &input_configuration() const override { return OneFrame(); }
};

} } } } // namespace Morphologic, Filters, MaskTools, Filtering
#endif
#endif