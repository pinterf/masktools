#ifndef __Mt_Base_H__
#define __Mt_Base_H__

#include "../../../common/functions/functions.h"
#include "../../../common/params/params.h"
#include "../../../common/parser/parser.h"
#include "../../../common/utils/utils.h"
#include "../../common/params/params.h"
#include "../../common/clip/inputconfig.h"
#include "stacked.h"

namespace Filtering { namespace MaskTools { 

enum class FilterProcessingType {
    INPLACE,
    CHILD
};

class Filter {

protected:
    ClipArray childs;
    Parameters parameters;

    int nWidth, nHeight;
    Colorspace C;

    String error;
    CpuFlags flags;
    Operator operators[4];

    bool inPlace_;
    int nXOffset, nYOffset, nXOffsetUV, nYOffsetUV;
    int nCoreWidth, nCoreHeight, nCoreWidthUV, nCoreHeightUV;

    bool has_avs_expr_support;

   virtual void process(int n, const Plane<uint8_t> &dst, int nPlane, const Frame<const uint8_t> frames[4], const Constraint constraints[4]) = 0;
   virtual InputConfiguration &input_configuration() const = 0;

   static Signature &add_defaults(Signature &signature)
   {
      signature.add( Parameter( 3.0f, "Y", true) );
      signature.add( Parameter( 1.0f, "U", true) );
      signature.add( Parameter( 1.0f, "V", true) );
      signature.add( Parameter( Value( String( "" ) ), "chroma", false));
      signature.add( Parameter( 0, "offX", false) );
      signature.add( Parameter( 0, "offY", false) );
      signature.add( Parameter( -1, "w", false) );
      signature.add( Parameter( -1, "h", false) );
      signature.add( Parameter( true, "sse2", false) );
      signature.add( Parameter( true, "sse3", false) );
      signature.add( Parameter( true, "ssse3", false) );
      signature.add( Parameter( true, "sse4", false) );
      signature.add( Parameter( true, "avx", false));
      signature.add( Parameter( true, "avx2", false));
      signature.add( Parameter( 1.0f, "A", true)); // put it at the end, don't change original parameter order
      signature.add(Parameter(Value(String("")), "alpha", false)); // same function as "chroma", for alpha plane
      signature.add(Parameter(String("i8"), "paramscale", false)); // like in expressions + none

      return signature;
   }

   // general helper function
   static bool checkValidScaleInputs(String scale_inputs, String &error) {
     if (scale_inputs != "int" && scale_inputs != "intf" &&
       scale_inputs != "floatUV" && scale_inputs != "float" && scale_inputs != "floatf" &&
       scale_inputs != "all" && scale_inputs != "allf" &&
       scale_inputs != "none") {
       error = "invalid parameter: scale_inputs. Use int, intf, floatUV, float, floatf, all, allf or none";
       return false;
     }
     return true;
   }


   // general helper function
   static bool ScaleParam(String scalemode, float input, int clip_bits_per_pixel, float &scaled_f, int &scaled_i, bool fullscale, bool allowNegative, bool chroma)
   {
     int param_bits_per_pixel;
     int max_pixel_value = (1 << clip_bits_per_pixel) - 1;
     if (scalemode == "i8") param_bits_per_pixel = 8;
     else if (scalemode == "i10") param_bits_per_pixel = 10;
     else if (scalemode == "i12") param_bits_per_pixel = 12;
     else if (scalemode == "i14") param_bits_per_pixel = 14;
     else if (scalemode == "i16") param_bits_per_pixel = 16;
     else if (scalemode == "f32") param_bits_per_pixel = 32;
     else if ((scalemode == "none") || scalemode == "") {
       scaled_f = input;
       if (clip_bits_per_pixel != 32) {
         if(scaled_f >= 0 || !allowNegative)
           scaled_i = max(min(int(scaled_f), max_pixel_value), 0);
         else
           scaled_i = -max(min(int(-scaled_f), max_pixel_value), 0);
       }
       return true;
     }
     else {
       return false; // invalid parameter
     }

     // identical bit-depth, return original
     if (param_bits_per_pixel == clip_bits_per_pixel) {
       scaled_f = input;
       if (clip_bits_per_pixel != 32) {
         if(scaled_f >= 0 || !allowNegative)
           scaled_i = max(min(int(scaled_f), max_pixel_value), 0);
         else
           scaled_i = -max(min(int(-scaled_f), max_pixel_value), 0);
       }
       return true;
     }

     // param_bits_per_pixel -> clip_bits_per_pixel
     if (fullscale) {
       // stretch mode
       if (param_bits_per_pixel >= 8 && param_bits_per_pixel <= 16) {
         input = input / ((1 << param_bits_per_pixel) - 1); // convert to 0..1 range
       }
       // input is now scaled_f to 0..1
       if (clip_bits_per_pixel == 32) {
         if (chroma) {
#ifdef FLOAT_CHROMA_IS_HALF_CENTERED
           scaled_f = input; // as-is
#else
           scaled_f = input - 0.5f; // +/-0.5
#endif
         }
         else {
           scaled_f = input; // as-is
         }
       } 
       else
         scaled_f = input * ((1 << clip_bits_per_pixel) - 1); // scaling up to video bit depth
     }
     else {
       // bit shift mode, for integers but preserves max value if max (255) is specified
       if (clip_bits_per_pixel == 32) { // convert to float
         // integer parameter -> float value
         const int param_max_pixel_value = (1 << param_bits_per_pixel) - 1;
         if (chroma) {
           const int chroma_center = 1 << (param_bits_per_pixel - 1);
#ifdef FLOAT_CHROMA_IS_HALF_CENTERED
           scaled_f = (input - chroma_center) / param_max_pixel_value + 0.5f;
#else
           scaled_f = (input - chroma_center) / param_max_pixel_value;
#endif
         }
         else {
           scaled_f = input / param_max_pixel_value;
         }
       }
       else if (param_bits_per_pixel == 32) {
         // float parameter -> integer value
         if (chroma) {
           const int target_chroma_center = 1 << (clip_bits_per_pixel - 1);
#ifdef FLOAT_CHROMA_IS_HALF_CENTERED
           scaled_f = (input - 0.5f) * max_pixel_value + target_chroma_center;
#else
           scaled_f = input * max_pixel_value + target_chroma_center;
#endif
         }
         else {
           scaled_f = input * ((1 << clip_bits_per_pixel) - 1);
         }
       }
       else {
         // integer parameter -> integer value
         bool isNegativeInput = input < 0;
         input = std::abs(input);
         float input_range_max = (float)((1 << param_bits_per_pixel) - 1);
         if (abs(input_range_max - input) < 0.000001) {
           scaled_f = (float)((1 << clip_bits_per_pixel) - 1); // keep max range
         }
         else {
           if (param_bits_per_pixel > clip_bits_per_pixel)
             scaled_f = input / ((1 << (param_bits_per_pixel - clip_bits_per_pixel))); // scale down by diff bits
           else
             scaled_f = input * ((1 << (clip_bits_per_pixel - param_bits_per_pixel))); // scale up by diff bits
         }
         if (isNegativeInput)
           scaled_f = -scaled_f;
       }
     }

     if (clip_bits_per_pixel != 32) {
       if (scaled_f >= 0 || !allowNegative)
         scaled_i = max(min(int(scaled_f), max_pixel_value), 0);
       else
         scaled_i = -max(min(int(-scaled_f), max_pixel_value), 0);
     }

     return true;
   }




public:

    // v2.2.15 Save parameters of lut functions, used when calling external "Expr":
    bool expr_need_process[4];
    String expr_scale_inputs;
    bool expr_clamp_float;
    bool expr_clamp_float_UV; // v2.2.20: new along with avs+ 3.5
    String expr_list[4];

    bool isStacked; // stacked 16 bit (MSB/LSB halves of an 8 bit clip): processed as native 16 bit, see stacked.h

    Filter(const Parameters &parameters, FilterProcessingType processingType, CpuFlags _flags) :
        parameters(parameters),
        flags(_flags),
        inPlace_(processingType == FilterProcessingType::INPLACE),
        nXOffset(parameters["offx"].toInt()),
        nYOffset(parameters["offy"].toInt()),
        nCoreWidth(parameters["w"].toInt()),
        nCoreHeight((parameters["stacked"].is_defined() && parameters["stacked"].toBool() && parameters["h"].toInt()>=0) ? (2 * parameters["h"].toInt()) : parameters["h"].toInt()),
        isStacked(parameters["stacked"].is_defined() && parameters["stacked"].toBool())
    {
        for (auto &param: parameters) {
            if (param.getType() == TYPE_CLIP) {
                childs.push_back(param.getValue().toClip());
            }
        }

        assert(!childs.empty());

        nWidth = childs[0]->width();
        nHeight = childs[0]->height();
        C = childs[0]->colorspace();

        operators[0] = Operator((float)parameters["Y"].toFloat()); // prepare, float for memset
        operators[1] = Operator((float)parameters["U"].toFloat());
        operators[2] = Operator((float)parameters["V"].toFloat());
        operators[3] = Operator((float)parameters["A"].toFloat()); // new from 2.2.7: alpha

        if (nXOffset < 0 || nXOffset > nWidth) nXOffset = 0;
        if (nYOffset < 0 || nYOffset > nHeight) nYOffset = 0;
        if (nXOffset + nCoreWidth  > nWidth  || nCoreWidth  < 0) nCoreWidth = nWidth - nXOffset;
        if (nYOffset + nCoreHeight > nHeight || nCoreHeight < 0) nCoreHeight = nHeight - nYOffset;

        // Snap the window to the subsampling grid the same way Frame::offset does,
        // so that nCoreWidth/nCoreHeight are the real luma window sizes.
        if (plane_counts[C] > 1) {
            const int rw = width_ratios[2][C];
            const int rh = height_ratios[2][C];
            const int x1 = (nXOffset + nCoreWidth) / rw * rw;
            const int y1 = (nYOffset + nCoreHeight) / rh * rh;
            nXOffset = nXOffset / rw * rw;
            nYOffset = nYOffset / rh * rh;
            nCoreWidth = x1 - nXOffset;
            nCoreHeight = y1 - nYOffset;
        }

        String scalemode = parameters["paramscale"].toString();
        const bool fullscale = planes_isRGB[C];
        const int bits_per_pixel = bit_depths[C];

        if (parameters["chroma"].is_defined())
        {
            /* overrides chroma channel operators according to the "chroma" string */
            String chroma = parameters["chroma"].toString();

            if (chroma == "process")
                operators[1] = operators[2] = PROCESS;
            else if (chroma == "copy")
                operators[1] = operators[2] = COPY;
            else if (chroma == "copy first")
                operators[1] = operators[2] = COPY;
            else if (chroma == "copy second")
                operators[1] = operators[2] = COPY_SECOND;
            else if (chroma == "copy third")
                operators[1] = operators[2] = COPY_THIRD;
            else if (chroma == "copy fourth")
                operators[1] = operators[2] = COPY_FOURTH;
            else if (chroma == "none") // 2.2.9
                operators[1] = operators[2] = NONE;
            else if (chroma == "ignore") // 2.2.9
                operators[1] = operators[2] = NONE;
            else {
              try {
                size_t i(0);
                float f = std::stof(chroma.c_str(), &i);
                if (f > 0) {
                  error = "filler value parameter should be <=0 for chroma";
                  return;
                }
#ifndef FLOAT_CHROMA_IS_HALF_CENTERED
                if (bits_per_pixel == 32 && (scalemode == "none" || scalemode == "f32")) {
                  error = "Cannot use 'chroma' to set filler value for 32 bit float formats, use paramscale";
                  return;
                }
#endif
                f = -f; // !! For 32bit float chroma +/-0.5 negative values cannot be set with this method, only by paramscale
                operators[1] = operators[2] = Operator(MEMSET, f);
              }
              catch (...)
              {
                error = "invalid parameter value for chroma";
                return;
              }
            }
        }

        if (parameters["alpha"].is_defined())
        {
          /* overrides alpha channel operators according to the "alpha" string v2.2.7- */
          String alpha = parameters["alpha"].toString();

          if (alpha == "process")
            operators[3] = PROCESS;
          else if (alpha == "copy")
            operators[3] = COPY;
          else if (alpha == "copy first")
            operators[3] = COPY;
          else if (alpha == "copy second")
            operators[3] = COPY_SECOND;
          else if (alpha == "copy third")
            operators[3] = COPY_THIRD;
          else if (alpha == "copy fourth")
            operators[3] = COPY_FOURTH;
          else if (alpha == "none") // 2.2.9
            operators[3] = NONE;
          else if (alpha == "ignore") // 2.2.9
            operators[3] = NONE;
          else {
            try {
              size_t i(0);
              float f = std::stof(alpha.c_str(), &i);
              if (f > 0) {
                error = "filler value parameter should be <=0 for alpha";
                return;
              }
              f = -f;
              operators[3] = Operator(MEMSET, f);
            }
            catch (...)
            {
              error = "invalid parameter value for alpha";
              return;
            }
          }
        }

        for (int i = 0; i < 4; i++) {
          if (operators[i].getMode() != MEMSET) continue;
          float op_f = operators[i].value_f();
          int op;
          const bool as_chroma = !planes_isRGB[C] && (i == 1 || i == 2); // scales to float specially
          const bool as_fullscale = fullscale || i == 3; // alpha channel is always fullscale
          if (!ScaleParam(scalemode, op_f, bits_per_pixel, op_f, op, as_fullscale, false, as_chroma))
          {
            error = "invalid parameter: paramscale. Use i8, i10, i12, i14, i16, f32 for scale or none/empty to disable scaling";
            return;
          }
          operators[i] = Operator(MEMSET, op_f);
        }

        /* checks the operators */
        for (int i = 0; i < 4; i++)
        {
            if (operators[i] == COPY_FOURTH && childs.size() < 4)
                operators[i] = COPY_THIRD;
            if (operators[i] == COPY_THIRD && childs.size() < 3)
                operators[i] = COPY_SECOND;
            if (operators[i] == COPY_SECOND && childs.size() < 2)
                operators[i] = COPY;
        }

        if (is_in_place())
        {
            /* in place filters copy differently */
            for (int i = 0; i < 4; i++)
            {
                switch (operators[i].getMode())
                {
                case COPY: operators[i] = NONE; break;
                case COPY_SECOND: operators[i] = COPY; break;
                case COPY_THIRD: operators[i] = COPY_SECOND; break;
                case COPY_FOURTH: operators[i] = COPY_THIRD; break;
                default: break;
                }
            }
        }

        // don't try to process non-existant planes
        for (int i = plane_counts[C]; i < 4; i++)
          operators[i] = NONE;

        /* effective modes */
        print(LOG_DEBUG, "modes : %i %i %i %i\n", operators[0].getMode(), operators[1].getMode(), operators[2].getMode(), operators[3].getMode());

        /* cpu flags */
        if (!parameters["sse2"].toBool()) flags &= ~CPU_SSE2;
        if (!parameters["sse3"].toBool()) flags &= ~CPU_SSE3;
        if (!parameters["ssse3"].toBool()) flags &= ~CPU_SSSE3;
        if (!parameters["sse4"].toBool()) {
            flags &= ~CPU_SSE4_1;
            flags &= ~CPU_SSE4_2;
        }
        if (!parameters["avx"].toBool()) flags &= ~CPU_AVX;
        if (!parameters["avx2"].toBool()) flags &= ~CPU_AVX2;

        print(LOG_DEBUG, "using cpu flags : 0x%x\n", flags);

        /* chroma offsets and box */
        if (C != COLORSPACE_Y8 && C != COLORSPACE_Y10 && C != COLORSPACE_Y12 && C != COLORSPACE_Y14 && C != COLORSPACE_Y16 && C != COLORSPACE_Y32 && C != COLORSPACE_NONE)
        {
          // also for planar RGB, naming is a bit confusing yet
            nXOffsetUV = nXOffset / width_ratios[1][C];
            nYOffsetUV = nYOffset / height_ratios[1][C];
            nCoreWidthUV = (nXOffset + nCoreWidth) / width_ratios[1][C] - nXOffsetUV;
            nCoreHeightUV = (nYOffset + nCoreHeight) / height_ratios[1][C] - nYOffsetUV;
        }

        /* effective offset */
        print(LOG_DEBUG, "offset : %i %i, width x height : %i x %i\n", nXOffset, nYOffset, nCoreWidth, nCoreHeight);

        /* check the colorspace */
        if (C == COLORSPACE_NONE)
            error = "masktools: unsupported colorspace, use Y8, YV12, YV16, YV24, YV411, greyscale, YUV(A)xxxP10-16/S, Planar RGB(A)";

        // use avs+ expr instead of realcalc lut
        // overridden in lut##.h
        for (int i = 0; i < 4; i++) {
          expr_need_process[i] = false;
          expr_list[i] = "";
        }
    }

    void process_plane(int n, const Plane<uint8_t> &output_plane, int nPlane, const Constraint constraints[4], const Frame<const uint8_t> frames[4])
    {
        // stacked clips arrive here as native 16 bit planes (process_stacked)
        switch (operators[nPlane].getMode())
        {
        case COPY:
        case COPY_SECOND:
        case COPY_THIRD:
        case COPY_FOURTH:
        {
            const int src = operators[nPlane].getMode() == COPY ? 0 : operators[nPlane].getMode() == COPY_SECOND ? 1 : operators[nPlane].getMode() == COPY_THIRD ? 2 : 3;
            Functions::copy_plane(output_plane.data(), output_plane.pitch(),
              frames[src].plane(nPlane).data(), frames[src].plane(nPlane).pitch(),
              output_plane.width()*output_plane.pixelsize(), output_plane.height()); // rowsize = width*pixelsize
            break;
        }
        case MEMSET:
            switch (output_plane.pixelsize()) {
            case 1:
              Functions::memset_plane(output_plane.data(), output_plane.pitch(),
                output_plane.width(), output_plane.height(), // memset needs real width
                static_cast<uint8_t>(operators[nPlane].value()));
              break;
            case 2: // 16 bit
              Functions::memset_plane_16(output_plane.data(), output_plane.pitch(),
                output_plane.width(), output_plane.height(),
                static_cast<uint16_t>(operators[nPlane].value()));
              break;
            case 4: // 32 bit/float
              Functions::memset_plane_32(output_plane.data(), output_plane.pitch(),
                output_plane.width(), output_plane.height(),
                static_cast<float>(operators[nPlane].value_f()));
              break;
            }
        break;
        case PROCESS:
            process(n, output_plane, nPlane, frames, constraints);
            break;
        case NONE:
        default: break;
        }
    }

    virtual Frame<uint8_t> get_frame(int n, const Frame<uint8_t> &output_frame, bool need_copy_frame_properties, PVideoFrame &dst, IScriptEnvironment *env)
    {
      Frame<uint8_t> output = output_frame.offset(nXOffset, nYOffset, nCoreWidth, nCoreHeight);

      Frame<const uint8_t> frames[4];
      Constraint constraints[4];

      int clipcount = int(input_configuration().size());

      std::vector<PVideoFrame> tmp_videoframes(clipcount);

      for (int i = 0; i < clipcount; i++) {
        int childindex = input_configuration()[i].index();
        PClip &currentClip = childs[childindex];
        // output in tmp_videoframes
        frames[i] = currentClip->get_const_frame(n + input_configuration()[i].offset(), tmp_videoframes[i], env)
          .offset(nXOffset, nYOffset, nCoreWidth, nCoreHeight);
        if (need_copy_frame_properties && i == 0) // copy from first
          env->copyFrameProps(tmp_videoframes[0], dst);
      }

      if (isStacked) {
        if (output.plane(0).width() > 0 && output.plane(0).height() > 0)
          process_stacked(n, output, frames, clipcount);
        return output_frame;
      }

      for (int i = 0; i < plane_counts[C]; i++) {
        constraints[i] = Constraint(plane_flags(output.plane(i)), output.plane(i));
      }

      for (int i = 0; i < clipcount; i++) {
        for (int j = 0; j < plane_counts[frames[i].colorspace()]; j++) {
          constraints[j] = Constraint(constraints[j], frames[i].plane(j));
        }
      }

      for (int i = 0; i < plane_counts[C]; i++) {
        // a window snapped to the subsampling can be empty (e.g. YV12 offx=0, w=1): nothing to process
        if (output.plane(i).width() <= 0 || output.plane(i).height() <= 0)
          continue;
        process_plane(n, output.plane(i), i, constraints, frames);
      }

      return output_frame;
    }

    // Planes narrower or lower than 2 pixels go to the C processors: the SIMD
    // neighbourhood kernels (3x3 etc.) assume at least 2 rows and columns.
    CpuFlags plane_flags(const Plane<uint8_t> &plane) const
    {
      return plane.width() < 2 || plane.height() < 2 ? CpuFlags(CPU_NONE) : flags;
    }

    // stacked 16 bit: the planes converted to native 16 bit, processed, the output converted back
    void process_stacked(int n, const Frame<uint8_t> &output, const Frame<const uint8_t> frames[4], int clipcount)
    {
      NativePlaneBuffer out_buf[4], in_buf[4][4];
      Plane<uint8_t> out_native[4];
      Frame<const uint8_t> frames_native[4];
      Constraint constraints[4];

      const int planecount = plane_counts[C];
      for (int i = 0; i < planecount; i++) {
        const Plane<uint8_t> &o = output.plane(i);
        // the output too: in place filters and the "none" mode keep its content
        out_native[i] = out_buf[i].from_stacked(Plane<const uint8_t>(o.data(), o.pitch(), o.width(), o.height(), o.pixelsize(), o.origheight()));
        constraints[i] = Constraint(plane_flags(out_native[i]), out_native[i]);
      }
      for (int i = 0; i < clipcount; i++) {
        Plane<const uint8_t> planes[4];
        const int pc = plane_counts[frames[i].colorspace()];
        for (int j = 0; j < pc; j++) {
          Plane<uint8_t> p = in_buf[i][j].from_stacked(frames[i].plane(j));
          planes[j] = Plane<const uint8_t>(p.data(), p.pitch(), p.width(), p.height(), p.pixelsize(), p.origheight());
          constraints[j] = Constraint(constraints[j], planes[j]);
        }
        frames_native[i] = Frame<const uint8_t>(planes, frames[i].colorspace());
      }
      for (int i = 0; i < planecount; i++) {
        if (out_native[i].width() <= 0 || out_native[i].height() <= 0)
          continue;
        process_plane(n, out_native[i], i, constraints, frames_native);
        NativePlaneBuffer::to_stacked(out_native[i], output.plane(i));
      }
    }

    ClipArray &get_childs() { return childs; }

    String get_error() const { return error; }
    bool is_error() const { return !error.empty(); }
    bool is_in_place() const { return inPlace_; }
    Colorspace colorspace() const { return C; }

};

} } // namespace MaskTools, Filtering

#endif
