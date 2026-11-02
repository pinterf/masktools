#ifndef __Mt_Lutsx_H__
#define __Mt_Lutsx_H__

#include "../../../common/base/filter.h"
#include "../../../../common/parser/parser.h"

#include "../functions.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Lut { namespace SpatialExtended {

typedef void(Processor)(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch, const uint8_t *pSrc2, ptrdiff_t nSrc2Pitch, const uint8_t *lut, const int *pCoordinates, int nCoordinates, int nWidth, int nHeight, const String &mode1, const String &mode2);
typedef void(ProcessorCtx)(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch, const uint8_t *pSrc2, ptrdiff_t nSrc2Pitch, Parser::Context *ctx, const int *pCoordinates, int nCoordinates, int nWidth, int nHeight, const String &mode1, const String &mode2);
typedef void(ProcessorCtx16)(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch, const uint8_t *pSrc2, ptrdiff_t nSrc2Pitch, Parser::Context *ctx, const int *pCoordinates, int nCoordinates, int nWidth, int nHeight, const String &mode1, const String &mode2, int bits_per_pixel);
typedef void(ProcessorCtx32)(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch, const uint8_t *pSrc2, ptrdiff_t nSrc2Pitch, Parser::Context *ctx, const int *pCoordinates, int nCoordinates, int nWidth, int nHeight, const String &mode1, const String &mode2, bool chroma);

extern Processor *processors_array[NUM_MODES][NUM_MODES];
extern ProcessorCtx *processors_realtime_8_array[NUM_MODES][NUM_MODES];
extern ProcessorCtx16 *processors_realtime_16_array[NUM_MODES][NUM_MODES];
extern ProcessorCtx32 *processors_realtime_32_array[NUM_MODES][NUM_MODES];

class Lutsx : public MaskTools::Filter
{
   std::pair<bool, uint8_t*> luts[4+1] = {};

   int *pCoordinates = nullptr;
   int nCoordinates;

   ProcessorList<Processor> processors;

   ProcessorList<ProcessorCtx> processorsCtx;
   ProcessorList<ProcessorCtx16> processorsCtx16;
   ProcessorList<ProcessorCtx32> processorsCtx32;

   // for realtime
   std::deque<Filtering::Parser::Symbol> *parsed_expressions[4];

   int bits_per_pixel;
   bool realtime;
   String scale_inputs;
   int clamp_float_i;

   String mode1, mode2;

   void FillCoordinates(const String &coordinates)
   {
      auto coeffs = Parser::getDefaultParser().parse( coordinates, Parser::SYMBOL_SEPARATORS_COORD).getExpression();
      nCoordinates = coeffs.size();
      pCoordinates = new int[nCoordinates];
      int i = 0;

      while ( !coeffs.empty() )
      {
         pCoordinates[i++] = int( coeffs.front().getValue(0, 0, 0) );
         coeffs.pop_front();
      }
   }

   static uint8_t *calculateLut(const std::deque<Filtering::Parser::Symbol> &expr, String scale_inputs, int clamp_float, int& compute_error) {
       Parser::Context ctx(expr, scale_inputs, clamp_float);
       uint8_t *lut = new uint8_t[256 * 256 * 256];

       for ( int x = 0; x < 256; x++ ) {
           for ( int y = 0; y < 256; y++ ) {
               for ( int z = 0; z < 256; z++ ) {
                   lut[(z<<16)+(x<<8)+y] = ctx.compute_byte_xyz(x, y, z);  // ZXY order!
               }
           }
       }

       // problem if compute_error != Parser::Context::compute_error_t::CE_NONE
       compute_error = ctx.get_compute_error();

       return lut;
   }

protected:
    virtual void process(int n, const Plane<uint8_t> &dst, int nPlane, const Filtering::Frame<const uint8_t> frames[4], const Constraint constraints[4]) override
    {
        UNUSED(n);
        if (realtime) {
          // thread safety
          Parser::Context ctx(*parsed_expressions[nPlane], scale_inputs, clamp_float_i);

          if (bits_per_pixel == 8) {
            processorsCtx.best_processor(constraints[nPlane])(dst.data(), dst.pitch(),
              frames[0].plane(nPlane).data(), frames[0].plane(nPlane).pitch(),
              frames[1].plane(nPlane).data(), frames[1].plane(nPlane).pitch(),
              &ctx, pCoordinates, nCoordinates, dst.width(), dst.height(), mode1, mode2);
          }
          else if (bits_per_pixel <= 16) {
            processorsCtx16.best_processor(constraints[nPlane])(dst.data(), dst.pitch(),
              frames[0].plane(nPlane).data(), frames[0].plane(nPlane).pitch(),
              frames[1].plane(nPlane).data(), frames[1].plane(nPlane).pitch(),
              &ctx, pCoordinates, nCoordinates, dst.width(), dst.height(), mode1, mode2, bits_per_pixel);
          }
          else {
            const bool chroma = ((nPlane == 1 || nPlane == 2) && !planes_isRGB[C]);
            processorsCtx32.best_processor(constraints[nPlane])(dst.data(), dst.pitch(),
              frames[0].plane(nPlane).data(), frames[0].plane(nPlane).pitch(),
              frames[1].plane(nPlane).data(), frames[1].plane(nPlane).pitch(),
              &ctx, pCoordinates, nCoordinates, dst.width(), dst.height(), mode1, mode2, chroma);
          }
        }
        else if (bits_per_pixel == 8) {
          processors.best_processor(constraints[nPlane])(dst.data(), dst.pitch(),
            frames[0].plane(nPlane).data(), frames[0].plane(nPlane).pitch(),
            frames[1].plane(nPlane).data(), frames[1].plane(nPlane).pitch(),
            luts[nPlane].second, pCoordinates, nCoordinates, dst.width(), dst.height(), mode1, mode2);
        }
    }

public:
   Lutsx(const Parameters &parameters, CpuFlags cpuFlags) : MaskTools::Filter( parameters, FilterProcessingType::INPLACE, (CpuFlags)cpuFlags)
   {
     for (int i = 0; i < 4; i++) {
       parsed_expressions[i] = nullptr;
     }

     bits_per_pixel = bit_depths[C];
     realtime = parameters["realtime"].toBool();
     scale_inputs = parameters["scale_inputs"].toString();
     if (!checkValidScaleInputs(scale_inputs, error))
       return; // error message filled
     bool clamp_float = parameters["clamp_float"].toBool();
     bool clamp_float_UV = parameters["clamp_float_UV"].toBool();
     clamp_float_i = clamp_float ? (clamp_float_UV ? 2 : 1) : 0;

     if (bits_per_pixel > 8)
       realtime = true;

      static const char *expr_strs[] = { "yExpr", "uExpr", "vExpr", "aExpr" };

      for (int i = 0; i < 4+1; ++i) {
          luts[i].first = false;
          luts[i].second = nullptr;
      }

      Parser::Parser parser = Parser::getDefaultParser().addSymbol(Parser::Symbol::X).addSymbol(Parser::Symbol::Y).addSymbol(Parser::Symbol::Z);

      /* compute the luts */
      for ( int i = 0; i < 4; i++ )
      {
          if (operators[i] != PROCESS) {
              continue;
          }

          if (parameters[expr_strs[i]].undefinedOrEmptyString() && parameters["expr"].undefinedOrEmptyString()) {
              operators[i] = NONE; //inplace
              continue;
          }

          bool customExpressionDefined = false;
          if (parameters[expr_strs[i]].is_defined()) {
            parser.parse_strict(parameters[expr_strs[i]].toString(), Parser::SYMBOL_SEPARATORS);
            customExpressionDefined = true;
          }
          else
            parser.parse_strict(parameters["expr"].toString(), Parser::SYMBOL_SEPARATORS);

          auto err_pos = parser.getErrorPos();
          if (err_pos >= 0) {
            error = "Error at position " + std::to_string(1 + err_pos) + ": cannot convert to number: " + parser.getFailedSymbol();
            return;
          }

          // for check:
          Parser::Context ctx(parser.getExpression(), scale_inputs, clamp_float);

          if (!ctx.check())
          {
            error = "invalid expression in the lut";
            return;
          }

          if (realtime) {
            parsed_expressions[i] = new std::deque<Parser::Symbol>(parser.getExpression());
            continue;
          }

          int compute_error = Parser::Context::compute_error_t::CE_NONE;

          if (customExpressionDefined) {
              luts[i].first = true;
              luts[i].second = calculateLut(parser.getExpression(), scale_inputs, clamp_float, /*ref*/ compute_error);
          }
          else {
              if (luts[4].second == nullptr) {
                  luts[4].first = true;
                  luts[4].second = calculateLut(parser.getExpression(), scale_inputs, clamp_float, /*ref*/ compute_error);
              }
              luts[i].second = luts[4].second;
          }

          if (compute_error != Parser::Context::compute_error_t::CE_NONE) {
            error = "invalid expression in the lut code = " + std::to_string(compute_error);
            return;
          }
      }

      /* get the pixels list */
      FillCoordinates( parameters["pixels"].toString() );

      /* choose the mode */
      mode1 = parameters["mode"].toString();
      mode2 = parameters["mode2"].toString();
      if (realtime) {
        const int m1 = ModeToInt(mode1), m2 = ModeToInt(mode2);
        if (bits_per_pixel == 8) processorsCtx.push_back(processors_realtime_8_array[m1][m2]);
        else if (bits_per_pixel <= 16) processorsCtx16.push_back(processors_realtime_16_array[m1][m2]);
        else processorsCtx32.push_back(processors_realtime_32_array[m1][m2]);
      }
      else {
        processors.push_back(processors_array[ModeToInt(mode1)][ModeToInt(mode2)]);
      }
   }

   ~Lutsx()
   {
       delete[] pCoordinates;
       for (int i = 0; i < 4+1; ++i) {
           if (luts[i].first) {
               delete[] luts[i].second;
           }
       }
       for (int i = 0; i < 4; i++) {
         delete parsed_expressions[i];
       }
   }

   InputConfiguration &input_configuration() const override { return InPlaceThreeFrame(); }

   static Signature filter_signature()
   {
      Signature signature = "mt_lutsx";

      signature.add( Parameter( TYPE_CLIP, "", false) );
      signature.add( Parameter( TYPE_CLIP, "", false) );
      signature.add( Parameter( TYPE_CLIP, "", false) );
      signature.add( Parameter( String( "average" ), "mode", false) );
      signature.add( Parameter( String( "none" ), "mode2", false) );
      signature.add( Parameter( String( "" ), "pixels", false) );
      signature.add( Parameter( String( "y" ), "expr", false) );
      signature.add( Parameter( String( "y" ), "yExpr", false) );
      signature.add( Parameter( String( "y" ), "uExpr", false) );
      signature.add( Parameter( String( "y" ), "vExpr", false) );

      add_defaults( signature );

      signature.add(Parameter(false, "realtime", false));
      signature.add(Parameter(String("y"), "aExpr", false));
      signature.add(Parameter(String("none"), "scale_inputs", false));
      signature.add(Parameter(false, "clamp_float", false));
      signature.add(Parameter(false, "clamp_float_UV", false));
      return signature;
   }
};

} } } } }

#endif