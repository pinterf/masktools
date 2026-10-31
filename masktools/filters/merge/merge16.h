#ifndef __Mt_Merge16_H__
#define __Mt_Merge16_H__

#if 0
#include "../../common/base/filter.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Merge16 {

typedef void(Processor16)(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch,
                        const uint8_t *pSrc2, ptrdiff_t nSrc2Pitch, int nWidth, int nHeight, int nOrigHeight);

extern Processor16 *merge16_c_stacked;
extern Processor16 *merge16_luma_420_c_stacked;
extern Processor16 *merge16_luma_422_c_stacked;

extern Processor16 *merge16_sse2_stacked;
extern Processor16 *merge16_sse4_1_stacked;

extern Processor16 *merge16_luma_420_sse2_stacked;
extern Processor16 *merge16_luma_420_ssse3_stacked; 
extern Processor16 *merge16_luma_420_sse4_1_stacked;

extern Processor16 *merge16_luma_422_sse2_stacked;
extern Processor16 *merge16_luma_422_ssse3_stacked;
extern Processor16 *merge16_luma_422_sse4_1_stacked;


#define MAKE_16BIT_EXTERNS(bits_per_pixel) \
extern Processor16 *merge16_##bits_per_pixel##_c; \
extern Processor16 *merge16_luma_420_##bits_per_pixel##_c; \
extern Processor16 *merge16_luma_422_##bits_per_pixel##_c; \
extern Processor16 *merge16_##bits_per_pixel##_sse2; \
extern Processor16 *merge16_##bits_per_pixel##_sse4_1; \
extern Processor16 *merge16_luma_420_##bits_per_pixel##_sse2; \
extern Processor16 *merge16_luma_420_##bits_per_pixel##_ssse3; \
extern Processor16 *merge16_luma_420_##bits_per_pixel##_sse4_1; \
extern Processor16 *merge16_luma_422_##bits_per_pixel##_sse2; \
extern Processor16 *merge16_luma_422_##bits_per_pixel##_ssse3; \
extern Processor16 *merge16_luma_422_##bits_per_pixel##_sse4_1;

MAKE_16BIT_EXTERNS(10)
MAKE_16BIT_EXTERNS(12)
MAKE_16BIT_EXTERNS(14)
MAKE_16BIT_EXTERNS(16)
#undef MAKE_16BIT_EXTERNS

class Merge16 : public MaskTools::Filter
{

   bool use_luma;
   ProcessorList<Processor16> processors16;
   ProcessorList<Processor16> chroma_processors16;

protected:

    virtual void process(int n, const Plane<uint8_t> &dst, int nPlane, const Frame<const uint8_t> frames[3], const Constraint constraints[3]) override
    {
        UNUSED(n);
        if (use_luma && (nPlane > 0)) {
          if (!(width_ratios[1][C] == 1 && height_ratios[1][C] == 1)) {
            // 420 or 422
            chroma_processors16.best_processor(constraints[nPlane])(dst.data(), dst.pitch(),
                    frames[0].plane(nPlane).data(), frames[0].plane(nPlane).pitch(),
                    frames[1].plane(0).data(), frames[1].plane(0).pitch(),
                    dst.width(), dst.height(), dst.origheight());
            }
            else {
                processors16.best_processor(constraints[nPlane])(dst.data(), dst.pitch(),
                    frames[0].plane(nPlane).data(), frames[0].plane(nPlane).pitch(),
                    frames[1].plane(0).data(), frames[1].plane(0).pitch(),
                    dst.width(), dst.height(), dst.origheight());
            }
        }
        else {
            processors16.best_processor(constraints[nPlane])(dst.data(), dst.pitch(),
                frames[0].plane(nPlane).data(), frames[0].plane(nPlane).pitch(),
                frames[1].plane(nPlane).data(), frames[1].plane(nPlane).pitch(),
                dst.width(), dst.height(), dst.origheight());
        }
    }

public:
   Merge16(const Parameters &parameters) : MaskTools::Filter( parameters, FilterProcessingType::INPLACE )
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

     use_luma = parameters["luma"].toBool();

      bool is420 = width_ratios[1][C] == 2 && height_ratios[1][C] == 2;
      bool is422 = width_ratios[1][C] == 2 && height_ratios[1][C] == 1;
     
      // PF: for greyscale: graceful fallback to chromaless operation
      if (plane_counts[C] == 1) {
        use_luma = false;
        operators[1] = operators[2] = NONE;
      }

      // todo: allow 8 bit masks for 10+ bits

      if (use_luma) {
          auto c1 = childs[0]->colorspace();
          auto c2 = childs[1]->colorspace();
          if ((width_ratios[1][c1] != width_ratios[1][c2]) || (height_ratios[1][c1] != height_ratios[1][c2])) {
              error = "clips should have identical colorspace";
              return;
          }

          /* if "luma" is set, we force the chroma processing. Much more handy */
          operators[1] = operators[2] = PROCESS;
          /* no need to change U/V default processing, because of in place filter */
      } else if (operators[1] == PROCESS || operators[2] == PROCESS) {
          auto mask_colorspace = childs[2]->colorspace();
          if (plane_counts[mask_colorspace] == 1) {
              error = "Mask cannot be greyscale when chroma mode is PROCESS";
          }
      }

      if (isStacked) {
          /* add the processors16 */
          processors16.push_back( Filtering::Processor<Processor16>( merge16_c_stacked, Constraint( CPU_NONE, 1, 1, 1, 1 ), 0 ) );
          processors16.push_back( Filtering::Processor<Processor16>( merge16_sse2_stacked, Constraint( CPU_SSE2, 1, 1, 1, 1 ), 1 ) );
          processors16.push_back( Filtering::Processor<Processor16>( merge16_sse4_1_stacked, Constraint( CPU_SSE4_1, 1, 1, 1, 1 ), 2 ) );

          /* add the chroma processors16 */
          if (is420) {
            chroma_processors16.push_back(Filtering::Processor<Processor16>(merge16_luma_420_c_stacked, Constraint(CPU_NONE, 1, 1, 1, 1), 0));
            chroma_processors16.push_back(Filtering::Processor<Processor16>(merge16_luma_420_sse2_stacked, Constraint(CPU_SSE2, 1, 1, 1, 1), 1));
            chroma_processors16.push_back(Filtering::Processor<Processor16>(merge16_luma_420_ssse3_stacked, Constraint(CPU_SSSE3, 1, 1, 1, 1), 2));
            chroma_processors16.push_back(Filtering::Processor<Processor16>(merge16_luma_420_sse4_1_stacked, Constraint(CPU_SSE4_1, 1, 1, 1, 1), 3));
          }
          else { // 422
            chroma_processors16.push_back(Filtering::Processor<Processor16>(merge16_luma_422_c_stacked, Constraint(CPU_NONE, 1, 1, 1, 1), 0));
            chroma_processors16.push_back(Filtering::Processor<Processor16>(merge16_luma_422_sse2_stacked, Constraint(CPU_SSE2, 1, 1, 1, 1), 1));
            chroma_processors16.push_back(Filtering::Processor<Processor16>(merge16_luma_422_ssse3_stacked, Constraint(CPU_SSSE3, 1, 1, 1, 1), 2));
            chroma_processors16.push_back(Filtering::Processor<Processor16>(merge16_luma_422_sse4_1_stacked, Constraint(CPU_SSE4_1, 1, 1, 1, 1), 3));
          }
      } else {
#define MAKE_16BIT_PROCESSORS(bits_per_pixel) \
          /* add the processors16 */ \
          processors16.push_back( Filtering::Processor<Processor16>( merge16_##bits_per_pixel##_c, Constraint( CPU_NONE, 1, 1, 1, 1 ), 0 ) ); \
          processors16.push_back( Filtering::Processor<Processor16>( merge16_##bits_per_pixel##_sse2, Constraint( CPU_SSE2, 1, 1, 1, 1 ), 1 ) ); \
          processors16.push_back( Filtering::Processor<Processor16>( merge16_##bits_per_pixel##_sse4_1, Constraint( CPU_SSE4_1, 1, 1, 1, 1 ), 2 ) ); \
          /* add the chroma processors16 */ \
          /* used only for 420 and 422  */ \
          if (is420) { \
            chroma_processors16.push_back(Filtering::Processor<Processor16>(merge16_luma_420_##bits_per_pixel##_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0)); \
            chroma_processors16.push_back(Filtering::Processor<Processor16>(merge16_luma_420_##bits_per_pixel##_sse2, Constraint(CPU_SSE2, 1, 1, 1, 1), 1)); \
            chroma_processors16.push_back(Filtering::Processor<Processor16>(merge16_luma_420_##bits_per_pixel##_ssse3, Constraint(CPU_SSSE3, 1, 1, 1, 1), 2)); \
            chroma_processors16.push_back(Filtering::Processor<Processor16>(merge16_luma_420_##bits_per_pixel##_sse4_1, Constraint(CPU_SSE4_1, 1, 1, 1, 1), 3)); \
          } \
          else { /* 422 */ \
            chroma_processors16.push_back(Filtering::Processor<Processor16>(merge16_luma_422_##bits_per_pixel##_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0)); \
            chroma_processors16.push_back(Filtering::Processor<Processor16>(merge16_luma_422_##bits_per_pixel##_sse2, Constraint(CPU_SSE2, 1, 1, 1, 1), 1)); \
            chroma_processors16.push_back(Filtering::Processor<Processor16>(merge16_luma_422_##bits_per_pixel##_ssse3, Constraint(CPU_SSSE3, 1, 1, 1, 1), 2)); \
            chroma_processors16.push_back(Filtering::Processor<Processor16>(merge16_luma_422_##bits_per_pixel##_sse4_1, Constraint(CPU_SSE4_1, 1, 1, 1, 1), 3)); \
          }

          switch (bit_depths[C]) {
          case 10: MAKE_16BIT_PROCESSORS(10);
            break;
          case 12: MAKE_16BIT_PROCESSORS(12);
            break;
          case 14: MAKE_16BIT_PROCESSORS(14);
            break;
          case 16: MAKE_16BIT_PROCESSORS(16);
            break;
          }
#undef MAKE_PROCESSORS
      }
   }

   InputConfiguration &input_configuration() const { return InPlaceThreeFrame(); }

   static Signature filter_signature()
   {
      Signature signature = "mt_merge16";

      signature.add( Parameter( TYPE_CLIP, "" ) );
      signature.add( Parameter( TYPE_CLIP, "" ) );
      signature.add( Parameter( TYPE_CLIP, "" ) );
      signature.add( Parameter( false, "luma" ) );
      signature.add( Parameter( false, "stacked" ) );

      return add_defaults( signature );
   }
};

} } } } // namespace Merge, Filters, MaskTools, Filtering
#endif 0

#endif