#ifndef __Mt_Lut_H__
#define __Mt_Lut_H__

#include "../../../common/base/filter.h"
#include "../../../../common/parser/parser.h"

/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
#include "avs/config.h" // WIN/POSIX/ETC defines

#if (defined(GCC) || defined(CLANG)) && !defined(_WIN32)
#include <stdlib.h>
#define _aligned_malloc(size, alignment) aligned_alloc(alignment, size)
#define _aligned_free(ptr) free(ptr)
#endif

namespace Filtering { namespace MaskTools { namespace Filters { namespace Lut { namespace Single {

typedef void(Processor)(uint8_t *pDst, ptrdiff_t nDstPitch, int nWidth, int nHeight, const uint8_t *lut);
typedef void(Processor16)(uint8_t *pDst, ptrdiff_t nDstPitch, int nWidth, int nHeight, const uint16_t *lut, int bits_per_pixel);
typedef void(ProcessorCtx)(uint8_t *pDst, ptrdiff_t nDstPitch, int nWidth, int nHeight, Parser::Context &ctx);
typedef void(ProcessorCtx16)(uint8_t *pDst, ptrdiff_t nDstPitch, int nWidth, int nHeight, Parser::Context &ctx, int bits_per_pixel);
typedef void(ProcessorCtx32)(uint8_t *pDst, ptrdiff_t nDstPitch, int nWidth, int nHeight, bool chroma, Parser::Context &ctx);

Processor lut_c;
Processor16 lut16_c;

ProcessorCtx realtime8_c;
ProcessorCtx16 realtime16_c;
ProcessorCtx32 realtime32_c;

class Lutx : public MaskTools::Filter
{
  // re-use repeated luts, like in lutxyz
  struct Lut {
    bool used;
    uint8_t* ptr;
  };

  Lut luts[4 + 2];

  static uint8_t* calculateLut(const std::deque<Filtering::Parser::Symbol>& expr, bool chroma, int bits_per_pixel, String scale_inputs, int clamp_float, int &compute_error) {
    Parser::Context ctx(expr, scale_inputs, clamp_float);
    ctx.set_integer_chroma(chroma);
    int pixelsize = bits_per_pixel == 8 ? 1 : 2; // byte / uint16_t
    size_t buffer_size = ((size_t)1 << bits_per_pixel) * pixelsize;
    uint8_t* lut = (uint8_t *)_aligned_malloc(buffer_size, 128);

    if (bits_per_pixel == 8) {
      for (int x = 0; x < 256; x++)
        lut[x] = ctx.compute_byte_x(x);
    }
    else {
      for (int x = 0; x < (1 << bits_per_pixel); x++)
        reinterpret_cast<uint16_t*>(lut)[x] = ctx.compute_word_x(x, bits_per_pixel);
    }

    // problem if compute_error != Parser::Context::compute_error_t::CE_NONE
    compute_error = ctx.get_compute_error();

    return lut;
  }

   // for realtime
   std::deque<Filtering::Parser::Symbol> *parsed_expressions[4];

   Processor *processor;
   Processor16 *processor16;
   ProcessorCtx *processorCtx; // 8 bits
   ProcessorCtx16 *processorCtx16; // 10-16 bits
   ProcessorCtx32 *processorCtx32;
   int bits_per_pixel;
   bool isStacked;
   bool realtime;
   String scale_inputs;
   int clamp_float_i;
   int use_expr;

protected:
    virtual void process(int n, const Plane<uint8_t> &dst, int nPlane, const ::Filtering::Frame<const uint8_t> frames[4], const Constraint constraints[4]) override
    {
        UNUSED(constraints);
        UNUSED(frames);
        if (realtime) {
          // thread safety
          Parser::Context ctx(*parsed_expressions[nPlane], scale_inputs, clamp_float_i);
          set_plane_context(ctx, dst, n, nPlane);
          if (bits_per_pixel == 8)
            processorCtx(dst.data(), dst.pitch(), dst.width(), dst.height(), ctx);
          else if (bits_per_pixel <= 16)
            processorCtx16(dst.data(), dst.pitch(), dst.width(), dst.height(), ctx, bits_per_pixel);
          else {
            const bool chroma = ((nPlane == 1 || nPlane == 2) && !isRGB);
            processorCtx32(dst.data(), dst.pitch(), dst.width(), dst.height(), chroma, ctx); // extra parameter
          }
        }
        else if (bits_per_pixel == 8)
          processor(dst.data(), dst.pitch(), dst.width(), dst.height(), luts[nPlane].ptr);
        else if (bits_per_pixel <= 16)
          processor16(dst.data(), dst.pitch(), dst.width(), dst.height(), (uint16_t*)luts[nPlane].ptr, bits_per_pixel);
    }

public:
  Lutx(const Parameters &parameters, CpuFlags cpuFlags) : MaskTools::Filter(parameters, FilterProcessingType::INPLACE, (CpuFlags)cpuFlags)
  {
      for (int i = 0; i < 4; i++) {
        parsed_expressions[i] = nullptr;
      }

      for (int i = 0; i < 4 + 2; ++i) {
        luts[i].used = false;
        luts[i].ptr = nullptr;
      }

      static const char *expr_strs[] = { "yExpr", "uExpr", "vExpr", "aExpr" };

      Parser::Parser parser = Parser::getDefaultParser().addSymbol(Parser::Symbol::X);

      isStacked = parameters["stacked"].toBool();
      bits_per_pixel = nBitDepth;

      if (isStacked && bits_per_pixel != 8) {
        error = "Stacked specified for a non-8 bit clip";
        return;
      }

      if (isStacked)
        bits_per_pixel = 16;

      realtime = parameters["realtime"].toBool();
      scale_inputs = parameters["scale_inputs"].toString();
      if (!checkValidScaleInputs(scale_inputs, error))
        return; // error message filled

      bool clamp_float = parameters["clamp_float"].toBool();
      bool clamp_float_UV = parameters["clamp_float_UV"].toBool();
      clamp_float_i = clamp_float ? (clamp_float_UV ? 2 : 1) : 0;

      if (bits_per_pixel == 32) { // no lookup for float
        realtime = true;
      }

      if (expressions_use_context(parser, expr_strs, "expr"))
        realtime = true; // width, height, sx, sy, sxr, syr, frameno, time: no lut, whatever 'realtime' says

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
      if (isStacked) // Expr would see the 8 bit double height clip: always the internal 16 bit calculation
        use_external_expr = false;
      if (use_external_expr) {
        if (nXOffset != 0 || nYOffset != 0 || nWidth != nCoreWidth || nHeight != nCoreHeight) {
          error = "use_expr: only the full frame can be passed to Expr, offx, offy, w and h cannot be used";
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

          Parser::Context ctx(parser.getExpression(), scale_inputs, clamp_float_i);

          if (!ctx.check())
          {
            error = "invalid expression in the lut";
            return;
          }

          if (realtime) {
            parsed_expressions[i] = new std::deque<Parser::Symbol>(parser.getExpression());

            if (bits_per_pixel == 8) processorCtx = realtime8_c;
            else if (bits_per_pixel <= 16) processorCtx16 = realtime16_c;
            else processorCtx32 = realtime32_c;
            continue;
          }

          // pure lut, no realtime

          int compute_error = Parser::Context::compute_error_t::CE_NONE;

          // save memory, reuse luts, like in xyz
          const bool chroma = !isRGB && (i == 1 || i == 2);
          const int shared = chroma && Parser::lut_depends_on_plane(parser.getExpression()) ? 5 : 4; // 0..3: planes
          if (customExpressionDefined) {
            luts[i].used = true;
            luts[i].ptr = calculateLut(parser.getExpression(), chroma, bits_per_pixel, scale_inputs, clamp_float_i, /*ref*/ compute_error);
          }
          else {
            if (luts[shared].ptr == nullptr) {
              luts[shared].used = true;
              luts[shared].ptr = calculateLut(parser.getExpression(), chroma, bits_per_pixel, scale_inputs, clamp_float_i, /*ref*/ compute_error);
            }
            luts[i].ptr = luts[shared].ptr;
          }

          if (compute_error != Parser::Context::compute_error_t::CE_NONE) {
            error = "invalid expression in the lut code = " + std::to_string(compute_error);
            return;
          }

          if (bits_per_pixel == 8)
            processor = lut_c;
          else
            processor16 = lut16_c; // stacked too: processed as native 16 bit
        }
      }
   }

   ~Lutx()
   {
     for (int i = 0; i < 4 + 2; ++i) {
       if (luts[i].used) {
         _aligned_free(luts[i].ptr);
       }
     }
     for (int i = 0; i < 4; i++) {
       delete parsed_expressions[i];
     }
   }

   InputConfiguration &input_configuration() const override { return InPlaceOneFrame(); }

   static Signature filter_signature()
   {
      Signature signature = "mt_lut";

      signature.add(Parameter(TYPE_CLIP, "", false));
      signature.add(Parameter(String("x"), "expr", false));
      signature.add(Parameter(String("x"), "yExpr", false));
      signature.add(Parameter(String("x"), "uExpr", false));
      signature.add(Parameter(String("x"), "vExpr", false));
      signature.add(Parameter(String("x"), "aExpr", false));

      add_defaults( signature );

      signature.add(Parameter(false, "stacked", false));
      signature.add(Parameter(false, "realtime", false));
      signature.add(Parameter(String("none"), "scale_inputs", false));
      signature.add(Parameter(false, "clamp_float", false));
      signature.add(Parameter(0, "use_expr", false));
      signature.add(Parameter(false, "clamp_float_UV", false));
      return signature;
   }
};


} } } } }

#endif