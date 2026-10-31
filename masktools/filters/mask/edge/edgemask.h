#ifndef __Mt_Edgemask_H__
#define __Mt_Edgemask_H__

#include "../../../common/base/filter.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Mask { namespace Edge {

typedef void(Processor)(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, const int16_t matrix[10], int nLowThreshold, int nHighThreshold, int nWidth, int nHeight);
typedef void(Processor16)(uint16_t *pDst, ptrdiff_t nDstPitch, const uint16_t *pSrc, ptrdiff_t nSrcPitch, const int16_t matrix[10], int nLowThreshold, int nHighThreshold, int nWidth, int nHeight);
typedef void(Processor32)(float *pDst, ptrdiff_t nDstPitch, const float *pSrc, ptrdiff_t nSrcPitch, const float matrix[10], float nLowThreshold, float nHighThreshold, int nWidth, int nHeight);

extern Processor *convolution_c;
extern Processor *convolution_sse2;
extern Processor *convolution_avx2;

extern Processor *sobel_c;
extern Processor *sobel_sse2;
extern Processor *sobel_ssse3;
extern Processor *sobel_avx2;

extern Processor *roberts_c;
extern Processor *roberts_sse2;
extern Processor *roberts_ssse3;
extern Processor *roberts_avx2;

extern Processor *laplace_c;
extern Processor *laplace_sse2;
extern Processor *laplace_ssse3;
extern Processor *laplace_avx2;

extern Processor *cartoon_c;
extern Processor *cartoon_sse2;
extern Processor *cartoon_avx2;

extern Processor *half_prewitt_c;
extern Processor *half_prewitt_sse2;
extern Processor *half_prewitt_ssse3;
extern Processor *half_prewitt_avx2;

extern Processor *prewitt_c;
extern Processor *prewitt_sse2;
extern Processor *prewitt_ssse3;
extern Processor *prewitt_avx2;

extern Processor *morpho_c;
extern Processor *morpho_sse2;
extern Processor *morpho_avx2;

// uint16_t
#define DEFINE_EXTERNS(name) \
extern Processor16 *name##_10_c; \
extern Processor16 *name##_12_c; \
extern Processor16 *name##_14_c; \
extern Processor16 *name##_16_c; \
extern Processor16 *name##_10_sse2; \
extern Processor16 *name##_12_sse2; \
extern Processor16 *name##_14_sse2; \
extern Processor16 *name##_16_sse2; \
extern Processor16 *name##_10_sse4; \
extern Processor16 *name##_12_sse4; \
extern Processor16 *name##_14_sse4; \
extern Processor16 *name##_16_sse4; \
extern Processor16 *name##_10_avx2; \
extern Processor16 *name##_12_avx2; \
extern Processor16 *name##_14_avx2; \
extern Processor16 *name##_16_avx2;

DEFINE_EXTERNS(convolution)
DEFINE_EXTERNS(sobel)
DEFINE_EXTERNS(roberts)
DEFINE_EXTERNS(laplace)
DEFINE_EXTERNS(cartoon)
DEFINE_EXTERNS(half_prewitt)
DEFINE_EXTERNS(prewitt)
DEFINE_EXTERNS(morpho)

#undef DEFINE_EXTERNS

// float
extern Processor32 *convolution_32_c;
extern Processor32 *sobel_32_c;
extern Processor32 *roberts_32_c;
extern Processor32 *laplace_32_c;
extern Processor32 *cartoon_32_c;
extern Processor32 *half_prewitt_32_c;
extern Processor32 *prewitt_32_c;
extern Processor32 *morpho_32_c;

extern Processor32 *convolution_32_sse2;
extern Processor32 *sobel_32_sse2;
extern Processor32 *roberts_32_sse2;
extern Processor32 *laplace_32_sse2;
extern Processor32 *cartoon_32_sse2;
extern Processor32 *half_prewitt_32_sse2;
extern Processor32 *prewitt_32_sse2;
extern Processor32 *morpho_32_sse2;

extern Processor32 *convolution_32_avx;
extern Processor32 *sobel_32_avx;
extern Processor32 *roberts_32_avx;
extern Processor32 *laplace_32_avx;
extern Processor32 *cartoon_32_avx;
extern Processor32 *half_prewitt_32_avx;
extern Processor32 *prewitt_32_avx;
extern Processor32 *morpho_32_avx;

class EdgeMask : public MaskTools::Filter
{
   int nLowThresholds[4];
   int nHighThresholds[4];

   float nLowThresholds_f[4];
   float nHighThresholds_f[4];

   int16_t matrix[10];
   float matrix_f[10];

   ProcessorList<Processor> processors;
   ProcessorList<Processor16> processors16;
   ProcessorList<Processor32> processors32;

protected:

    virtual void process(int n, const Plane<uint8_t> &dst, int nPlane, const Filtering::Frame<const uint8_t> frames[4], const Constraint constraints[4]) override
    {
        UNUSED(n);
        if (bit_depths[C] == 8) {
          processors.best_processor(constraints[nPlane])(dst.data(), dst.pitch(),
            frames[0].plane(nPlane).data(), frames[0].plane(nPlane).pitch(),
            matrix, nLowThresholds[nPlane], nHighThresholds[nPlane], dst.width(), dst.height());
        }
        else if(bit_depths[C] <= 16) {
          processors16.best_processor(constraints[nPlane])((uint16_t *)dst.data(), dst.pitch(),
            (uint16_t *)frames[0].plane(nPlane).data(), frames[0].plane(nPlane).pitch(),
            matrix, nLowThresholds[nPlane], nHighThresholds[nPlane], dst.width(), dst.height());
        }
        else { // float
          processors32.best_processor(constraints[nPlane])((float *)dst.data(), dst.pitch(),
            (float *)frames[0].plane(nPlane).data(), frames[0].plane(nPlane).pitch(),
            matrix_f, nLowThresholds_f[nPlane], nHighThresholds_f[nPlane], dst.width(), dst.height());
        }
    }

public:
   EdgeMask(const Parameters &parameters, CpuFlags cpuFlags) : MaskTools::Filter( parameters, FilterProcessingType::CHILD, (CpuFlags)cpuFlags)
   {
     int bits_per_pixel = bit_depths[C];
     bool isFloat = bits_per_pixel == 32;

     bool fullscale = planes_isRGB[C];
     String scalemode = parameters["paramscale"].toString();

     int thY1, thC1, thY2, thC2;
     float thY1_f, thC1_f, thY2_f, thC2_f;

     // defaults
     thY1_f = thC1_f = thY2_f = thC2_f = 10.0f / 255;
     thY1 = thC1 = thY2 = thC2 = 10 << (bits_per_pixel - 8);

     const char *errortxt = "invalid parameter: paramscale. Use i8, i10, i12, i14, i16, f32 for scale or none/empty to disable scaling";

     // Y threshold
     if (parameters["thY1"].is_defined()) {
       thY1_f = (float)parameters["thY1"].toFloat();
       if (!ScaleParam(scalemode, thY1_f, bits_per_pixel, thY1_f, thY1, fullscale, false, false))
       {
         error = errortxt;
         return;
       }
     }

     if (parameters["thY2"].is_defined()) {
       thY2_f = (float)parameters["thY2"].toFloat();
       if (!ScaleParam(scalemode, thY2_f, bits_per_pixel, thY2_f, thY2, fullscale, false, false))
       {
         error = errortxt;
         return;
       }
     }

     // chroma threshold
     if (parameters["thC1"].is_defined()) {
       thC1_f = (float)parameters["thC1"].toFloat();
       if (!ScaleParam(scalemode, thC1_f, bits_per_pixel, thC1_f, thC1, fullscale, false, true))
       {
         error = errortxt;
         return;
       }
     }

     if (parameters["thC2"].is_defined()) {
       thC2_f = (float)parameters["thC2"].toFloat();
       if (!ScaleParam(scalemode, thC2_f, bits_per_pixel, thC2_f, thC2, fullscale, false, true))
       {
         error = errortxt;
         return;
       }
     }

     if (isFloat) {
       nLowThresholds_f[0] = thY1_f;
       nLowThresholds_f[1] = nLowThresholds_f[2] = thC1_f;
       nHighThresholds_f[0] = thY2_f;
       nHighThresholds_f[1] = nHighThresholds_f[2] = thC2_f;
       nLowThresholds_f[3] = thY1_f; // no separate parameter for alpha
       nHighThresholds_f[3] = thY2_f;
     }
     else {
       nLowThresholds[0] = thY1;
       nLowThresholds[1] = nLowThresholds[2] = thC1;
       nHighThresholds[0] = thY2;
       nHighThresholds[1] = nHighThresholds[2] = thC2;
       nLowThresholds[3] = thY1; // no separate parameter for alpha
       nHighThresholds[3] = thY2;
     }

      /* add the processors */
      if ( parameters["mode"].toString() == "sobel" )
      {
         print(LOG_DEBUG, "Edge : using sobel detector");
         switch (bits_per_pixel) {
         case 8:
           processors.push_back(Filtering::Processor<Processor>(sobel_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0));
#ifdef INTEL_INTRINSICS
           processors.push_back(Filtering::Processor<Processor>(sobel_sse2, Constraint(CPU_SSE2, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 17), 1));
           processors.push_back(Filtering::Processor<Processor>(sobel_ssse3, Constraint(CPU_SSSE3, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 17), 2));
           processors.push_back(Filtering::Processor<Processor>(sobel_avx2, Constraint(CPU_AVX2, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 33), 3));
#endif
           break;
         case 10: 
           processors16.push_back(Filtering::Processor<Processor16>(sobel_10_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0));
#ifdef INTEL_INTRINSICS
           processors16.push_back(Filtering::Processor<Processor16>(sobel_10_sse2, Constraint(CPU_SSE2, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 16), 1));
           processors16.push_back(Filtering::Processor<Processor16>(sobel_10_sse4, Constraint(CPU_SSE4_1, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 16), 2));
           processors16.push_back(Filtering::Processor<Processor16>(sobel_10_avx2, Constraint(CPU_AVX2, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 32), 3));
#endif
           break;
         case 12:
           processors16.push_back(Filtering::Processor<Processor16>(sobel_12_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0));
#ifdef INTEL_INTRINSICS
           processors16.push_back(Filtering::Processor<Processor16>(sobel_12_sse2, Constraint(CPU_SSE2, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 16), 1));
           processors16.push_back(Filtering::Processor<Processor16>(sobel_12_sse4, Constraint(CPU_SSE4_1, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 16), 2));
           processors16.push_back(Filtering::Processor<Processor16>(sobel_12_avx2, Constraint(CPU_AVX2, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 32), 3));
#endif
           break;
         case 14:
           processors16.push_back(Filtering::Processor<Processor16>(sobel_14_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0));
#ifdef INTEL_INTRINSICS
           processors16.push_back(Filtering::Processor<Processor16>(sobel_14_sse2, Constraint(CPU_SSE2, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 16), 1));
           processors16.push_back(Filtering::Processor<Processor16>(sobel_14_sse4, Constraint(CPU_SSE4_1, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 16), 2));
           processors16.push_back(Filtering::Processor<Processor16>(sobel_14_avx2, Constraint(CPU_AVX2, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 32), 3));
#endif
           break;
         case 16:
           processors16.push_back(Filtering::Processor<Processor16>(sobel_16_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0));
#ifdef INTEL_INTRINSICS
           processors16.push_back(Filtering::Processor<Processor16>(sobel_16_sse2, Constraint(CPU_SSE2, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 16), 1));
           processors16.push_back(Filtering::Processor<Processor16>(sobel_16_sse4, Constraint(CPU_SSE4_1, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 16), 2));
           processors16.push_back(Filtering::Processor<Processor16>(sobel_16_avx2, Constraint(CPU_AVX2, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 32), 3));
#endif
           break;
         case 32: 
           processors32.push_back(Filtering::Processor<Processor32>(sobel_32_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0));
#ifdef INTEL_INTRINSICS
           processors32.push_back(Filtering::Processor<Processor32>(sobel_32_sse2, Constraint(CPU_SSE2, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 16), 1));
           processors32.push_back(Filtering::Processor<Processor32>(sobel_32_avx, Constraint(CPU_AVX, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 32), 2));
#endif
           break;
         }
      }
      else if ( parameters["mode"].toString() == "roberts" )
      {
         print(LOG_DEBUG, "Edge : using roberts detector");
         switch(bits_per_pixel) {
         case 8: 
           processors.push_back(Filtering::Processor<Processor>(roberts_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0));
#ifdef INTEL_INTRINSICS
           processors.push_back(Filtering::Processor<Processor>(roberts_sse2, Constraint(CPU_SSE2, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 17), 1));
           processors.push_back(Filtering::Processor<Processor>(roberts_ssse3, Constraint(CPU_SSSE3, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 17), 2));
           processors.push_back(Filtering::Processor<Processor>(roberts_avx2, Constraint(CPU_AVX2, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 33), 3));
#endif
           break;
         case 10:
           processors16.push_back(Filtering::Processor<Processor16>(roberts_10_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0));
#ifdef INTEL_INTRINSICS
           processors16.push_back(Filtering::Processor<Processor16>(roberts_10_sse2, Constraint(CPU_SSE2, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 16), 1));
           processors16.push_back(Filtering::Processor<Processor16>(roberts_10_sse4, Constraint(CPU_SSE4_1, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 16), 2));
           processors16.push_back(Filtering::Processor<Processor16>(roberts_10_avx2, Constraint(CPU_AVX2, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 32), 3));
#endif
           break;
         case 12:
           processors16.push_back(Filtering::Processor<Processor16>(roberts_12_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0));
#ifdef INTEL_INTRINSICS
           processors16.push_back(Filtering::Processor<Processor16>(roberts_12_sse2, Constraint(CPU_SSE2, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 16), 1));
           processors16.push_back(Filtering::Processor<Processor16>(roberts_12_sse4, Constraint(CPU_SSE4_1, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 16), 2));
           processors16.push_back(Filtering::Processor<Processor16>(roberts_12_avx2, Constraint(CPU_AVX2, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 32), 3));
#endif
           break;
         case 14:
           processors16.push_back(Filtering::Processor<Processor16>(roberts_14_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0));
#ifdef INTEL_INTRINSICS
           processors16.push_back(Filtering::Processor<Processor16>(roberts_14_sse2, Constraint(CPU_SSE2, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 16), 1));
           processors16.push_back(Filtering::Processor<Processor16>(roberts_14_sse4, Constraint(CPU_SSE4_1, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 16), 2));
           processors16.push_back(Filtering::Processor<Processor16>(roberts_14_avx2, Constraint(CPU_AVX2, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 32), 3));
#endif
           break;
         case 16:
           processors16.push_back(Filtering::Processor<Processor16>(roberts_16_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0));
#ifdef INTEL_INTRINSICS
           processors16.push_back(Filtering::Processor<Processor16>(roberts_16_sse2, Constraint(CPU_SSE2, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 16), 1));
           processors16.push_back(Filtering::Processor<Processor16>(roberts_16_sse4, Constraint(CPU_SSE4_1, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 16), 2));
           processors16.push_back(Filtering::Processor<Processor16>(roberts_16_avx2, Constraint(CPU_AVX2, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 32), 3));
#endif
           break;
         case 32: 
           processors32.push_back(Filtering::Processor<Processor32>(roberts_32_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0));
#ifdef INTEL_INTRINSICS
           processors32.push_back(Filtering::Processor<Processor32>(roberts_32_sse2, Constraint(CPU_SSE2, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 16), 1));
           processors32.push_back(Filtering::Processor<Processor32>(roberts_32_avx, Constraint(CPU_AVX, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 32), 2));
#endif
           break;
         }
      }
      else if (parameters["mode"].toString() == "laplace")
      {
        print(LOG_DEBUG, "Edge : using laplace detector");
        switch (bits_per_pixel) {
        case 8:
          processors.push_back(Filtering::Processor<Processor>(laplace_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0));
#ifdef INTEL_INTRINSICS
          processors.push_back(Filtering::Processor<Processor>(laplace_sse2, Constraint(CPU_SSE2, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 17), 1));
          processors.push_back(Filtering::Processor<Processor>(laplace_ssse3, Constraint(CPU_SSSE3, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 17), 2));
          processors.push_back(Filtering::Processor<Processor>(laplace_avx2, Constraint(CPU_AVX2, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 33), 3));
#endif
          break;
        case 10: 
          processors16.push_back(Filtering::Processor<Processor16>(laplace_10_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0));
#ifdef INTEL_INTRINSICS
          processors16.push_back(Filtering::Processor<Processor16>(laplace_10_sse2, Constraint(CPU_SSE2, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 16), 1));
          processors16.push_back(Filtering::Processor<Processor16>(laplace_10_sse4, Constraint(CPU_SSE4_1, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 16), 2));
          processors16.push_back(Filtering::Processor<Processor16>(laplace_10_avx2, Constraint(CPU_AVX2, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 32), 3));
#endif
          break;
        case 12:
          processors16.push_back(Filtering::Processor<Processor16>(laplace_12_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0));
#ifdef INTEL_INTRINSICS
          processors16.push_back(Filtering::Processor<Processor16>(laplace_12_sse2, Constraint(CPU_SSE2, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 16), 1));
          processors16.push_back(Filtering::Processor<Processor16>(laplace_12_sse4, Constraint(CPU_SSE4_1, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 16), 2));
          processors16.push_back(Filtering::Processor<Processor16>(laplace_12_avx2, Constraint(CPU_AVX2, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 32), 3));
#endif
          break;
        case 14:
          processors16.push_back(Filtering::Processor<Processor16>(laplace_14_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0));
#ifdef INTEL_INTRINSICS
          processors16.push_back(Filtering::Processor<Processor16>(laplace_14_sse2, Constraint(CPU_SSE2, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 16), 1));
          processors16.push_back(Filtering::Processor<Processor16>(laplace_14_sse4, Constraint(CPU_SSE4_1, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 16), 2));
          processors16.push_back(Filtering::Processor<Processor16>(laplace_14_avx2, Constraint(CPU_AVX2, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 32), 3));
#endif
          break;
        case 16:
          processors16.push_back(Filtering::Processor<Processor16>(laplace_16_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0));
#ifdef INTEL_INTRINSICS
          processors16.push_back(Filtering::Processor<Processor16>(laplace_16_sse2, Constraint(CPU_SSE2, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 16), 1));
          processors16.push_back(Filtering::Processor<Processor16>(laplace_16_sse4, Constraint(CPU_SSE4_1, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 16), 2));
          processors16.push_back(Filtering::Processor<Processor16>(laplace_16_avx2, Constraint(CPU_AVX2, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 32), 3));
#endif
          break;
        case 32: 
          processors32.push_back(Filtering::Processor<Processor32>(laplace_32_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0));
#ifdef INTEL_INTRINSICS
          processors32.push_back(Filtering::Processor<Processor32>(laplace_32_sse2, Constraint(CPU_SSE2, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 16), 1));
          processors32.push_back(Filtering::Processor<Processor32>(laplace_32_avx, Constraint(CPU_AVX, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 32), 2));
#endif
          break;
        }
      }
      else if (parameters["mode"].toString() == "cartoon")
      {
        print(LOG_DEBUG, "Edge : using cartoon detector");
        switch (bits_per_pixel) {
        case 8:
          processors.push_back(Filtering::Processor<Processor>(cartoon_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0));
#ifdef INTEL_INTRINSICS
          processors.push_back(Filtering::Processor<Processor>(cartoon_sse2, Constraint(CPU_SSE2, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 17), 2));
          processors.push_back(Filtering::Processor<Processor>(cartoon_avx2, Constraint(CPU_AVX2, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 33), 3));
#endif
          break;
        case 10: 
          processors16.push_back(Filtering::Processor<Processor16>(cartoon_10_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0));
#ifdef INTEL_INTRINSICS
          processors16.push_back(Filtering::Processor<Processor16>(cartoon_10_sse2, Constraint(CPU_SSE2, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 16), 1));
          processors16.push_back(Filtering::Processor<Processor16>(cartoon_10_sse4, Constraint(CPU_SSE4_1, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 16), 2));
          processors16.push_back(Filtering::Processor<Processor16>(cartoon_10_avx2, Constraint(CPU_AVX2, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 32), 3));
#endif
          break;
        case 12:
          processors16.push_back(Filtering::Processor<Processor16>(cartoon_12_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0));
#ifdef INTEL_INTRINSICS
          processors16.push_back(Filtering::Processor<Processor16>(cartoon_12_sse2, Constraint(CPU_SSE2, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 16), 1));
          processors16.push_back(Filtering::Processor<Processor16>(cartoon_12_sse4, Constraint(CPU_SSE4_1, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 16), 2));
          processors16.push_back(Filtering::Processor<Processor16>(cartoon_12_avx2, Constraint(CPU_AVX2, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 32), 3));
#endif
          break;
        case 14:
          processors16.push_back(Filtering::Processor<Processor16>(cartoon_14_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0));
#ifdef INTEL_INTRINSICS
          processors16.push_back(Filtering::Processor<Processor16>(cartoon_14_sse2, Constraint(CPU_SSE2, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 16), 1));
          processors16.push_back(Filtering::Processor<Processor16>(cartoon_14_sse4, Constraint(CPU_SSE4_1, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 16), 2));
          processors16.push_back(Filtering::Processor<Processor16>(cartoon_14_avx2, Constraint(CPU_AVX2, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 32), 3));
#endif
          break;
        case 16:
          processors16.push_back(Filtering::Processor<Processor16>(cartoon_16_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0));
#ifdef INTEL_INTRINSICS
          processors16.push_back(Filtering::Processor<Processor16>(cartoon_16_sse2, Constraint(CPU_SSE2, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 16), 1));
          processors16.push_back(Filtering::Processor<Processor16>(cartoon_16_sse4, Constraint(CPU_SSE4_1, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 16), 2));
          processors16.push_back(Filtering::Processor<Processor16>(cartoon_16_avx2, Constraint(CPU_AVX2, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 32), 3));
#endif
          break;
        case 32: 
          processors32.push_back(Filtering::Processor<Processor32>(cartoon_32_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0));
#ifdef INTEL_INTRINSICS
          processors32.push_back(Filtering::Processor<Processor32>(cartoon_32_sse2, Constraint(CPU_SSE2, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 16), 1));
          processors32.push_back(Filtering::Processor<Processor32>(cartoon_32_avx, Constraint(CPU_AVX, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 32), 2));
#endif
          break;
        }
      }
      else if (parameters["mode"].toString() == "min/max")
      {
        print(LOG_DEBUG, "Edge : using morphologic detector");
        switch (bits_per_pixel) {
        case 8:
          processors.push_back(Filtering::Processor<Processor>(morpho_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0));
#ifdef INTEL_INTRINSICS
          processors.push_back(Filtering::Processor<Processor>(morpho_sse2, Constraint(CPU_SSE2, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 17), 1));
          processors.push_back(Filtering::Processor<Processor>(morpho_avx2, Constraint(CPU_AVX2, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 33), 3));
#endif
          break;
        case 10: 
          processors16.push_back(Filtering::Processor<Processor16>(morpho_10_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0));
#ifdef INTEL_INTRINSICS
          processors16.push_back(Filtering::Processor<Processor16>(morpho_10_sse2, Constraint(CPU_SSE2, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 16), 1));
          processors16.push_back(Filtering::Processor<Processor16>(morpho_10_sse4, Constraint(CPU_SSE4_1, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 16), 2));
          processors16.push_back(Filtering::Processor<Processor16>(morpho_10_avx2, Constraint(CPU_AVX2, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 32), 3));
#endif
          break;
        case 12:
          processors16.push_back(Filtering::Processor<Processor16>(morpho_12_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0));
#ifdef INTEL_INTRINSICS
          processors16.push_back(Filtering::Processor<Processor16>(morpho_12_sse2, Constraint(CPU_SSE2, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 16), 1));
          processors16.push_back(Filtering::Processor<Processor16>(morpho_12_sse4, Constraint(CPU_SSE4_1, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 16), 2));
          processors16.push_back(Filtering::Processor<Processor16>(morpho_12_avx2, Constraint(CPU_AVX2, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 32), 3));
#endif
          break;
        case 14:
          processors16.push_back(Filtering::Processor<Processor16>(morpho_14_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0));
#ifdef INTEL_INTRINSICS
          processors16.push_back(Filtering::Processor<Processor16>(morpho_14_sse2, Constraint(CPU_SSE2, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 16), 1));
          processors16.push_back(Filtering::Processor<Processor16>(morpho_14_sse4, Constraint(CPU_SSE4_1, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 16), 2));
          processors16.push_back(Filtering::Processor<Processor16>(morpho_14_avx2, Constraint(CPU_AVX2, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 32), 3));
#endif
          break;
        case 16:
          processors16.push_back(Filtering::Processor<Processor16>(morpho_16_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0));
#ifdef INTEL_INTRINSICS
          processors16.push_back(Filtering::Processor<Processor16>(morpho_16_sse2, Constraint(CPU_SSE2, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 16), 1));
          processors16.push_back(Filtering::Processor<Processor16>(morpho_16_sse4, Constraint(CPU_SSE4_1, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 16), 2));
          processors16.push_back(Filtering::Processor<Processor16>(morpho_16_avx2, Constraint(CPU_AVX2, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 32), 3));
#endif
          break;
        case 32: 
          processors32.push_back(Filtering::Processor<Processor32>(morpho_32_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0));
#ifdef INTEL_INTRINSICS
          processors32.push_back(Filtering::Processor<Processor32>(morpho_32_sse2, Constraint(CPU_SSE2, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 16), 1));
          processors32.push_back(Filtering::Processor<Processor32>(morpho_32_avx, Constraint(CPU_AVX, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE ,32), 2));
#endif
          break;
        }
      }
      else if (parameters["mode"].toString() == "prewitt")
      {
        print(LOG_DEBUG, "Edge : using prewitt detector");
        switch (bits_per_pixel) {
        case 8:
          processors.push_back(Filtering::Processor<Processor>(prewitt_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0));
#ifdef INTEL_INTRINSICS
          processors.push_back(Filtering::Processor<Processor>(prewitt_sse2, Constraint(CPU_SSE2, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 17), 1));
          processors.push_back(Filtering::Processor<Processor>(prewitt_ssse3, Constraint(CPU_SSSE3, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 17), 2));
          processors.push_back(Filtering::Processor<Processor>(prewitt_avx2, Constraint(CPU_AVX2, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 33), 3));
#endif
          break;
        case 10: 
          processors16.push_back(Filtering::Processor<Processor16>(prewitt_10_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0));
#ifdef INTEL_INTRINSICS
          processors16.push_back(Filtering::Processor<Processor16>(prewitt_10_sse2, Constraint(CPU_SSE2, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 16), 1));
          processors16.push_back(Filtering::Processor<Processor16>(prewitt_10_sse4, Constraint(CPU_SSE4_1, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 16), 2));
          processors16.push_back(Filtering::Processor<Processor16>(prewitt_10_avx2, Constraint(CPU_AVX2, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 32), 3));
#endif
          break;
        case 12:
          processors16.push_back(Filtering::Processor<Processor16>(prewitt_12_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0));
#ifdef INTEL_INTRINSICS
          processors16.push_back(Filtering::Processor<Processor16>(prewitt_12_sse2, Constraint(CPU_SSE2, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 16), 1));
          processors16.push_back(Filtering::Processor<Processor16>(prewitt_12_sse4, Constraint(CPU_SSE4_1, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 16), 2));
          processors16.push_back(Filtering::Processor<Processor16>(prewitt_12_avx2, Constraint(CPU_AVX2, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 32), 3));
#endif
          break;
        case 14:
          processors16.push_back(Filtering::Processor<Processor16>(prewitt_14_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0));
#ifdef INTEL_INTRINSICS
          processors16.push_back(Filtering::Processor<Processor16>(prewitt_14_sse2, Constraint(CPU_SSE2, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 16), 1));
          processors16.push_back(Filtering::Processor<Processor16>(prewitt_14_sse4, Constraint(CPU_SSE4_1, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 16), 2));
          processors16.push_back(Filtering::Processor<Processor16>(prewitt_14_avx2, Constraint(CPU_AVX2, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 32), 3));
#endif
          break;
        case 16:
          processors16.push_back(Filtering::Processor<Processor16>(prewitt_16_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0));
#ifdef INTEL_INTRINSICS
          processors16.push_back(Filtering::Processor<Processor16>(prewitt_16_sse2, Constraint(CPU_SSE2, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 16), 1));
          processors16.push_back(Filtering::Processor<Processor16>(prewitt_16_sse4, Constraint(CPU_SSE4_1, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 16), 2));
          processors16.push_back(Filtering::Processor<Processor16>(prewitt_16_avx2, Constraint(CPU_AVX2, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 32), 3));
#endif
          break;
        case 32: 
          processors32.push_back(Filtering::Processor<Processor32>(prewitt_32_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0));
#ifdef INTEL_INTRINSICS
          processors32.push_back(Filtering::Processor<Processor32>(prewitt_32_sse2, Constraint(CPU_SSE2, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 16), 1));
          processors32.push_back(Filtering::Processor<Processor32>(prewitt_32_avx, Constraint(CPU_AVX, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 32), 2));
#endif
          break;
        }
      }
      else if (parameters["mode"].toString() == "hprewitt")
      {
        print(LOG_DEBUG, "Edge : using hprewitt detector");
        switch (bits_per_pixel) {
        case 8:
          processors.push_back(Filtering::Processor<Processor>(half_prewitt_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0));
#ifdef INTEL_INTRINSICS
          processors.push_back(Filtering::Processor<Processor>(half_prewitt_sse2, Constraint(CPU_SSE2, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 17), 1));
          processors.push_back(Filtering::Processor<Processor>(half_prewitt_ssse3, Constraint(CPU_SSSE3, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 17), 2));
          processors.push_back(Filtering::Processor<Processor>(half_prewitt_avx2, Constraint(CPU_AVX2, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 33), 3));
#endif
          break;
        case 10: 
          processors16.push_back(Filtering::Processor<Processor16>(half_prewitt_10_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0));
#ifdef INTEL_INTRINSICS
          processors16.push_back(Filtering::Processor<Processor16>(half_prewitt_10_sse2, Constraint(CPU_SSE2, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 16), 1));
          processors16.push_back(Filtering::Processor<Processor16>(half_prewitt_10_sse4, Constraint(CPU_SSE4_1, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 16), 2));
          processors16.push_back(Filtering::Processor<Processor16>(half_prewitt_10_avx2, Constraint(CPU_AVX2, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 32), 3));
#endif
          break;
        case 12:
          processors16.push_back(Filtering::Processor<Processor16>(half_prewitt_12_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0));
#ifdef INTEL_INTRINSICS
          processors16.push_back(Filtering::Processor<Processor16>(half_prewitt_12_sse2, Constraint(CPU_SSE2, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 16), 1));
          processors16.push_back(Filtering::Processor<Processor16>(half_prewitt_12_sse4, Constraint(CPU_SSE4_1, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 16), 2));
          processors16.push_back(Filtering::Processor<Processor16>(half_prewitt_12_avx2, Constraint(CPU_AVX2, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 32), 3));
#endif
          break;
        case 14:
          processors16.push_back(Filtering::Processor<Processor16>(half_prewitt_14_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0));
#ifdef INTEL_INTRINSICS
          processors16.push_back(Filtering::Processor<Processor16>(half_prewitt_14_sse2, Constraint(CPU_SSE2, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 16), 1));
          processors16.push_back(Filtering::Processor<Processor16>(half_prewitt_14_sse4, Constraint(CPU_SSE4_1, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 16), 2));
          processors16.push_back(Filtering::Processor<Processor16>(half_prewitt_14_avx2, Constraint(CPU_AVX2, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 32), 3));
#endif
          break;
        case 16:
          processors16.push_back(Filtering::Processor<Processor16>(half_prewitt_16_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0));
#ifdef INTEL_INTRINSICS
          processors16.push_back(Filtering::Processor<Processor16>(half_prewitt_16_sse2, Constraint(CPU_SSE2, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 16), 1));
          processors16.push_back(Filtering::Processor<Processor16>(half_prewitt_16_sse4, Constraint(CPU_SSE4_1, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 16), 2));
          processors16.push_back(Filtering::Processor<Processor16>(half_prewitt_16_avx2, Constraint(CPU_AVX2, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 32), 3));
#endif
          break;
        case 32:
          processors32.push_back(Filtering::Processor<Processor32>(half_prewitt_32_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0));
#ifdef INTEL_INTRINSICS
          processors32.push_back(Filtering::Processor<Processor32>(half_prewitt_32_sse2, Constraint(CPU_SSE2, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 16), 1));
          processors32.push_back(Filtering::Processor<Processor32>(half_prewitt_32_avx, Constraint(CPU_AVX, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 32), 2));
#endif
          break;
        }
      }
      else
      {
         print(LOG_DEBUG, "Edge : using custom detector");
         auto coeffs = Parser::getDefaultParser().parse(parameters["mode"].toString(), Parser::SYMBOL_SEPARATORS).getExpression();
         bool isAsmOk = true;
         if (isFloat) {
           memset(matrix_f, 0, sizeof(matrix_f));
           float nNegative = 0, nPositive = 0;
           for (int i = 0; i < 9; i++)
           {
             float coefficient;

             if (!coeffs.size())
             {
               error = "invalid mode";
               return;
             }

             print(LOG_DEBUG, "%i\n", coeffs.size());
             matrix_f[i] = coefficient = static_cast<float>(coeffs.front().getValue(0, 0, 0));

             nNegative = nNegative + (coefficient < 0 ? -coefficient : 0);
             nPositive = nPositive + (coefficient > 0 ? +coefficient : 0);
             coeffs.pop_front();
           }
           float nSum = max<float>(nNegative, nPositive);

           // when there is a 10th value, it overwrites max(possum, negsum)
           if (coeffs.size())
             nSum = static_cast<float>(coeffs.front().getValue(0, 0, 0));

           /* disable asm if sum of coefficients indicates a risk of overflow */
#if 0
           // float: no overflow
           if (nNegative > 128 || nPositive > 128)
             isAsmOk = false;

           // float: no integer division by shift
           /* find the upper power of 2, if possible */
           int i;
           for (i = 0; i < 15; i++)
           {
             if ((1 << i) >= nSum)
             {
               if (!coeffs.size())
                 nSum = (float)(1 << i);
               break;
             }
           }
#endif

           matrix_f[9] = static_cast<float>(nSum);

#if 0
           // float: no integer division by shift
           if (i >= 15)
             error = "Too high divisor, please specify a divisor between 1 and 32767";
           if (1 << i != matrix_f[9])
             isAsmOk = false;
#endif
           if (matrix_f[9] <= 0)
             error = "Divisor must be positive";
         }
         else { // integer matrix
           memset(matrix, 0, sizeof(matrix));
           int nNegative = 0, nPositive = 0;
           for (int i = 0; i < 9; i++)
           {
             int16_t coefficient;

             if (!coeffs.size())
             {
               error = "invalid mode";
               return;
             }

             print(LOG_DEBUG, "%i\n", coeffs.size());
             matrix[i] = coefficient = static_cast<int16_t>(coeffs.front().getValue(0, 0, 0));

             nNegative += coefficient < 0 ? -coefficient : 0;
             nPositive += coefficient > 0 ? +coefficient : 0;
             coeffs.pop_front();
           }
           int nSum = max<int>(nNegative, nPositive);

           if (coeffs.size())
             nSum = static_cast<int16_t>(coeffs.front().getValue(0, 0, 0));

           /* disable asm if sum of coefficients indicates a risk of overflow */
           if (nNegative > 128 || nPositive > 128)
             isAsmOk = false;

           // for SIMD division means bit-shifts, this is possible only when the divisor is power of two
           // all other cases go to non-SIMD C version

           /* find the upper power of 2, if possible */
           int i;
           for (i = 0; i < 15; i++)
           {
             if ((1 << i) >= nSum)
             {
               if (!coeffs.size())
                 nSum = 1 << i;
               break;
             }
           }

           matrix[9] = static_cast<int16_t>(nSum);

           if (i >= 15)
             error = "Too high divisor, please specify a divisor between 1 and 32767";
           if (1 << i != matrix[9]) // when divisor is not power of two: C version instead of asm
             isAsmOk = false;
           if (matrix[9] <= 0)
             error = "Divisor must be positive";
         }

         print(LOG_DEBUG, "Edge : %3i %3i %3i\n"
                        "       %3i %3i %3i\n"
                        "       %3i %3i %3i\n"
                        "normalize: %3i", matrix[0], matrix[1], matrix[2], matrix[3], matrix[4], matrix[5], matrix[6], matrix[7], matrix[8], matrix[9]);
         switch(bits_per_pixel) {
         case 8: 
           processors.push_back(Filtering::Processor<Processor>(convolution_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0));
           if (isAsmOk)
           {
#ifdef INTEL_INTRINSICS
             processors.push_back(Filtering::Processor<Processor>(convolution_sse2, Constraint(CPU_SSE2, 8, 1, 1, 17), 2)); // why is it mod 8????
             processors.push_back(Filtering::Processor<Processor>(convolution_avx2, Constraint(CPU_AVX2, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 33), 3));
#endif
           }
           break;
         case 10: 
           processors16.push_back(Filtering::Processor<Processor16>(convolution_10_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0));
           if (isAsmOk)
           {
#ifdef INTEL_INTRINSICS
             processors16.push_back(Filtering::Processor<Processor16>(convolution_10_sse2, Constraint(CPU_SSE2, 8, 1, 1, 1), 1));
             processors16.push_back(Filtering::Processor<Processor16>(convolution_10_sse4, Constraint(CPU_SSE4_1, 8, 1, 1, 1), 2));
             processors16.push_back(Filtering::Processor<Processor16>(convolution_10_avx2, Constraint(CPU_AVX2, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 32), 3));
#endif
           }
           break;
         case 12:
           processors16.push_back(Filtering::Processor<Processor16>(convolution_12_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0));
           if (isAsmOk)
           {
#ifdef INTEL_INTRINSICS
             processors16.push_back(Filtering::Processor<Processor16>(convolution_12_sse2, Constraint(CPU_SSE2, 8, 1, 1, 1), 1));
             processors16.push_back(Filtering::Processor<Processor16>(convolution_12_sse4, Constraint(CPU_SSE4_1, 8, 1, 1, 1), 2));
             processors16.push_back(Filtering::Processor<Processor16>(convolution_12_avx2, Constraint(CPU_AVX2, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 32), 3));
#endif
           }
           break;
         case 14:
           processors16.push_back(Filtering::Processor<Processor16>(convolution_14_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0));
           if (isAsmOk)
           {
#ifdef INTEL_INTRINSICS
             processors16.push_back(Filtering::Processor<Processor16>(convolution_14_sse2, Constraint(CPU_SSE2, 8, 1, 1, 1), 1));
             processors16.push_back(Filtering::Processor<Processor16>(convolution_14_sse4, Constraint(CPU_SSE4_1, 8, 1, 1, 1), 2));
             processors16.push_back(Filtering::Processor<Processor16>(convolution_14_avx2, Constraint(CPU_AVX2, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 32), 3));
#endif
           }
           break;
         case 16:
           processors16.push_back(Filtering::Processor<Processor16>(convolution_16_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0));
           if (isAsmOk)
           {
#ifdef INTEL_INTRINSICS
             processors16.push_back(Filtering::Processor<Processor16>(convolution_16_sse2, Constraint(CPU_SSE2, 8, 1, 1, 1), 1));
             processors16.push_back(Filtering::Processor<Processor16>(convolution_16_sse4, Constraint(CPU_SSE4_1, 8, 1, 1, 1), 2));
             processors16.push_back(Filtering::Processor<Processor16>(convolution_16_avx2, Constraint(CPU_AVX2, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 32), 3));
#endif
           }
           break;
         case 32:
           processors32.push_back(Filtering::Processor<Processor32>(convolution_32_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0));
#ifdef INTEL_INTRINSICS
           processors32.push_back(Filtering::Processor<Processor32>(convolution_32_sse2, Constraint(CPU_SSE2, MODULO_8, MODULO_NONE, ALIGNMENT_NONE, 16), 1));
           processors32.push_back(Filtering::Processor<Processor32>(convolution_32_avx, Constraint(CPU_AVX, MODULO_8, MODULO_NONE, ALIGNMENT_NONE, 32), 2));
#endif
           break;
         }
      }
   }

   InputConfiguration &input_configuration() const override { return OneFrame(); }

   static Signature filter_signature()
   {
      Signature signature = "mt_edge"; // common for any bit depths

      signature.add(Parameter(TYPE_CLIP, "", false));
      signature.add(Parameter(String("sobel"), "mode", false));
      signature.add(Parameter(TYPE_FLOAT, "thY1", true)); // default 10 must be scaled + prepare float support
      signature.add(Parameter(TYPE_FLOAT, "thY2", true));
      signature.add(Parameter(TYPE_FLOAT, "thC1", true));
      signature.add(Parameter(TYPE_FLOAT, "thC2", true));

      add_defaults( signature );

      return signature;
   }

};

} } } } }

#endif