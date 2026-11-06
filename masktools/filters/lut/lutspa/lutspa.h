#ifndef __Mt_Lutspa_H__
#define __Mt_Lutspa_H__

#include "../../../common/base/filter.h"
#include "../../../../common/parser/parser.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Lut { namespace Coordinate {

typedef void(Processor)(uint8_t *pDst, ptrdiff_t nDstPitch, int nWidth, int nHeight, const uint8_t *lut);

Processor lut_c;

class Lutspa : public MaskTools::Filter
{
   uint8_t *luts[4] = {};
   int bits_per_pixel;
   int pixelsize;
   bool is_relative;
   bool is_biased;
   String scale_inputs;
   int clamp_float_i;
   // frameno, time: computed for each frame
   std::deque<Parser::Symbol> *parsed_expressions[4] = {};

   void fill_plane(uint8_t *out, ptrdiff_t pitch, int w, int h, int nPlane, Parser::Context &ctx) const
   {
      const int divisor_x = (is_biased || w < 2) ? w : w - 1;
      const int divisor_y = (is_biased || h < 2) ? h : h - 1;
      const bool chroma = !isRGB && (nPlane == 1 || nPlane == 2); // not RGB and plane U or V
      ctx.set_integer_chroma(chroma);
      for (int y = 0; y < h; y++) {
         ctx.set_row(y);
         for (int x = 0; x < w; x++) {
            ctx.set_column(x);
            switch (bits_per_pixel) {
            case 8:
               out[x] = is_relative ? ctx.compute_byte_xy_dblinput(x * 1.0 / divisor_x, y * 1.0 / divisor_y) : ctx.compute_byte_xy(x, y);
               break;
            case 32:
               reinterpret_cast<float *>(out)[x] = is_relative ?
                  ctx.compute_float_xy(x * 1.0 / divisor_x, y * 1.0 / divisor_y, chroma) : ctx.compute_float_xy(x, y, chroma);
               break;
            default:
               // input is not integer, don't use compute_word_xy<n>; bit depth goes to param B
               reinterpret_cast<uint16_t *>(out)[x] = is_relative ?
                  ctx.compute_word_xy_safe_dblinput(x * 1.0 / divisor_x, y * 1.0 / divisor_y, bits_per_pixel) :
                  ctx.compute_word_xy_safe(x, y, bits_per_pixel);
               break;
            }
         }
         out += pitch;
      }
   }

protected:
    virtual void process(int n, const Plane<uint8_t> &dst, int nPlane, const Filtering::Frame<const uint8_t> frames[4], const Constraint constraints[4]) override
    {
        UNUSED(frames); UNUSED(constraints);
        if (parsed_expressions[nPlane]) {
           // thread safety
           Parser::Context ctx(*parsed_expressions[nPlane], scale_inputs, clamp_float_i);
           set_plane_context(ctx, dst, n, nPlane);
           fill_plane(dst.data(), dst.pitch(), dst.width(), dst.height(), nPlane, ctx);
           return;
        }
        Functions::copy_plane(dst.data(), dst.pitch(), luts[nPlane], dst.width() * pixelsize, dst.width() * pixelsize, dst.height());
        // copy_plane needs row_size in bytes
    }

public:
   Lutspa(const Parameters &parameters, CpuFlags cpuFlags) : MaskTools::Filter(parameters, FilterProcessingType::CHILD, (CpuFlags)cpuFlags)
   {
      bits_per_pixel = nBitDepth;
      pixelsize = bits_per_pixel == 8 ? 1 : (bits_per_pixel <= 16) ? 2 : 4;

      static const char *expr_strs[] = { "yExpr", "uExpr", "vExpr", "aExpr" };
      is_relative = parameters["relative"].toBool();
      is_biased = parameters["biased"].toBool();
      
      if (parameters["mode"].is_defined())
      {
          if (parameters["mode"].toString() == "absolute") is_relative = false, is_biased = false;
          else if (parameters["mode"].toString() == "relative inclusive") is_relative = true, is_biased = false;
          else if (parameters["mode"].toString() == "relative closed") is_relative = true, is_biased = false;
          else if (parameters["mode"].toString() == "relative exclusive") is_relative = true, is_biased = true;
          else if (parameters["mode"].toString() == "relative opened") is_relative = true, is_biased = true;
          else if (parameters["mode"].toString() == "relative") is_relative = true, is_biased = true;
      }

      Parser::Parser parser = Parser::getDefaultParser().addSymbol(Parser::Symbol::X).addSymbol(Parser::Symbol::Y);

      scale_inputs = parameters["scale_inputs"].toString();
      if (!checkValidScaleInputs(scale_inputs, error))
        return; // error message filled
      bool clamp_float = parameters["clamp_float"].toBool();
      bool clamp_float_UV = parameters["clamp_float_UV"].toBool();
      clamp_float_i = clamp_float ? (clamp_float_UV ? 2 : 1) : 0;

      /* compute the luts */
      for ( int i = 0; i < 4; i++ )
      {
          luts[i] = nullptr;

          if (operators[i] != PROCESS) {
              continue;
          }
          
          if (parameters[expr_strs[i]].undefinedOrEmptyString() && parameters["expr"].undefinedOrEmptyString()) {
              operators[i] = Operator(MEMSET, 0); //for no real reason
              continue;
          }

         const bool is_chroma = i == 1 || i == 2;
         const int w = is_chroma ? nCoreWidthUV : nCoreWidth;
         const int h = is_chroma ? nCoreHeightUV : nCoreHeight;
         if ( parameters[expr_strs[i]].is_defined() ) 
            parser.parse_strict(parameters[expr_strs[i]].toString(), Parser::SYMBOL_SEPARATORS);
         else
            parser.parse_strict(parameters["expr"].toString(), Parser::SYMBOL_SEPARATORS);

         auto err_pos = parser.getErrorPos();
         if (err_pos >= 0) {
           error = "Error at position " + std::to_string(1 + err_pos) + ": cannot convert to number: " + parser.getFailedSymbol();
           return;
         }

         Parser::Context ctx(parser.getExpression(), scale_inputs, clamp_float_i);

         if ( !ctx.check() )
         {
            error = "invalid expression in the lut";
            return;
         }

         if (Parser::expression_dependencies(parser.getExpression()) & Parser::DEPENDS_ON_FRAME) {
            parsed_expressions[i] = new std::deque<Parser::Symbol>(parser.getExpression());
            continue;
         }

         luts[i] = reinterpret_cast<uint8_t*>(_aligned_malloc(w*h*pixelsize, 16));
         ctx.set_plane(w, h, 0, childs[0]->frame_count());
         fill_plane(luts[i], (ptrdiff_t)w * pixelsize, w, h, i, ctx);
      }
   }

   ~Lutspa()
   {
       for (int i = 0; i < 4; i++) {
           _aligned_free(luts[i]);
           delete parsed_expressions[i];
       }
   }

   InputConfiguration &input_configuration() const override { 
       for (int i = 0; i < 4; i++) {
           if (operators[i] == COPY) {
               return OneFrame();
           }
       }
       return InPlaceOneFrame();
   }

   static Signature filter_signature()
   {
      Signature signature = "mt_lutspa";

      signature.add(Parameter(TYPE_CLIP, "", false));
      signature.add(Parameter(String("relative"), "mode", false));
      signature.add(Parameter(Value(true), "relative", false));
      signature.add(Parameter(Value(true), "biased", false));
      signature.add(Parameter(String("x"), "expr", false));
      signature.add(Parameter(String("x"), "yExpr", false));
      signature.add(Parameter(String("x"), "uExpr", false));
      signature.add(Parameter(String("x"), "vExpr", false));

      add_defaults( signature );

      signature.add(Parameter(String("x"), "aExpr", false));
      signature.add(Parameter(String("none"), "scale_inputs", false));
      signature.add(Parameter(false, "clamp_float", false));
      signature.add(Parameter(false, "clamp_float_UV", false));

      return signature;

   }
};

} } } } }

#endif