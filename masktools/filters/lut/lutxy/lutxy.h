#ifndef __Mt_Lutxy_H__
#define __Mt_Lutxy_H__

#include "../../../common/base/filter.h"
#include "../../../../common/parser/parser.h"
#include <mutex>

namespace Filtering { namespace MaskTools { namespace Filters { namespace Lut { namespace Dual {

typedef void(Processor)(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int nWidth, int nHeight, const uint8_t lut[65536]);
typedef void(Processor16)(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int nWidth, int nHeight, const uint16_t *lut);
typedef void(ProcessorCtx)(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int nWidth, int nHeight, Parser::Context &ctx);
typedef void(ProcessorCtx32)(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int nWidth, int nHeight, bool chroma, Parser::Context &ctx);

Processor lut_c;
extern Processor16 *lut10_c;
extern Processor16 *lut12_c;
extern Processor16 *lut14_c;
extern Processor16 *lut16_c;

ProcessorCtx realtime8_c;
extern ProcessorCtx *realtime10_c;
extern ProcessorCtx *realtime12_c;
extern ProcessorCtx *realtime14_c;
extern ProcessorCtx *realtime16_c;
ProcessorCtx32 realtime32_c;


class Lutxy : public MaskTools::Filter
{
   // re-use repeated luts, like in lutxyz
   struct Lut {
     bool used;
     uint8_t *ptr;
   };

   Lut luts[4+1];
   
   static uint8_t *calculateLut(const std::deque<Filtering::Parser::Symbol> &expr, int bits_per_pixel, String scale_inputs, int clamp_float, int& compute_error) {
     Parser::Context ctx(expr, scale_inputs, clamp_float);
     int pixelsize = bits_per_pixel == 8 ? 1 : 2; // byte / uint16_t
     size_t buffer_size = ((size_t)1 << bits_per_pixel) * ((size_t)1 << bits_per_pixel) *pixelsize;
     uint8_t *lut = new (std::nothrow) uint8_t[buffer_size];
     if (!lut)
       return nullptr;

     switch (bits_per_pixel) {
     case 8:
       for (int x = 0; x < 256; x++)
         for (int y = 0; y < 256; y++)
           lut[(x << 8) + y] = ctx.compute_byte_xy(x, y);
       break;
     case 10:
       for (int x = 0; x < 1024; x++)
         for (int y = 0; y < 1024; y++)
           reinterpret_cast<uint16_t *>(lut)[(x << 10) + y] = ctx.compute_word_xy<10>(x, y);
       break;
     case 12:
       for (int x = 0; x < 4096; x++)
         for (int y = 0; y < 4096; y++)
           reinterpret_cast<uint16_t *>(lut)[(x << 12) + y] = ctx.compute_word_xy<12>(x, y);
       break;
     case 14:
       for (int x = 0; x < 16384; x++)
         for (int y = 0; y < 16384; y++)
           reinterpret_cast<uint16_t *>(lut)[(x << 14) + y] = ctx.compute_word_xy<14>(x, y);
       break;
     case 16:
       // 64bit only
       for (int x = 0; x < 65536; x++)
         for (int y = 0; y < 65536; y++)
           reinterpret_cast<uint16_t *>(lut)[((size_t)x << 16) + y] = ctx.compute_word_xy<16>(x, y);
       break;
     }

     // problem if compute_error != Parser::Context::compute_error_t::CE_NONE
     compute_error = ctx.get_compute_error();

     return lut;
   }

   // for realtime
   std::deque<Filtering::Parser::Symbol> *parsed_expressions[4];

   Processor *processor;
   Processor16 *processor16;
   ProcessorCtx *processorCtx; // for all 8-16
   ProcessorCtx32 *processorCtx32;
   int bits_per_pixel;
   bool realtime;
   String scale_inputs;
   int clamp_float_i;
   int use_expr;

protected:
    virtual void process(int n, const Plane<uint8_t> &dst, int nPlane, const ::Filtering::Frame<const uint8_t> frames[4], const Constraint constraints[4]) override
    {
        UNUSED(n);
        UNUSED(constraints);
        if (realtime) {
          // thread safety
          Parser::Context ctx(*parsed_expressions[nPlane], scale_inputs, clamp_float_i);
          if (bits_per_pixel <= 16)
            processorCtx(dst.data(), dst.pitch(), frames[0].plane(nPlane).data(), frames[0].plane(nPlane).pitch(), dst.width(), dst.height(), ctx);
          else {
            const bool chroma = ((nPlane == 1 || nPlane == 2) && !planes_isRGB[C]);
            processorCtx32(dst.data(), dst.pitch(), frames[0].plane(nPlane).data(), frames[0].plane(nPlane).pitch(), dst.width(), dst.height(), chroma, ctx); // extra parameter
          }
        }
        else if (bits_per_pixel == 8)
          processor(dst.data(), dst.pitch(), frames[0].plane(nPlane).data(), frames[0].plane(nPlane).pitch(), dst.width(), dst.height(), luts[nPlane].ptr);
        else if (bits_per_pixel <= 16)
          processor16(dst.data(), dst.pitch(), frames[0].plane(nPlane).data(), frames[0].plane(nPlane).pitch(), dst.width(), dst.height(), (uint16_t *)luts[nPlane].ptr);
    }

public:
   Lutxy(const Parameters &parameters, CpuFlags cpuFlags) : MaskTools::Filter( parameters, FilterProcessingType::INPLACE, (CpuFlags)cpuFlags)
   {
      for (int i = 0; i < 4; i++) {
        parsed_expressions[i] = nullptr;
      }

      for (int i = 0; i < 4+1; ++i) {
        luts[i].used = false;
        luts[i].ptr = nullptr;
      }

      static const char *expr_strs[] = { "yExpr", "uExpr", "vExpr", "aExpr" };

      Parser::Parser parser = Parser::getDefaultParser().addSymbol(Parser::Symbol::X).addSymbol(Parser::Symbol::Y);

      bits_per_pixel = bit_depths[C];
      realtime = parameters["realtime"].toBool();
      scale_inputs = parameters["scale_inputs"].toString();
      if (!checkValidScaleInputs(scale_inputs, error))
        return; // error message filled
      bool clamp_float = parameters["clamp_float"].toBool();
      bool clamp_float_UV = parameters["clamp_float_UV"].toBool();
      clamp_float_i = clamp_float ? (clamp_float_UV ? 2 : 1) : 0;

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
        if ((uint64_t)std::numeric_limits<size_t>::max() <= 0xFFFFFFFFull && bits_per_pixel == 16) {
          realtime = true; // not even possible a real 16 bit lutxy on 32 bit environment
        }
      }

      // 2.2.15- decide on using external Expr filter
      // This part is duplicated in lut, lutxy, lutxyz, lutxyza
      use_expr = parameters["use_expr"].toInt();
      bool use_external_expr = false;
      if (use_expr < 0 || use_expr > 3) {
        error = "invalid value for parameter 'use_expr'";
        return;
      }
      if (use_expr == 1 && bits_per_pixel > 8) // mode 1: use Expr when over 8 bits or lutxyza
        use_external_expr = true;
      if (use_expr == 2 && realtime) // mode 2: use Expr when masktools would use its own slow calculation
        use_external_expr = true;
      if (use_expr == 3) // mode 3: use Expr always
        use_external_expr = true;
      if (use_external_expr) {
        if (nXOffset != 0 || nYOffset != 0) {
          error = "X and Y offset must be zero when using 'use_expr'";
          return;
        }
        if (nWidth != nCoreWidth || nHeight != nCoreHeight) {
          error = "Cannot change original width or height when using 'use_expr'";
          return;
        }

        expr_clamp_float = clamp_float;
        expr_clamp_float_UV = clamp_float_UV;
        // expr_need_process[4] and expr_list[4] is defined in filter level
        expr_scale_inputs = scale_inputs; // copy parameter
        // we pass whole frame for Expr, set expression for all planes
        for (int i = 0; i < 4; i++)
        {
          expr_need_process[i] = true;
          if (operators[i] == COPY) {
            expr_list[i] = "x"; // Expr will optimize it to 'copy plane'
          }
          else if (operators[i] == COPY_SECOND) {
            expr_list[i] = "y";
          }
          else if (operators[i] == COPY_THIRD) {
            expr_list[i] = "z";
          }
          else if (operators[i] == COPY_FOURTH) {
            expr_list[i] = "a";
          }
          else if (operators[i] == MEMSET) {
            expr_list[i] = std::to_string(operators[i].value_f()); // Expr will optimize it to 'fill plane'
          }
          else if (operators[i] == PROCESS) {
            // no expression for spec y/u/v/a plane, neither have we the jolly joker "expr"
            if (parameters[expr_strs[i]].undefinedOrEmptyString() && parameters["expr"].undefinedOrEmptyString()) {
              expr_list[i] = "x"; // in Expr: no such thing as 'no process', copy 1st clip
              continue;
            }
            else if (parameters[expr_strs[i]].is_defined()) {
              expr_list[i] = parameters[expr_strs[i]].toString();
            }
            else {
              expr_list[i] = parameters["expr"].toString();
            }
          }
          else {
            expr_need_process[i] = false;
          }
        } // planes
      }
      else {
        /* compute the luts or set up internal realtime processor */
        for (int i = 0; i < 4; i++)
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
          Parser::Context ctx(parser.getExpression(), scale_inputs, clamp_float_i);

          if (!ctx.check())
          {
            error = "invalid expression in the lut";
            return;
          }

          if (realtime) {
            parsed_expressions[i] = new std::deque<Parser::Symbol>(parser.getExpression());

            switch (bits_per_pixel) {
            case 8: processorCtx = realtime8_c; break;
            case 10: processorCtx = realtime10_c; break;
            case 12: processorCtx = realtime12_c; break;
            case 14: processorCtx = realtime14_c; break;
            case 16: processorCtx = realtime16_c; break;
            case 32: processorCtx32 = realtime32_c; break;
            }
            continue;
          }

          // pure lut, no realtime

          int compute_error = Parser::Context::compute_error_t::CE_NONE;

          // save memory, reuse luts, like in xyz
          if (customExpressionDefined) {
            luts[i].used = true;
            luts[i].ptr = calculateLut(parser.getExpression(), bits_per_pixel, scale_inputs, clamp_float_i, /*ref*/ compute_error); // fixme/fixed: this was clamp_float before 2.2.27
          }
          else {
            if (luts[4].ptr == nullptr) {
              luts[4].used = true;
              luts[4].ptr = calculateLut(parser.getExpression(), bits_per_pixel, scale_inputs, clamp_float_i, /*ref*/ compute_error);
            }
            luts[i].ptr = luts[4].ptr;
          }

          if (luts[i].ptr == nullptr) {
            error = "not enough memory for the lut, use realtime=true";
            return;
          }

          if (compute_error != Parser::Context::compute_error_t::CE_NONE) {
            error = "invalid expression in the lut code = " + std::to_string(compute_error);
            return;
          }

          switch (bits_per_pixel) {
          case 8:
            processor = lut_c;
            break;
          case 10:
            processor16 = lut10_c;
            break;
          case 12:
            processor16 = lut12_c;
            break;
          case 14:
            processor16 = lut14_c;
            break;
          case 16:
            // 64bit only
            processor16 = lut16_c;
            break;
          }
        }
      }
   }

   ~Lutxy()
   {
     for (int i = 0; i < 4+1; ++i) {
       if (luts[i].used) {
         delete[] luts[i].ptr;
       }
     }
     for (int i = 0; i < 4; i++) {
       delete parsed_expressions[i];
     }
   }

   InputConfiguration &input_configuration() const override { return InPlaceTwoFrame(); }

   static Signature filter_signature()
   {
      Signature signature = "mt_lutxy";

      signature.add(Parameter(TYPE_CLIP, "", false));
      signature.add(Parameter(TYPE_CLIP, "", false));
      signature.add(Parameter(String("x"), "expr", false));
      signature.add(Parameter(String("x"), "yExpr", false));
      signature.add(Parameter(String("x"), "uExpr", false));
      signature.add(Parameter(String("x"), "vExpr", false));

      add_defaults( signature );

      signature.add(Parameter(false, "realtime", false));
      signature.add(Parameter(String("x"), "aExpr", false));
      signature.add(Parameter(String("none"), "scale_inputs", false));
      signature.add(Parameter(false, "clamp_float", false));
      signature.add(Parameter(0, "use_expr", false));
      signature.add(Parameter(false, "clamp_float_UV", false));
      return signature;
   }
};

} } } } }

#endif