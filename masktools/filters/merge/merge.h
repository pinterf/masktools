#ifndef __Mt_Merge8_H__
#define __Mt_Merge8_H__

#include "../../common/base/filter.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Merge {

/* 8 bit */
typedef void(Processor)(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch,
                        const uint8_t *pSrc2, ptrdiff_t nSrc2Pitch, int nWidth, int nHeight);
typedef void(Processor16)(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch,
  const uint8_t *pSrc2, ptrdiff_t nSrc2Pitch, int nWidth, int nHeight, int bits_per_pixel);
typedef void(Processor32)(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch,
  const uint8_t *pSrc2, ptrdiff_t nSrc2Pitch, int nWidth, int nHeight);

extern Processor *merge_c;
extern Processor *merge_luma_420_c;
extern Processor *merge_luma_420_mpeg2_c;
extern Processor *merge_luma_420_topleft_c;
extern Processor *merge_luma_422_c;
extern Processor *merge_luma_422_mpeg2_c;
extern Processor *merge_luma_411_c;
extern Processor *merge_luma_440_c;
extern Processor *merge_luma_440_topleft_c;

extern Processor *merge_sse2;
extern Processor *merge_luma_420_sse2;
extern Processor *merge_luma_420_mpeg2_sse2;
extern Processor* merge_luma_420_topleft_sse2;
extern Processor *merge_luma_422_sse2;
extern Processor *merge_luma_422_mpeg2_sse2;
extern Processor *merge_luma_411_sse2;
extern Processor *merge_luma_440_sse2;
extern Processor *merge_luma_440_topleft_sse2;

extern Processor *merge_avx2;
extern Processor *merge_luma_420_avx2;
extern Processor *merge_luma_422_avx2;
extern Processor *merge_luma_411_avx2;
extern Processor *merge_luma_440_avx2;
extern Processor *merge_luma_440_topleft_avx2;
extern Processor *merge_luma_420_mpeg2_avx2;
extern Processor *merge_luma_420_topleft_avx2;
extern Processor *merge_luma_422_mpeg2_avx2;
#ifdef NEON_INTRINSICS
extern Processor *merge_neon;
extern Processor *merge_luma_420_neon;
extern Processor *merge_luma_422_neon;
extern Processor *merge_luma_411_neon;
extern Processor *merge_luma_440_neon;
extern Processor *merge_luma_440_topleft_neon;
#endif

/* 10-16 bit, bits_per_pixel at run time */
extern Processor16 *merge16_c;
extern Processor16 *merge16_luma_420_c;
extern Processor16 *merge16_luma_420_mpeg2_c;
extern Processor16 *merge16_luma_420_topleft_c;
extern Processor16 *merge16_luma_422_c;
extern Processor16 *merge16_luma_422_mpeg2_c;
extern Processor16 *merge16_luma_411_c;
extern Processor16 *merge16_luma_440_c;
extern Processor16 *merge16_luma_440_topleft_c;

extern Processor16 *merge16_sse2;
extern Processor16 *merge16_luma_420_sse2;
extern Processor16 *merge16_luma_420_mpeg2_sse2;
extern Processor16 *merge16_luma_420_topleft_sse2;
extern Processor16 *merge16_luma_422_sse2;
extern Processor16 *merge16_luma_422_mpeg2_sse2;
extern Processor16 *merge16_luma_411_sse2;
extern Processor16 *merge16_luma_440_sse2;
extern Processor16 *merge16_luma_440_topleft_sse2;

extern Processor16 *merge16_avx2;
extern Processor16 *merge16_luma_420_avx2;
extern Processor16 *merge16_luma_422_avx2;
extern Processor16 *merge16_luma_420_mpeg2_avx2;
extern Processor16 *merge16_luma_420_topleft_avx2;
extern Processor16 *merge16_luma_422_mpeg2_avx2;
extern Processor16 *merge16_luma_411_avx2;
extern Processor16 *merge16_luma_440_avx2;
extern Processor16 *merge16_luma_440_topleft_avx2;
#ifdef NEON_INTRINSICS
extern Processor16 *merge16_neon;
extern Processor16 *merge16_luma_420_neon;
extern Processor16 *merge16_luma_422_neon;
extern Processor16 *merge16_luma_411_neon;
extern Processor16 *merge16_luma_440_neon;
extern Processor16 *merge16_luma_440_topleft_neon;
#endif

/* 32 bit */
Processor32 merge32_c;
Processor32 merge32_luma_420_c;
Processor32 merge32_luma_420_mpeg2_c;
Processor32 merge32_luma_420_topleft_c;
Processor32 merge32_luma_422_c;
Processor32 merge32_luma_422_mpeg2_c;
Processor32 merge32_luma_411_c;
extern Processor32 *merge32_luma_440_c;
extern Processor32 *merge32_luma_440_topleft_c;

extern Processor32 *merge32_sse2;
extern Processor32 *merge32_luma_440_sse2;
extern Processor32 *merge32_luma_440_topleft_sse2;
extern Processor32 *merge32_luma_420_sse2;
extern Processor32 *merge32_luma_420_mpeg2_sse2;
extern Processor32 *merge32_luma_422_sse2;
extern Processor32 *merge32_luma_422_mpeg2_sse2;
extern Processor32 *merge32_avx2;
extern Processor32 *merge32_luma_440_avx2;
extern Processor32 *merge32_luma_440_topleft_avx2;
extern Processor32 *merge32_luma_420_avx2;
extern Processor32 *merge32_luma_420_mpeg2_avx2;
extern Processor32 *merge32_luma_422_avx2;
extern Processor32 *merge32_luma_422_mpeg2_avx2;
#ifdef NEON_INTRINSICS
extern Processor32 *merge32_neon;
extern Processor32 *merge32_luma_440_neon;
extern Processor32 *merge32_luma_440_topleft_neon;
extern Processor32 *merge32_luma_420_neon;
extern Processor32 *merge32_luma_422_neon;
#endif

class Merge : public MaskTools::Filter
{

   bool use_luma;
   ProcessorList<Processor> processors;
   ProcessorList<Processor> chroma_processors;
   ProcessorList<Processor16> processors16;
   ProcessorList<Processor16> chroma_processors16;
   ProcessorList<Processor32> processors32;
   ProcessorList<Processor32> chroma_processors32;

protected:

  virtual void process(int n, const Plane<uint8_t> &dst, int nPlane, const Frame<const uint8_t> frames[4], const Constraint constraints[4]) override
  {
    UNUSED(n);
    int bits_per_pixel = nBitDepth;
    if (parameters["stacked"].toBool())
      bits_per_pixel = 16;
    if (use_luma && ((nPlane == 1 || nPlane == 2))) {
      if (!(width_ratios[1] == 1 && height_ratios[1] == 1)) {
        // Subsampled chroma.
        switch (bits_per_pixel) {
        case 8:
          chroma_processors.best_processor(constraints[nPlane])(dst.data(), dst.pitch(),
            frames[0].plane(nPlane).data(), frames[0].plane(nPlane).pitch(),
            frames[1].plane(0).data(), frames[1].plane(0).pitch(),
            dst.width(), dst.height());
          break;
        case 10: case 12: case 14: case 16:
          chroma_processors16.best_processor(constraints[nPlane])(dst.data(), dst.pitch(),
            frames[0].plane(nPlane).data(), frames[0].plane(nPlane).pitch(),
            frames[1].plane(0).data(), frames[1].plane(0).pitch(),
            dst.width(), dst.height(), bits_per_pixel);
          break;
        case 32:
          chroma_processors32.best_processor(constraints[nPlane])(dst.data(), dst.pitch(),
            frames[0].plane(nPlane).data(), frames[0].plane(nPlane).pitch(),
            frames[1].plane(0).data(), frames[1].plane(0).pitch(),
            dst.width(), dst.height());
          break;
        }
      }
      else {
        // 444
        switch (bits_per_pixel) {
        case 8:
          processors.best_processor(constraints[nPlane])(dst.data(), dst.pitch(),
            frames[0].plane(nPlane).data(), frames[0].plane(nPlane).pitch(),
            frames[1].plane(0).data(), frames[1].plane(0).pitch(),
            dst.width(), dst.height());
          break;
        case 10: case 12: case 14: case 16:
          processors16.best_processor(constraints[nPlane])(dst.data(), dst.pitch(),
            frames[0].plane(nPlane).data(), frames[0].plane(nPlane).pitch(),
            frames[1].plane(0).data(), frames[1].plane(0).pitch(),
            dst.width(), dst.height(), bits_per_pixel);
          break;
        case 32:
          processors32.best_processor(constraints[nPlane])(dst.data(), dst.pitch(),
            frames[0].plane(nPlane).data(), frames[0].plane(nPlane).pitch(),
            frames[1].plane(0).data(), frames[1].plane(0).pitch(),
            dst.width(), dst.height());
          break;
        }
      }
    }
    else { // Y or Alpha plane or luma==false
      switch (bits_per_pixel) {
      case 8:
        processors.best_processor(constraints[nPlane])(dst.data(), dst.pitch(),
          frames[0].plane(nPlane).data(), frames[0].plane(nPlane).pitch(),
          frames[1].plane(nPlane).data(), frames[1].plane(nPlane).pitch(),
          dst.width(), dst.height());
        break;
      case 10: case 12: case 14: case 16:
        processors16.best_processor(constraints[nPlane])(dst.data(), dst.pitch(),
          frames[0].plane(nPlane).data(), frames[0].plane(nPlane).pitch(),
          frames[1].plane(nPlane).data(), frames[1].plane(nPlane).pitch(),
          dst.width(), dst.height(), bits_per_pixel);
        break;
      case 32:
        processors32.best_processor(constraints[nPlane])(dst.data(), dst.pitch(),
          frames[0].plane(nPlane).data(), frames[0].plane(nPlane).pitch(),
          frames[1].plane(nPlane).data(), frames[1].plane(nPlane).pitch(),
          dst.width(), dst.height());
        break;
      }
    }
  }

public:
   Merge(const Parameters &parameters, CpuFlags cpuFlags) : MaskTools::Filter( parameters, FilterProcessingType::INPLACE, (CpuFlags)cpuFlags)
   {
      bool isStacked = parameters["stacked"].toBool();
      int bits_per_pixel = nBitDepth;

      bool is420 = width_ratios[1] == 2 && height_ratios[1] == 2;
      bool is422 = width_ratios[1] == 2 && height_ratios[1] == 1;
      bool is411 = width_ratios[1] == 4 && height_ratios[1] == 1;
      bool is440 = width_ratios[1] == 1 && height_ratios[1] == 2;

      String cplace = parameters["cplace"].toString();
      bool isMpeg2 = false;
      bool isTopLeft = false;
      if (cplace != "mpeg1" && cplace != "mpeg2" && cplace != "topleft" && cplace != "top_left") {
        error = "cplace: only mpeg1, mpeg2 and top_left allowed";
        return;
      }
      isMpeg2 = cplace == "mpeg2" && (is420 || is422);
      isTopLeft = (cplace == "topleft" || cplace == "top_left") && (is420 || is440);

      if (isStacked && bits_per_pixel != 8) {
        error = "Stacked specified for a non-8 bit clip";
        return;
      }
      if (isStacked)
        bits_per_pixel = 16; // processed as native 16 bit

      use_luma = parameters["luma"].toBool();
      // Y and YA have no chroma: luma=true must not force processing of YA's alpha.
      if (nPlanes < 3) {
        use_luma = false;
        operators[1] = operators[2] = NONE;
      }

      // childs[2]: the mask
      if (childs[0]->bit_depth() != childs[1]->bit_depth() || childs[0]->bit_depth() != childs[2]->bit_depth()) {
        error = "clips should have identical bit depths";
        return;
      }

      // error when when mask clip is greyscale and 'u' or 'v' or 'a' processing is set (2.2.27-)
      if (!use_luma && childs[2]->plane_count() == 1 && (operators[1] == PROCESS || operators[2] == PROCESS || operators[3] == PROCESS)) {
        error = "mask clip is greyscale and 'u' or 'v' or 'a' processing is set with luma=false";
        return;
      }

      if (use_luma) {
         // PF: implement use_luma for 422 and 411 clips
        if ((childs[0]->width_ratio(1) != childs[1]->width_ratio(1)) || (childs[0]->height_ratio(1) != childs[1]->height_ratio(1))) {
            error = "clips should have identical colorspace";
            return;
        }

        /* if "luma" is set, we force the chroma processing. Much more handy */
        operators[1] = operators[2] = PROCESS;
        /* no need to change U/V default processing, because of in place filter */
      }

      for (int i = 0; i < nPlanes; ++i) {
        const int p = plane_index(i);
        if (operators[p] != PROCESS)
          continue;
        if (!childs[1]->has_plane(p)) {
          error = "source clip lacks a processed plane";
          return;
        }
        // luma=true substitutes the Y mask only for U/V; alpha still needs an alpha mask.
        const int mask_plane = use_luma && (p == 1 || p == 2) ? 0 : p;
        if (!childs[2]->has_plane(mask_plane)) {
          error = "mask clip lacks a processed plane";
          return;
        }
      }

      switch (bits_per_pixel) {
      case 8:
        /* add the processors */
        processors.push_back(Filtering::Processor<Processor>(merge_c, Constraint(CPU_NONE), 0));
#ifdef INTEL_INTRINSICS
        processors.push_back(Filtering::Processor<Processor>(merge_sse2, Constraint(CPU_SSE2), 1));
        processors.push_back(Filtering::Processor<Processor>(merge_avx2, Constraint(CPU_AVX2), 5));
#endif
#ifdef NEON_INTRINSICS
        processors.push_back(Filtering::Processor<Processor>(merge_neon, Constraint(CPU_NEON), 1));
#endif

        /* add the chroma processors */
        // Reduced luma masks for subsampled chroma.
        if (is440) {
          chroma_processors.push_back(Filtering::Processor<Processor>(
            isTopLeft ? merge_luma_440_topleft_c : merge_luma_440_c, Constraint(CPU_NONE), 0));
          MT_X86(chroma_processors.push_back(Filtering::Processor<Processor>(
            isTopLeft ? merge_luma_440_topleft_sse2 : merge_luma_440_sse2, Constraint(CPU_SSE2), 1)));
          MT_X86(chroma_processors.push_back(Filtering::Processor<Processor>(
            isTopLeft ? merge_luma_440_topleft_avx2 : merge_luma_440_avx2, Constraint(CPU_AVX2), 5)));
          MT_NEON(chroma_processors.push_back(Filtering::Processor<Processor>(
            isTopLeft ? merge_luma_440_topleft_neon : merge_luma_440_neon, Constraint(CPU_NEON), 1)));
        }
        else if (is420) {
          if (isTopLeft) {
            chroma_processors.push_back(Filtering::Processor<Processor>(merge_luma_420_topleft_c, Constraint(CPU_NONE), 0));
#ifdef INTEL_INTRINSICS
            chroma_processors.push_back(Filtering::Processor<Processor>(merge_luma_420_topleft_sse2, Constraint(CPU_SSE2), 1));
            chroma_processors.push_back(Filtering::Processor<Processor>(merge_luma_420_topleft_avx2, Constraint(CPU_AVX2), 5));
#endif

          } else if (isMpeg2) {
            chroma_processors.push_back(Filtering::Processor<Processor>(merge_luma_420_mpeg2_c, Constraint(CPU_NONE), 0));
#ifdef INTEL_INTRINSICS
            chroma_processors.push_back(Filtering::Processor<Processor>(merge_luma_420_mpeg2_sse2, Constraint(CPU_SSE2), 1));
            chroma_processors.push_back(Filtering::Processor<Processor>(merge_luma_420_mpeg2_avx2, Constraint(CPU_AVX2), 5));
#endif
          }
          else {
            chroma_processors.push_back(Filtering::Processor<Processor>(merge_luma_420_c, Constraint(CPU_NONE), 0));
#ifdef INTEL_INTRINSICS
            chroma_processors.push_back(Filtering::Processor<Processor>(merge_luma_420_sse2, Constraint(CPU_SSE2), 1));
            chroma_processors.push_back(Filtering::Processor<Processor>(merge_luma_420_avx2, Constraint(CPU_AVX2), 5));
#endif
#ifdef NEON_INTRINSICS
            chroma_processors.push_back(Filtering::Processor<Processor>(merge_luma_420_neon, Constraint(CPU_NEON), 1));
#endif
          }
        }
        else if (is422) { // 422
          if (isMpeg2) {
            chroma_processors.push_back(Filtering::Processor<Processor>(merge_luma_422_mpeg2_c, Constraint(CPU_NONE), 0));
#ifdef INTEL_INTRINSICS
            chroma_processors.push_back(Filtering::Processor<Processor>(merge_luma_422_mpeg2_sse2, Constraint(CPU_SSE2), 1));
            chroma_processors.push_back(Filtering::Processor<Processor>(merge_luma_422_mpeg2_avx2, Constraint(CPU_AVX2), 5));
#endif
          }
          else {
            chroma_processors.push_back(Filtering::Processor<Processor>(merge_luma_422_c, Constraint(CPU_NONE), 0));
#ifdef INTEL_INTRINSICS
            chroma_processors.push_back(Filtering::Processor<Processor>(merge_luma_422_sse2, Constraint(CPU_SSE2), 1));
            chroma_processors.push_back(Filtering::Processor<Processor>(merge_luma_422_avx2, Constraint(CPU_AVX2), 5));
#endif
#ifdef NEON_INTRINSICS
            chroma_processors.push_back(Filtering::Processor<Processor>(merge_luma_422_neon, Constraint(CPU_NEON), 1));
#endif
          }
        }
        else {
          chroma_processors.push_back(Filtering::Processor<Processor>(merge_luma_411_c, Constraint(CPU_NONE), 0));
#ifdef INTEL_INTRINSICS
          chroma_processors.push_back(Filtering::Processor<Processor>(merge_luma_411_sse2, Constraint(CPU_SSE2), 1));
          chroma_processors.push_back(Filtering::Processor<Processor>(merge_luma_411_avx2, Constraint(CPU_AVX2), 5));
#endif
#ifdef NEON_INTRINSICS
          chroma_processors.push_back(Filtering::Processor<Processor>(merge_luma_411_neon, Constraint(CPU_NEON), 1));
#endif
        }
        break;
      case 10: case 12: case 14: case 16:
        processors16.push_back(Filtering::Processor<Processor16>(merge16_c, Constraint(CPU_NONE), 0));
        MT_X86(processors16.push_back(Filtering::Processor<Processor16>(merge16_sse2, Constraint(CPU_SSE2), 1)));
        MT_X86(processors16.push_back(Filtering::Processor<Processor16>(merge16_avx2, Constraint(CPU_AVX2), 3)));
        MT_NEON(processors16.push_back(Filtering::Processor<Processor16>(merge16_neon, Constraint(CPU_NEON), 1)));
        /* Reduced luma masks for subsampled chroma. */
        if (is440) {
          chroma_processors16.push_back(Filtering::Processor<Processor16>(
            isTopLeft ? merge16_luma_440_topleft_c : merge16_luma_440_c, Constraint(CPU_NONE), 0));
          MT_X86(chroma_processors16.push_back(Filtering::Processor<Processor16>(
            isTopLeft ? merge16_luma_440_topleft_sse2 : merge16_luma_440_sse2, Constraint(CPU_SSE2), 1)));
          MT_X86(chroma_processors16.push_back(Filtering::Processor<Processor16>(
            isTopLeft ? merge16_luma_440_topleft_avx2 : merge16_luma_440_avx2, Constraint(CPU_AVX2), 4)));
          MT_NEON(chroma_processors16.push_back(Filtering::Processor<Processor16>(
            isTopLeft ? merge16_luma_440_topleft_neon : merge16_luma_440_neon, Constraint(CPU_NEON), 1)));
        }
        else if (is411) {
          chroma_processors16.push_back(Filtering::Processor<Processor16>(merge16_luma_411_c, Constraint(CPU_NONE), 0));
          MT_X86(chroma_processors16.push_back(Filtering::Processor<Processor16>(merge16_luma_411_sse2, Constraint(CPU_SSE2), 1)));
          MT_X86(chroma_processors16.push_back(Filtering::Processor<Processor16>(merge16_luma_411_avx2, Constraint(CPU_AVX2), 4)));
          MT_NEON(chroma_processors16.push_back(Filtering::Processor<Processor16>(merge16_luma_411_neon, Constraint(CPU_NEON), 1)));
        }
        else if (is420) {
          if (isTopLeft) {
            chroma_processors16.push_back(Filtering::Processor<Processor16>(merge16_luma_420_topleft_c, Constraint(CPU_NONE), 0));
            MT_X86(chroma_processors16.push_back(Filtering::Processor<Processor16>(merge16_luma_420_topleft_sse2, Constraint(CPU_SSE2), 1)));
            MT_X86(chroma_processors16.push_back(Filtering::Processor<Processor16>(merge16_luma_420_topleft_avx2, Constraint(CPU_AVX2), 4)));
          }
          else if (isMpeg2) {
            chroma_processors16.push_back(Filtering::Processor<Processor16>(merge16_luma_420_mpeg2_c, Constraint(CPU_NONE), 0));
            MT_X86(chroma_processors16.push_back(Filtering::Processor<Processor16>(merge16_luma_420_mpeg2_sse2, Constraint(CPU_SSE2), 1)));
            MT_X86(chroma_processors16.push_back(Filtering::Processor<Processor16>(merge16_luma_420_mpeg2_avx2, Constraint(CPU_AVX2), 4)));
          }
          else {
            chroma_processors16.push_back(Filtering::Processor<Processor16>(merge16_luma_420_c, Constraint(CPU_NONE), 0));
            MT_X86(chroma_processors16.push_back(Filtering::Processor<Processor16>(merge16_luma_420_sse2, Constraint(CPU_SSE2), 1)));
            MT_X86(chroma_processors16.push_back(Filtering::Processor<Processor16>(merge16_luma_420_avx2, Constraint(CPU_AVX2), 4)));
            MT_NEON(chroma_processors16.push_back(Filtering::Processor<Processor16>(merge16_luma_420_neon, Constraint(CPU_NEON), 1)));
          }
        }
        else { /* 422 */
          if (isMpeg2) {
            chroma_processors16.push_back(Filtering::Processor<Processor16>(merge16_luma_422_mpeg2_c, Constraint(CPU_NONE), 0));
            MT_X86(chroma_processors16.push_back(Filtering::Processor<Processor16>(merge16_luma_422_mpeg2_sse2, Constraint(CPU_SSE2), 1)));
            MT_X86(chroma_processors16.push_back(Filtering::Processor<Processor16>(merge16_luma_422_mpeg2_avx2, Constraint(CPU_AVX2), 4)));
          }
          else {
            chroma_processors16.push_back(Filtering::Processor<Processor16>(merge16_luma_422_c, Constraint(CPU_NONE), 0));
            MT_X86(chroma_processors16.push_back(Filtering::Processor<Processor16>(merge16_luma_422_sse2, Constraint(CPU_SSE2), 1)));
            MT_X86(chroma_processors16.push_back(Filtering::Processor<Processor16>(merge16_luma_422_avx2, Constraint(CPU_AVX2), 4)));
            MT_NEON(chroma_processors16.push_back(Filtering::Processor<Processor16>(merge16_luma_422_neon, Constraint(CPU_NEON), 1)));
          }
        }
        break;
      case 32:
        /* add the processors */
        processors32.push_back(Filtering::Processor<Processor32>(merge32_c, Constraint(CPU_NONE), 0));
#ifdef INTEL_INTRINSICS
        processors32.push_back(Filtering::Processor<Processor32>(merge32_sse2, Constraint(CPU_SSE2), 1));
        processors32.push_back(Filtering::Processor<Processor32>(merge32_avx2, Constraint(CPU_AVX2), 3));
#endif
#ifdef NEON_INTRINSICS
        processors32.push_back(Filtering::Processor<Processor32>(merge32_neon, Constraint(CPU_NEON), 1));
#endif

        /* add the chroma processors */
        // Float 4:1:1 currently uses C.
        if (is440) {
          chroma_processors32.push_back(Filtering::Processor<Processor32>(
            isTopLeft ? merge32_luma_440_topleft_c : merge32_luma_440_c, Constraint(CPU_NONE), 0));
          MT_X86(chroma_processors32.push_back(Filtering::Processor<Processor32>(
            isTopLeft ? merge32_luma_440_topleft_sse2 : merge32_luma_440_sse2, Constraint(CPU_SSE2), 1)));
          MT_X86(chroma_processors32.push_back(Filtering::Processor<Processor32>(
            isTopLeft ? merge32_luma_440_topleft_avx2 : merge32_luma_440_avx2, Constraint(CPU_AVX2), 3)));
          MT_NEON(chroma_processors32.push_back(Filtering::Processor<Processor32>(
            isTopLeft ? merge32_luma_440_topleft_neon : merge32_luma_440_neon, Constraint(CPU_NEON), 1)));
        }
        else if (is411) {
          chroma_processors32.push_back(Filtering::Processor<Processor32>(merge32_luma_411_c, Constraint(CPU_NONE), 0));
        }
        else if (is420) {
          if (isTopLeft) {
            chroma_processors32.push_back(Filtering::Processor<Processor32>(merge32_luma_420_topleft_c, Constraint(CPU_NONE), 0)); \
          } else if (isMpeg2) {
            chroma_processors32.push_back(Filtering::Processor<Processor32>(merge32_luma_420_mpeg2_c, Constraint(CPU_NONE), 0));
#ifdef INTEL_INTRINSICS
            chroma_processors32.push_back(Filtering::Processor<Processor32>(merge32_luma_420_mpeg2_sse2, Constraint(CPU_SSE2), 1));
            chroma_processors32.push_back(Filtering::Processor<Processor32>(merge32_luma_420_mpeg2_avx2, Constraint(CPU_AVX2), 3));
#endif
          }
          else {
            chroma_processors32.push_back(Filtering::Processor<Processor32>(merge32_luma_420_c, Constraint(CPU_NONE), 0));
#ifdef INTEL_INTRINSICS
            chroma_processors32.push_back(Filtering::Processor<Processor32>(merge32_luma_420_sse2, Constraint(CPU_SSE2), 1));
            chroma_processors32.push_back(Filtering::Processor<Processor32>(merge32_luma_420_avx2, Constraint(CPU_AVX2), 3));
#endif
#ifdef NEON_INTRINSICS
            chroma_processors32.push_back(Filtering::Processor<Processor32>(merge32_luma_420_neon, Constraint(CPU_NEON), 1));
#endif
          }
        }
        else if (is422) { // 422
          if (isMpeg2) {
            chroma_processors32.push_back(Filtering::Processor<Processor32>(merge32_luma_422_mpeg2_c, Constraint(CPU_NONE), 0));
#ifdef INTEL_INTRINSICS
            chroma_processors32.push_back(Filtering::Processor<Processor32>(merge32_luma_422_mpeg2_sse2, Constraint(CPU_SSE2), 1));
            chroma_processors32.push_back(Filtering::Processor<Processor32>(merge32_luma_422_mpeg2_avx2, Constraint(CPU_AVX2), 3));
#endif
          }
          else {
            chroma_processors32.push_back(Filtering::Processor<Processor32>(merge32_luma_422_c, Constraint(CPU_NONE), 0));
#ifdef INTEL_INTRINSICS
            chroma_processors32.push_back(Filtering::Processor<Processor32>(merge32_luma_422_sse2, Constraint(CPU_SSE2), 1));
            chroma_processors32.push_back(Filtering::Processor<Processor32>(merge32_luma_422_avx2, Constraint(CPU_AVX2), 3));
#endif
#ifdef NEON_INTRINSICS
            chroma_processors32.push_back(Filtering::Processor<Processor32>(merge32_luma_422_neon, Constraint(CPU_NEON), 1));
#endif
          }
        }
        break;
      }
   }

   InputConfiguration &input_configuration() const override { return InPlaceThreeFrame(); }

   static Signature filter_signature()
   {
      Signature signature = "mt_merge";

      signature.add( Parameter( TYPE_CLIP, "", false) );
      signature.add( Parameter( TYPE_CLIP, "", false) );
      signature.add( Parameter( TYPE_CLIP, "", false) );
      signature.add( Parameter( false, "luma", false) );

      add_defaults( signature );

      signature.add(Parameter(false, "stacked", false));
      signature.add(Parameter(String("mpeg2"), "cplace", false));
      return signature;
   }
};

} } } } // namespace Merge, Filters, MaskTools, Filtering

#endif