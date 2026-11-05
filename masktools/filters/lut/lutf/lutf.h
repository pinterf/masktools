#ifndef __Mt_Lutf_H__
#define __Mt_Lutf_H__

#include "../../../common/base/filter.h"
#include "../../../../common/parser/parser.h"

#include "../functions.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Lut { namespace Frame {

typedef void(Processor)(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, const uint8_t lut[65536], Parser::Context *ctx, int nWidth, int nHeight); // common lut/realtime, templatized
typedef void(Processor16)(uint16_t *pDst, ptrdiff_t nDstPitch, const uint16_t *pSrc, ptrdiff_t nSrcPitch, const uint16_t *lut, Parser::Context *ctx, int nWidth, int nHeight, int bits_per_pixel); // common lut/realtime, templatized
typedef void(Processor32)(float *pDst, ptrdiff_t nDstPitch, const float *pSrc, ptrdiff_t nSrcPitch, Parser::Context *ctx, int nWidth, int nHeight, bool chroma); // realtime only

extern Processor *processors_array[NUM_MODES];
extern Processor *processorsCtx_array[NUM_MODES];

extern Processor16 *processors16_array[NUM_MODES];
extern Processor16 *processors16Ctx_array[NUM_MODES];
extern Processor32 *processors32Ctx_array[NUM_MODES];

class Lutf : public MaskTools::Filter
{
  // re-use repeated luts, like in lutxyz
  struct Lut {
    bool used;
    uint8_t *ptr;
  };

  Lut luts[4+1]; // max plane count + 1

  static uint8_t *calculateLut(const std::deque<Filtering::Parser::Symbol> &expr, int bits_per_pixel, String scale_inputs, int clamp_float, int& compute_error) {
    Parser::Context ctx(expr, scale_inputs, clamp_float);
    int pixelsize = bits_per_pixel == 8 ? 1 : 2; // byte / uint16_t
    size_t buffer_size = ((size_t)1 << bits_per_pixel) * ((size_t)1 << bits_per_pixel) *pixelsize;
    uint8_t *lut = new (std::nothrow) uint8_t[buffer_size];
    if (!lut)
      return nullptr;

    if (bits_per_pixel == 8) {
      for (int x = 0; x < 256; x++)
        for (int y = 0; y < 256; y++)
          lut[(x << 8) + y] = ctx.compute_byte_xy(x, y);
    }
    else {
      // 16 bit: 64 bit only
      const int size = 1 << bits_per_pixel;
      for (int x = 0; x < size; x++)
        for (int y = 0; y < size; y++)
          reinterpret_cast<uint16_t *>(lut)[((size_t)x << bits_per_pixel) + y] = ctx.compute_word_xy(x, y, bits_per_pixel);
    }

    // problem if compute_error != Parser::Context::compute_error_t::CE_NONE
    compute_error = ctx.get_compute_error();

    return lut;
  }

  // for realtime
  std::deque<Filtering::Parser::Symbol> *parsed_expressions[4];

  ProcessorList<Processor> processors;
  ProcessorList<Processor16> processors16;
  ProcessorList<Processor32> processors32;

  int bits_per_pixel;
  bool realtime;
  String scale_inputs;
  int clamp_float_i;

protected:
    virtual void process(int n, const Plane<uint8_t> &dst, int nPlane, const Filtering::Frame<const uint8_t> frames[4], const Constraint constraints[4]) override
    {
        
        if (realtime) {
          // thread safety
          Parser::Context ctx(*parsed_expressions[nPlane], scale_inputs, clamp_float_i);
          set_plane_context(ctx, dst, n);
          if (bits_per_pixel == 8)
            processors.best_processor(constraints[nPlane])(dst.data(), dst.pitch(),
              frames[0].plane(nPlane).data(), frames[0].plane(nPlane).pitch(),
              nullptr, &ctx, dst.width(), dst.height());
          else if (bits_per_pixel <= 16)
            processors16.best_processor(constraints[nPlane])((uint16_t *)dst.data(), dst.pitch(),
              (uint16_t *)frames[0].plane(nPlane).data(), frames[0].plane(nPlane).pitch(),
              nullptr, &ctx, dst.width(), dst.height(), bits_per_pixel);
          else {
            const bool chroma = ((nPlane == 1 || nPlane == 2) && !isRGB);
            processors32.best_processor(constraints[nPlane])((float *)dst.data(), dst.pitch(),
              (float *)frames[0].plane(nPlane).data(), frames[0].plane(nPlane).pitch(),
              &ctx, dst.width(), dst.height(), chroma);
          }
        }
        else {
          // lut
          if (bits_per_pixel == 8)
            processors.best_processor(constraints[nPlane])(dst.data(), dst.pitch(),
              frames[0].plane(nPlane).data(), frames[0].plane(nPlane).pitch(),
              luts[nPlane].ptr, nullptr, dst.width(), dst.height());
          else if (bits_per_pixel <= 16)
            processors16.best_processor(constraints[nPlane])((uint16_t *)dst.data(), dst.pitch(),
              (uint16_t *)frames[0].plane(nPlane).data(), frames[0].plane(nPlane).pitch(),
              (uint16_t *)luts[nPlane].ptr, nullptr, dst.width(), dst.height(), bits_per_pixel);
          // no 32 bit lut
        }
    }

public:
   Lutf(const Parameters &parameters, CpuFlags cpuFlags) : MaskTools::Filter( parameters, FilterProcessingType::INPLACE, (CpuFlags)cpuFlags)
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

      bits_per_pixel = nBitDepth;
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

      if (expressions_use_context(parser, expr_strs, "expr"))
        realtime = true; // width, height, sx, sy, sxr, syr, frameno, time: no lut, whatever 'realtime' says

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

          // pure lut, no realtime

          int compute_error = Parser::Context::compute_error_t::CE_NONE;

          // save memory, reuse luts, like in xyz
          if (customExpressionDefined) {
            luts[i].used = true;
            luts[i].ptr = calculateLut(parser.getExpression(), bits_per_pixel, scale_inputs, clamp_float, /*ref*/ compute_error);
          }
          else {
            if (luts[4].ptr == nullptr) { // 0..3: planes, 4:last common
              luts[4].used = true;
              luts[4].ptr = calculateLut(parser.getExpression(), bits_per_pixel, scale_inputs, clamp_float, /*ref*/ compute_error);
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

      }
      const int m = ModeToInt(parameters["mode"].toString());
      if (realtime) {
        if (bits_per_pixel == 8) processors.push_back(processorsCtx_array[m]);
        else if (bits_per_pixel <= 16) processors16.push_back(processors16Ctx_array[m]);
        else processors32.push_back(processors32Ctx_array[m]);
      }
      else {
        // real lut
        if (bits_per_pixel == 8) processors.push_back(processors_array[m]);
        else processors16.push_back(processors16_array[m]);
      }
   }

   ~Lutf()
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
      Signature signature = "mt_lutf";

      signature.add( Parameter( TYPE_CLIP, "", false) );
      signature.add( Parameter( TYPE_CLIP, "", false) );
      signature.add( Parameter( String( "average" ), "mode", false) );
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