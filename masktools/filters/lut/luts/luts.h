#ifndef __Mt_Luts_H__
#define __Mt_Luts_H__

#include <cstdint>
#include "../../../common/base/filter.h"
#include "../../../../common/parser/parser.h"

#include "../functions.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Lut { namespace Spatial {

typedef void(Processor)(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, const uint8_t lut[65536], const float lut_w[65536], Parser::Context *ctx, Parser::Context *ctx_w, const int *pCoordinates, int nCoordinates, int nWidth, int nHeight, const String &mode);
typedef void(Processor32)(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, const uint8_t lut[65536], const float lut_w[65536], Parser::Context *ctx, Parser::Context *ctx_w, const int *pCoordinates, int nCoordinates, int nWidth, int nHeight, const String &mode, bool chroma);

// lut 8-16
extern Processor *processors_array[NUM_MODES];
extern Processor *processors_10_array[NUM_MODES];
extern Processor *processors_12_array[NUM_MODES];
extern Processor *processors_14_array[NUM_MODES];
extern Processor *processors_16_array[NUM_MODES];

extern Processor *processors_weight_array[NUM_MODES];
extern Processor *processors_weight_10_array[NUM_MODES];
extern Processor *processors_weight_12_array[NUM_MODES];
extern Processor *processors_weight_14_array[NUM_MODES];
extern Processor *processors_weight_16_array[NUM_MODES];
// realtime 8-32
extern Processor *processors_realtime_8_array[NUM_MODES];
extern Processor *processors_realtime_10_array[NUM_MODES];
extern Processor *processors_realtime_12_array[NUM_MODES];
extern Processor *processors_realtime_14_array[NUM_MODES];
extern Processor *processors_realtime_16_array[NUM_MODES];
extern Processor32 *processors_realtime_32_array[NUM_MODES];

extern Processor *processors_weight_realtime_8_array[NUM_MODES];
extern Processor *processors_weight_realtime_10_array[NUM_MODES];
extern Processor *processors_weight_realtime_12_array[NUM_MODES];
extern Processor *processors_weight_realtime_14_array[NUM_MODES];
extern Processor *processors_weight_realtime_16_array[NUM_MODES];
extern Processor32 *processors_weight_realtime_32_array[NUM_MODES];

class Luts : public MaskTools::Filter
{
   int *pCoordinates = nullptr;
   int nCoordinates;

   ProcessorList<Processor> processors;
   ProcessorList<Processor> processors_weight;
   ProcessorList<Processor32> processors32;
   ProcessorList<Processor32> processors32_weight;

   // for realtime
   std::deque<Filtering::Parser::Symbol> *parsed_expressions[4];
   std::deque<Filtering::Parser::Symbol> *parsed_expressions_w[4];

   int bits_per_pixel;
   bool realtime;
   String scale_inputs;
   int clamp_float_i;

   String mode;

   // re-use repeated luts, like in lutxyz
   struct Lut {
     bool used;
     uint8_t *ptr;
   };

   Lut luts[4+1];

   struct Lut_w {
     bool used;
     float *ptr;
   };

   Lut_w luts_weight[4+1]; // max planes + 1

   static uint8_t *calculateLut(const std::deque<Filtering::Parser::Symbol> &expr, int bits_per_pixel, String scale_inputs, int clamp_float) {
     Parser::Context ctx(expr, scale_inputs, clamp_float);
     int pixelsize = bits_per_pixel == 8 ? 1 : 2; // byte / uint16_t
     size_t buffer_size = ((size_t)1 << bits_per_pixel) * ((size_t)1 << bits_per_pixel) *pixelsize;
     uint8_t *lut = new (std::nothrow) uint8_t[buffer_size];
     if (!lut)
       return nullptr;

     const int size = 1 << bits_per_pixel;

     switch (bits_per_pixel) {
     case 8:
       for (int x = 0; x < 256; x++)
         for (int y = 0; y < 256; y++)
           lut[(x << 8) + y] = ctx.compute_byte_xy(x, y);
       break;
     case 10:
       for (int x = 0; x < size; x++)
         for (int y = 0; y < size; y++)
           reinterpret_cast<uint16_t *>(lut)[(x << 10) + y] = ctx.compute_word_xy<10>(x, y);
       break;
     case 12:
       for (int x = 0; x < size; x++)
         for (int y = 0; y < size; y++)
           reinterpret_cast<uint16_t *>(lut)[(x << 12) + y] = ctx.compute_word_xy<12>(x, y);
       break;
     case 14:
       for (int x = 0; x < size; x++)
         for (int y = 0; y < size; y++)
           reinterpret_cast<uint16_t *>(lut)[(x << 14) + y] = ctx.compute_word_xy<14>(x, y);
       break;
     case 16:
       // 64bit only
       for (int x = 0; x < 65536; x++)
         for (int y = 0; y < 65536; y++)
           reinterpret_cast<uint16_t *>(lut)[((size_t)x << 16) + y] = ctx.compute_word_xy<16>(x, y);
       break;
     }
     return lut;
   }

   // weight luts: float content
   template<int bits_per_pixel>
   static float *calculateLut_w(const std::deque<Filtering::Parser::Symbol> &expr, String scale_inputs, int clamp_float) {
     Parser::Context ctx(expr, scale_inputs, clamp_float);
     const int size = 1 << bits_per_pixel;

     size_t buffer_size = ((size_t)size) * ((size_t)size);
     float *lut = new (std::nothrow) float[buffer_size];
     if (!lut)
       return nullptr;

     for (int x = 0; x < size; x++)
       for (int y = 0; y < size; y++)
         lut[(x << bits_per_pixel) + y] = ctx.compute_float_xy_intinput<bits_per_pixel>(x, y);
     return lut;
   }


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

protected:
    virtual void process(int n, const Plane<uint8_t> &dst, int nPlane, const Filtering::Frame<const uint8_t> frames[4], const Constraint constraints[4]) override
    {
        UNUSED(n);
        if (realtime) {
          // thread safety
          Parser::Context ctx(*parsed_expressions[nPlane], scale_inputs, clamp_float_i);
          if (!parsed_expressions_w[nPlane]) {
            // no weights
            if (bits_per_pixel <= 16) {
              processors.best_processor(constraints[nPlane])(dst.data(), dst.pitch(),
                frames[0].plane(nPlane).data(), frames[0].plane(nPlane).pitch(),
                nullptr, nullptr, &ctx, nullptr, pCoordinates, nCoordinates, dst.width(), dst.height(), mode);
            }
            else {
              const bool chroma = ((nPlane == 1 || nPlane == 2) && !planes_isRGB[C]);
              processors32.best_processor(constraints[nPlane])(dst.data(), dst.pitch(),
                frames[0].plane(nPlane).data(), frames[0].plane(nPlane).pitch(),
                nullptr, nullptr, &ctx, nullptr, pCoordinates, nCoordinates, dst.width(), dst.height(), mode, chroma);
            }
          }
          else {
            Parser::Context ctx_w(*parsed_expressions_w[nPlane]);
            if (bits_per_pixel <= 16) {
              processors_weight.best_processor(constraints[nPlane])(dst.data(), dst.pitch(),
                frames[0].plane(nPlane).data(), frames[0].plane(nPlane).pitch(),
                nullptr, nullptr, &ctx, &ctx_w, pCoordinates, nCoordinates, dst.width(), dst.height(), mode);
            }
            else {
              const bool chroma = ((nPlane == 1 || nPlane == 2) && !planes_isRGB[C]);
              processors32_weight.best_processor(constraints[nPlane])(dst.data(), dst.pitch(),
                frames[0].plane(nPlane).data(), frames[0].plane(nPlane).pitch(),
                nullptr, nullptr, &ctx, &ctx_w, pCoordinates, nCoordinates, dst.width(), dst.height(), mode, chroma);
            }
          }
        }
        else {
          if (!luts_weight[nPlane].ptr) {
            // no weights
            processors.best_processor(constraints[nPlane])(dst.data(), dst.pitch(),
              frames[0].plane(nPlane).data(), frames[0].plane(nPlane).pitch(),
              luts[nPlane].ptr, nullptr, nullptr, nullptr, pCoordinates, nCoordinates, dst.width(), dst.height(), mode);
          }
          else {
            processors_weight.best_processor(constraints[nPlane])(dst.data(), dst.pitch(),
              frames[0].plane(nPlane).data(), frames[0].plane(nPlane).pitch(),
              luts[nPlane].ptr, luts_weight[nPlane].ptr, nullptr, nullptr, pCoordinates, nCoordinates, dst.width(), dst.height(), mode);
          }
        }
    }

public:
   Luts(const Parameters &parameters, CpuFlags cpuFlags) : MaskTools::Filter(parameters, FilterProcessingType::INPLACE, (CpuFlags)cpuFlags)
   {
     for (int i = 0; i < 4; i++) {
       parsed_expressions[i] = nullptr;
       parsed_expressions_w[i] = nullptr;
     }

     for (int i = 0; i < 4+1; ++i) {
       luts[i].used = false;
       luts[i].ptr = nullptr;
       luts_weight[i].used = false;
       luts_weight[i].ptr = nullptr;
     }

     static const char *expr_strs[] = { "yExpr", "uExpr", "vExpr", "aExpr" };
     static const char *expr_strs_w[] = { "ywExpr", "uwExpr", "vwExpr", "awExpr" };

     Parser::Parser parser = Parser::getDefaultParser().addSymbol(Parser::Symbol::X).addSymbol(Parser::Symbol::Y);

     bits_per_pixel = bit_depths[C];
     realtime = parameters["realtime"].toBool();
     scale_inputs = parameters["scale_inputs"].toString();
     if (!checkValidScaleInputs(scale_inputs, error))
       return; // error message filled
     bool clamp_float = parameters["clamp_float"].toBool();
     bool clamp_float_UV = parameters["clamp_float_UV"].toBool();
     clamp_float_i = clamp_float ? (clamp_float_UV ? 2 : 1) : 0;

     // same as in lut_xy
     // 14, 16 bit and float: default realtime, 
     // hardcore users on x64 with 8GB memory and plenty of time can override 16 bit lutxy with realtime=false :)
     // lut sizes
     // 10 bits: 2 MBytes (2*1024*1024) per expression 
     // 12 bits: 32 MBytes (2*4096*4096) per expression 
     // 14 bits: 512 MBytes (2*16384*16384) per expression 
     // 16 bits: 8 GBytes (2*65536*65536) per expression 
     if (bits_per_pixel >= 14 && !parameters["realtime"].is_defined()) {
       realtime = true;
     }

     if (bits_per_pixel == 32)
       realtime = true;

     if (bits_per_pixel == 16) {
       if ((uint64_t)std::numeric_limits<size_t>::max() <= 0xFFFFFFFFull) {
         realtime = true; // not even possible a real 16 bit lutxy on 32 bit environment
       }
     }

     bool hasWeights = false;

     /* compute the luts */
     for (int i = 0; i < 4; i++)
     {
       if (operators[i] != PROCESS) {
         continue;
       }

       if (parameters[expr_strs[i]].undefinedOrEmptyString() && parameters["expr"].undefinedOrEmptyString()) {
         operators[i] = NONE; //inplace
         continue;
       }

       //----- main expression
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

       // store expression on compute lut
       if (realtime) {
         parsed_expressions[i] = new std::deque<Parser::Symbol>(parser.getExpression());
         // fallthough to optimal weigth mode
       }
       else {
         // pure lut, no realtime
         // save memory, reuse luts, like in xyz
         if (customExpressionDefined) {
           luts[i].used = true;
           luts[i].ptr = calculateLut(parser.getExpression(), bits_per_pixel, scale_inputs, clamp_float);
         }
         else {
           if (luts[4].ptr == nullptr) { // 0..3 planes, 4:extra
             luts[4].used = true;
             luts[4].ptr = calculateLut(parser.getExpression(), bits_per_pixel, scale_inputs, clamp_float);
           }
           luts[i].ptr = luts[4].ptr;
         }
         if (luts[i].ptr == nullptr) {
           error = "not enough memory for the lut, use realtime=true";
           return;
         }
       }

       //Part #2 ----- weight expression
       bool customExpressionDefined_w = false;
       if (parameters[expr_strs_w[i]].is_defined()) {
         parser.parse_strict(parameters[expr_strs_w[i]].toString(), Parser::SYMBOL_SEPARATORS);
         customExpressionDefined_w = true;
       }
       else if (parameters["wexpr"].is_defined()) {
         parser.parse_strict(parameters["wexpr"].toString(), Parser::SYMBOL_SEPARATORS);
       }
       else {
         continue; // no parse context/lut will be used for weights
       }

       err_pos = parser.getErrorPos();
       if (err_pos >= 0) {
         error = "Error at position " + std::to_string(1 + err_pos) + ": cannot convert to number: " + parser.getFailedSymbol();
         return;
       }

       hasWeights = true;

       // for check:
       Parser::Context ctx_w(parser.getExpression(), scale_inputs, clamp_float);

       if (!ctx_w.check())
       {
         error = "invalid expression in the lut";
         return;
       }

       // store expression on compute lut
       if (realtime) {
         parsed_expressions_w[i] = new std::deque<Parser::Symbol>(parser.getExpression());
         continue;
       }
       else {
         // pure lut, no realtime
         // save memory, reuse luts, like in xyz
         if (customExpressionDefined_w) {
           luts_weight[i].used = true;
           switch (bits_per_pixel) {
           case 8: luts_weight[i].ptr = calculateLut_w<8>(parser.getExpression(), scale_inputs, clamp_float); break;
           case 10: luts_weight[i].ptr = calculateLut_w<10>(parser.getExpression(), scale_inputs, clamp_float); break;
           case 12: luts_weight[i].ptr = calculateLut_w<12>(parser.getExpression(), scale_inputs, clamp_float); break;
           case 14: luts_weight[i].ptr = calculateLut_w<14>(parser.getExpression(), scale_inputs, clamp_float); break;
#if SIZE_MAX > 0xFFFFFFFFu // 64 bit (32 bit: 16 bit is realtime)
           case 16: luts_weight[i].ptr = calculateLut_w<16>(parser.getExpression(), scale_inputs, clamp_float); break;
#endif
           }
         }
         else {
           if (luts_weight[4].ptr == nullptr) {
             luts_weight[4].used = true;
             switch (bits_per_pixel) {
             case 8: luts_weight[4].ptr = calculateLut_w<8>(parser.getExpression(), scale_inputs, clamp_float); break;
             case 10: luts_weight[4].ptr = calculateLut_w<10>(parser.getExpression(), scale_inputs, clamp_float); break;
             case 12: luts_weight[4].ptr = calculateLut_w<12>(parser.getExpression(), scale_inputs, clamp_float); break;
             case 14: luts_weight[4].ptr = calculateLut_w<14>(parser.getExpression(), scale_inputs, clamp_float); break;
#if SIZE_MAX > 0xFFFFFFFFu // 64 bit (32 bit: 16 bit is realtime)
             case 16: luts_weight[4].ptr = calculateLut_w<16>(parser.getExpression(), scale_inputs, clamp_float); break;
#endif
             }
           }
           luts_weight[i].ptr = luts_weight[4].ptr;
         }
         if (luts_weight[i].ptr == nullptr) {
           error = "not enough memory for the lut, use realtime=true";
           return;
         }
       }
     }

     /* get the pixels list */
     FillCoordinates(parameters["pixels"].toString());

     /* choose the mode */
     mode = parameters["mode"].toString();
     if (realtime) {
       switch (bits_per_pixel) {
       case 8:  processors.push_back(processors_realtime_8_array[ModeToInt(mode)]); break;
       case 10:  processors.push_back(processors_realtime_10_array[ModeToInt(mode)]); break;
       case 12:  processors.push_back(processors_realtime_12_array[ModeToInt(mode)]); break;
       case 14:  processors.push_back(processors_realtime_14_array[ModeToInt(mode)]); break;
       case 16:  processors.push_back(processors_realtime_16_array[ModeToInt(mode)]); break;
       case 32:  processors32.push_back(processors_realtime_32_array[ModeToInt(mode)]); break;
       }
     }
     else {
       switch (bits_per_pixel) {
       case 8:  processors.push_back(processors_array[ModeToInt(mode)]); break;
       case 10:  processors.push_back(processors_10_array[ModeToInt(mode)]); break;
       case 12:  processors.push_back(processors_12_array[ModeToInt(mode)]); break;
       case 14:  processors.push_back(processors_14_array[ModeToInt(mode)]); break;
       case 16:  processors.push_back(processors_16_array[ModeToInt(mode)]); break;
       }
     };
     if (hasWeights) {
       if (realtime) {
         switch (bits_per_pixel) {
         case 8:  processors_weight.push_back(processors_weight_realtime_8_array[ModeToInt(mode)]); break;
         case 10:  processors_weight.push_back(processors_weight_realtime_10_array[ModeToInt(mode)]); break;
         case 12:  processors_weight.push_back(processors_weight_realtime_12_array[ModeToInt(mode)]); break;
         case 14:  processors_weight.push_back(processors_weight_realtime_14_array[ModeToInt(mode)]); break;
         case 16:  processors_weight.push_back(processors_weight_realtime_16_array[ModeToInt(mode)]); break;
         case 32:  processors32_weight.push_back(processors_weight_realtime_32_array[ModeToInt(mode)]); break;
         }
       }
       else {
         switch (bits_per_pixel) {
         case 8:  processors_weight.push_back(processors_weight_array[ModeToInt(mode)]); break;
         case 10:  processors_weight.push_back(processors_weight_10_array[ModeToInt(mode)]); break;
         case 12:  processors_weight.push_back(processors_weight_12_array[ModeToInt(mode)]); break;
         case 14:  processors_weight.push_back(processors_weight_14_array[ModeToInt(mode)]); break;
         case 16:  processors_weight.push_back(processors_weight_16_array[ModeToInt(mode)]); break;
         }
       };
     }
   }

   ~Luts()
   {
     if (pCoordinates) delete[] pCoordinates;
     
     for (int i = 0; i < 4+1; ++i) {
       if (luts[i].used) {
         delete[] luts[i].ptr;
       }
       if (luts_weight[i].used) {
         delete[] luts_weight[i].ptr;
       }
     }
     for (int i = 0; i < 4; i++) {
       delete parsed_expressions[i];
       delete parsed_expressions_w[i];
     }
   }

   InputConfiguration &input_configuration() const override { return InPlaceTwoFrame(); }

   static Signature filter_signature()
   {
      Signature signature = "mt_luts";

      signature.add( Parameter( TYPE_CLIP, "", false ) );
      signature.add( Parameter( TYPE_CLIP, "", false) );
      signature.add( Parameter( String( "average" ), "mode", false) );
      signature.add( Parameter( String( "" ), "pixels", false) );
      signature.add( Parameter( String( "y" ), "expr", false) );
      signature.add( Parameter( String( "y" ), "yExpr", false) );
      signature.add( Parameter( String( "y" ), "uExpr", false) );
      signature.add( Parameter( String( "y" ), "vExpr", false) );

      add_defaults( signature );

      signature.add(Parameter(false, "realtime", false));
      signature.add(Parameter(String("1"), "wexpr", false));
      signature.add(Parameter(String("1"), "ywExpr", false));
      signature.add(Parameter(String("1"), "uwExpr", false));
      signature.add(Parameter(String("1"), "vwExpr", false));
      signature.add(Parameter(String("y"), "aExpr", false));
      signature.add(Parameter(String("1"), "awExpr", false));
      signature.add(Parameter(String("none"), "scale_inputs", false));
      signature.add(Parameter(false, "clamp_float", false));
      signature.add(Parameter(false, "clamp_float_UV", false));
      return signature;
   }
};

} } } } }

#endif