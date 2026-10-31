#ifndef __Mt_MorphologicFunctions16_AVX2_H__
#define __Mt_MorphologicFunctions16_AVX2_H__

#include "../../../common/utils/utils.h"
#include "../../common/simd_avx2.h"
#include "../../common/16bit.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Morphologic {
// Anonymous namespace: this header is included from files built with different per file compiler flags.
namespace {

typedef uint16_t (Operator16)(uint16_t, uint16_t, uint16_t, uint16_t, uint16_t, uint16_t, uint16_t, uint16_t, uint16_t, int); // last is nMaxDeviation
typedef __m256i (Limit16)(__m256i source, __m256i sum, __m256i deviation);
typedef void (ProcessLine16Avx2)(uint8_t *pDst, const uint8_t *pSrcp, const uint8_t *pSrc, const uint8_t *pSrcn, const __m256i &maxDeviation, int width);

enum Directions {
    Vertical = 1,
    Horizontal = 2,
    Both = Vertical | Horizontal,
    Square = 7
};


template<Border borderMode, Operator16 op>
void process_line_morpho_16_avx2_c(uint16_t *pDst, const uint16_t *pSrcp, const uint16_t *pSrc, const uint16_t *pSrcn, int maxDeviation, int width, int height, ptrdiff_t srcPitch, ptrdiff_t dstPitch, int nOrigHeight) {
    UNUSED(dstPitch); UNUSED(srcPitch); UNUSED(height); UNUSED(nOrigHeight);

    const int leftOffset = borderMode == Border::Left ? 0 : 1;
    const int rightOffset = borderMode == Border::Right ? 0 : 1;

    for (int x = 0; x < width; ++x) {
        pDst[x] = op(
            reinterpret_cast<const uint16_t*>(pSrcp)[x-leftOffset], 
            reinterpret_cast<const uint16_t*>(pSrcp)[x], 
            reinterpret_cast<const uint16_t*>(pSrcp)[x+rightOffset], 
            reinterpret_cast<const uint16_t*>(pSrc)[x-leftOffset], 
            reinterpret_cast<const uint16_t*>(pSrc)[x], 
            reinterpret_cast<const uint16_t*>(pSrc)[x+rightOffset], 
            reinterpret_cast<const uint16_t*>(pSrcn)[x-leftOffset], 
            reinterpret_cast<const uint16_t*>(pSrcn)[x], 
            reinterpret_cast<const uint16_t*>(pSrcn)[x+rightOffset], 
            maxDeviation);
    }
}

template<class T>
struct MorphologicProcessor {
    typedef void (ProcessLineC)(T *pDst, const T *pSrcp, const T *pSrc, const T *pSrcn, int maxDeviation, int width, int height, ptrdiff_t srcPitch, ptrdiff_t dstPitch, int nOrigHeight);

    template<ProcessLineC process_line_left, ProcessLineC process_line, ProcessLineC process_line_right>
    static void generic_16_c(T *pDst, ptrdiff_t nDstPitch, const T *pSrc, ptrdiff_t nSrcPitch, int nMaxDeviation, const int *pCoordinates, int nCoordinates, int nWidth, int nHeight, int nOrigHeight)
    {
        const T *pSrcp = pSrc - nSrcPitch;
        const T *pSrcn = pSrc + nSrcPitch;


        UNUSED(nCoordinates); UNUSED(pCoordinates);

        /* top-left */
        process_line_left(pDst, pSrc, pSrc, pSrcn, nMaxDeviation, 1, nHeight, nSrcPitch, nDstPitch, nOrigHeight);

        /* top */
        process_line(pDst + 1, pSrc+1, pSrc+1, pSrcn+1, nMaxDeviation, nWidth - 2, nHeight, nSrcPitch, nDstPitch, nOrigHeight);

        /* top-right */
        process_line_right(pDst + nWidth - 1, pSrc + nWidth - 1, pSrc + nWidth - 1, pSrcn + nWidth - 1, nMaxDeviation, 1, nHeight, nSrcPitch, nDstPitch, nOrigHeight);

        pDst  += nDstPitch;
        pSrcp += nSrcPitch;
        pSrc  += nSrcPitch;
        pSrcn += nSrcPitch;

        for ( int y = 1; y < nHeight - 1; y++ )
        {
            /* left */
            process_line_left(pDst, pSrcp, pSrc, pSrcn, nMaxDeviation, 1, nHeight, nSrcPitch, nDstPitch, nOrigHeight);
            /* center */
            process_line(pDst + 1, pSrcp+1, pSrc+1, pSrcn+1, nMaxDeviation, nWidth - 2, nHeight, nSrcPitch, nDstPitch, nOrigHeight);
            /* right */
            process_line_right(pDst + nWidth - 1, pSrcp + nWidth - 1, pSrc + nWidth - 1, pSrcn + nWidth - 1, nMaxDeviation, 1, nHeight, nSrcPitch, nDstPitch, nOrigHeight);

            pDst  += nDstPitch;
            pSrcp += nSrcPitch;
            pSrc  += nSrcPitch;
            pSrcn += nSrcPitch;
        }

        /* bottom-left */
        process_line_left(pDst, pSrcp, pSrc, pSrc, nMaxDeviation, 1, nHeight, nSrcPitch, nDstPitch, nOrigHeight);
        /* bottom */
        process_line(pDst + 1, pSrcp+1, pSrc+1, pSrc+1, nMaxDeviation, nWidth - 2, nHeight, nSrcPitch, nDstPitch, nOrigHeight);
        /* bottom-right */
        process_line_right(pDst + nWidth - 1, pSrcp + nWidth - 1, pSrc + nWidth - 1, pSrc + nWidth - 1, nMaxDeviation, 1, nHeight, nSrcPitch, nDstPitch, nOrigHeight);
    }
};

//-------------------------extern "C" ----------------------------------------
#if defined(GCC) || defined(CLANG)
__attribute__((__target__("avx2")))
#endif
static MT_FORCEINLINE __m256i limit_up_16_avx2(__m256i source, __m256i sum, __m256i deviation) {
  auto limit = _mm256_adds_epu16(source, deviation);
  return _mm256_min_epu16(limit, _mm256_max_epu16(source, sum));
}

#if defined(GCC) || defined(CLANG)
__attribute__((__target__("avx2")))
#endif
static MT_FORCEINLINE __m256i limit_down_16_avx2(__m256i source, __m256i sum, __m256i deviation) {
  auto limit = _mm256_subs_epu16(source, deviation);
  return _mm256_max_epu16(limit, _mm256_min_epu16(source, sum));
}


template<Border borderMode, Limit16 limit, MemoryMode mem_mode>
static void process_line_xxflate_16_avx2(uint8_t *pDst, const uint8_t *pSrcp, const uint8_t *pSrc, const uint8_t *pSrcn, const __m256i &maxDeviation, int byte_width) {
  auto zero = _mm256_setzero_si256();
  for (int x = 0; x < byte_width; x += 32) {
    auto up_left = load16_one_to_left_si256<borderMode, mem_mode>(pSrcp + x);
    auto up_center = simd256_load_si256<mem_mode>(pSrcp + x);
    auto up_right = load16_one_to_right_si256<borderMode, mem_mode>(pSrcp + x);

    auto middle_left = load16_one_to_left_si256<borderMode, mem_mode>(pSrc + x);
    auto middle_right = load16_one_to_right_si256<borderMode, mem_mode>(pSrc + x);

    auto down_left = load16_one_to_left_si256<borderMode, mem_mode>(pSrcn + x);
    auto down_center = simd256_load_si256<mem_mode>(pSrcn + x);
    auto down_right = load16_one_to_right_si256<borderMode, mem_mode>(pSrcn + x);

    auto up_left_lo = _mm256_unpacklo_epi16(up_left, zero);
    auto up_left_hi = _mm256_unpackhi_epi16(up_left, zero);

    auto up_center_lo = _mm256_unpacklo_epi16(up_center, zero);
    auto up_center_hi = _mm256_unpackhi_epi16(up_center, zero);

    auto up_right_lo = _mm256_unpacklo_epi16(up_right, zero);
    auto up_right_hi = _mm256_unpackhi_epi16(up_right, zero);

    auto middle_left_lo = _mm256_unpacklo_epi16(middle_left, zero);
    auto middle_left_hi = _mm256_unpackhi_epi16(middle_left, zero);

    auto middle_right_lo = _mm256_unpacklo_epi16(middle_right, zero);
    auto middle_right_hi = _mm256_unpackhi_epi16(middle_right, zero);

    auto down_left_lo = _mm256_unpacklo_epi16(down_left, zero);
    auto down_left_hi = _mm256_unpackhi_epi16(down_left, zero);

    auto down_center_lo = _mm256_unpacklo_epi16(down_center, zero);
    auto down_center_hi = _mm256_unpackhi_epi16(down_center, zero);

    auto down_right_lo = _mm256_unpacklo_epi16(down_right, zero);
    auto down_right_hi = _mm256_unpackhi_epi16(down_right, zero);

    auto sum_lo = _mm256_add_epi32(up_left_lo, up_center_lo);
    sum_lo = _mm256_add_epi32(sum_lo, up_right_lo);
    sum_lo = _mm256_add_epi32(sum_lo, middle_left_lo);
    sum_lo = _mm256_add_epi32(sum_lo, middle_right_lo);
    sum_lo = _mm256_add_epi32(sum_lo, down_left_lo);
    sum_lo = _mm256_add_epi32(sum_lo, down_center_lo);
    sum_lo = _mm256_add_epi32(sum_lo, down_right_lo);

    auto sum_hi = _mm256_add_epi32(up_left_hi, up_center_hi);
    sum_hi = _mm256_add_epi32(sum_hi, up_right_hi);
    sum_hi = _mm256_add_epi32(sum_hi, middle_left_hi);
    sum_hi = _mm256_add_epi32(sum_hi, middle_right_hi);
    sum_hi = _mm256_add_epi32(sum_hi, down_left_hi);
    sum_hi = _mm256_add_epi32(sum_hi, down_center_hi);
    sum_hi = _mm256_add_epi32(sum_hi, down_right_hi);

    sum_lo = _mm256_srai_epi32(sum_lo, 3);
    sum_hi = _mm256_srai_epi32(sum_hi, 3);

    auto result = _mm256_packus_epi32(sum_lo, sum_hi);

    auto middle_center = simd256_load_si256<mem_mode>(pSrc + x);

    result = limit(middle_center, result, maxDeviation); // ??? bits_per_pixel template?

    simd256_store_si256<mem_mode>(pDst + x, result);
  }
}

template<Border borderMode, decltype(_mm256_max_epu16) op, Limit16 limit, MemoryMode mem_mode>
static MT_FORCEINLINE void process_line_xxpand_both_16_avx2(uint8_t *pDst, const uint8_t *pSrcp, const uint8_t *pSrc, const uint8_t *pSrcn, const __m256i &maxDeviation, int byte_width) {
  for (int x = 0; x < byte_width; x += 32) {
    __m256i up_center = simd256_load_si256<mem_mode>(pSrcp + x);
    __m256i middle_center = simd256_load_si256<mem_mode>(pSrc + x);
    __m256i down_center = simd256_load_si256<mem_mode>(pSrcn + x);
    __m256i middle_left = load16_one_to_left_si256<borderMode, mem_mode>(pSrc + x);
    __m256i middle_right = load16_one_to_right_si256<borderMode, mem_mode>(pSrc + x);
    __m256i acc = op(up_center, middle_left);
    acc = op(acc, middle_right);
    acc = op(acc, down_center);

    __m256i result = limit(middle_center, acc, maxDeviation);
    simd256_store_si256<mem_mode>(pDst + x, result);
  }
}

/* Vertical mt_xxpand */

template<decltype(_mm256_max_epu16) op, Limit16 limit, MemoryMode mem_mode>
static MT_FORCEINLINE void process_line_xxpand_vertical_16_avx2(uint8_t *pDst, const uint8_t *pSrcp, const uint8_t *pSrc, const uint8_t *pSrcn, const __m256i &maxDeviation, int byte_width) {
  for (int x = 0; x < byte_width; x += 32) {
    auto up_center = simd256_load_si256<mem_mode>(pSrcp + x);
    auto down_center = simd256_load_si256<mem_mode>(pSrcn + x);
    auto middle_center = simd256_load_si256<mem_mode>(pSrc + x);
    auto acc = op(up_center, down_center);
    auto result = limit(middle_center, acc, maxDeviation);
    simd256_store_si256<mem_mode>(pDst + x, result);
  }
}

template<decltype(_mm256_max_epu16) op, Limit16 limit, MemoryMode mem_mode>
static void xxpand_avx2_vertical_16(uint16_t *pDst16, ptrdiff_t nDstPitch, const uint16_t *pSrc16, ptrdiff_t nSrcPitch, int nMaxDeviation, const int *pCoordinates, int nCoordinates, int nWidth, int nHeight, int nOrigHeight) {
  // back to byte level
  const BYTE* pSrc = reinterpret_cast<const BYTE *>(pSrc16);
  BYTE* pDst = reinterpret_cast<BYTE *>(pDst16);
  nSrcPitch *= sizeof(uint16_t);
  nDstPitch *= sizeof(uint16_t);

  const uint8_t *pSrcp = pSrc - nSrcPitch;
  const uint8_t *pSrcn = pSrc + nSrcPitch;

  UNUSED(nCoordinates); UNUSED(pCoordinates); UNUSED(nOrigHeight);
  int byte_width = nWidth * sizeof(uint16_t);
  auto max_dev_v = _mm256_set1_epi16(uint16_t(nMaxDeviation));
  int mod32_width = byte_width / 32 * 32;
  bool not_mod32 = byte_width != mod32_width;

  process_line_xxpand_vertical_16_avx2<op, limit, mem_mode>(pDst, pSrc, pSrc, pSrcn, max_dev_v, mod32_width);

  if (not_mod32) {
    process_line_xxpand_vertical_16_avx2<op, limit, MemoryMode::SSE2_UNALIGNED>(pDst + byte_width - 32, pSrc + byte_width - 32, pSrc + byte_width - 32, pSrcn + byte_width - 32, max_dev_v, 32);
  }

  pDst += nDstPitch;
  pSrcp += nSrcPitch;
  pSrc += nSrcPitch;
  pSrcn += nSrcPitch;

  for (int y = 1; y < nHeight - 1; y++)
  {
    process_line_xxpand_vertical_16_avx2<op, limit, mem_mode>(pDst, pSrcp, pSrc, pSrcn, max_dev_v, mod32_width);

    if (not_mod32) {
      process_line_xxpand_vertical_16_avx2<op, limit, MemoryMode::SSE2_UNALIGNED>(pDst + byte_width - 32, pSrcp + byte_width - 32, pSrc + byte_width - 32, pSrcn + byte_width - 32, max_dev_v, 32);
    }

    pDst += nDstPitch;
    pSrcp += nSrcPitch;
    pSrc += nSrcPitch;
    pSrcn += nSrcPitch;
  }

  process_line_xxpand_vertical_16_avx2<op, limit, mem_mode>(pDst, pSrcp, pSrc, pSrc, max_dev_v, mod32_width);

  if (not_mod32) {
    process_line_xxpand_vertical_16_avx2<op, limit, MemoryMode::SSE2_UNALIGNED>(pDst + byte_width - 32, pSrcp + byte_width - 32, pSrc + byte_width - 32, pSrc + byte_width - 32, max_dev_v, 32);
  }
}


/* Horizontal mt_xxpand */

static MT_FORCEINLINE uint16_t expand_c_horizontal_core_16_avx2(uint16_t left, uint16_t center, uint16_t right, uint16_t max_dev) {
  uint16_t ma = left;

  if (center > ma) ma = center;
  if (right > ma) ma = right;

  if (ma - center > max_dev) ma = center + max_dev;
  return static_cast<uint16_t>(ma); // todo bits_per_pixel template????
}

static MT_FORCEINLINE uint16_t inpand_c_horizontal_core_16_avx2(uint16_t left, uint16_t center, uint16_t right, uint16_t max_dev) {
  uint16_t mi = left;

  if (center < mi) mi = center;
  if (right < mi) mi = right;

  if (center - mi > max_dev) mi = center - max_dev;
  return static_cast<uint16_t>(mi);
}

//this implemented in a somewhat retarded way to allow pDst and pSrc be the same pointer (used in 3x3 expand)
template<decltype(_mm256_max_epu16) op, Limit16 limit, MemoryMode mem_mode, decltype(expand_c_horizontal_core_16_avx2) c_core>
static void xxpand_avx2_horizontal_16(uint16_t *pDst16, ptrdiff_t nDstPitch, const uint16_t *pSrc16, ptrdiff_t nSrcPitch, int nMaxDeviation, const int *pCoordinates, int nCoordinates, int nWidth, int nHeight, int nOrigHeight) {
  // back to byte level
  const BYTE* pSrc = reinterpret_cast<const BYTE *>(pSrc16);
  BYTE* pDst = reinterpret_cast<BYTE *>(pDst16);
  nSrcPitch *= sizeof(uint16_t);
  nDstPitch *= sizeof(uint16_t);

  UNUSED(nCoordinates); UNUSED(pCoordinates); UNUSED(nOrigHeight);
  const int byte_width = nWidth * sizeof(uint16_t);
  int mod32_width = byte_width / 32 * 32;
  int sse_loop_limit = byte_width == mod32_width ? mod32_width - 32 : mod32_width;

  uint16_t max_dev = uint16_t(nMaxDeviation);
  __m256i max_dev_v = _mm256_set1_epi16(max_dev);
  __m256i left_mask = _mm256_set_epi32(0, 0, 0, 0, 0, 0, 0, 0xFFFF);
#pragma warning(disable: 4309)
  __m256i right_mask = _mm256_set_epi16(0xFFFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
#pragma warning(default: 4309)

  __m256i left;

  for (int y = 0; y < nHeight; ++y) {
    //left border
    __m256i center = simd256_load_si256<mem_mode>(pSrc);
    __m256i right = simd256_load_si256<MemoryMode::SSE2_UNALIGNED>(pSrc + sizeof(uint16_t)); // +2: word
    left = _mm256_or_si256(_mm256_and_si256(center, left_mask), _MM256_SLLI_SI256<2>(center)); // shift 2: word

    __m256i result = op(left, right);
    result = limit(center, result, max_dev_v);

    left = simd256_load_si256<MemoryMode::SSE2_UNALIGNED>(pSrc + 32- sizeof(uint16_t));
    simd256_store_si256<mem_mode>(pDst, result);

    //main processing loop
    for (int x = 32; x < sse_loop_limit; x += 32) {
      center = simd256_load_si256<mem_mode>(pSrc + x);
      right = simd256_load_si256<MemoryMode::SSE2_UNALIGNED>(pSrc + x + sizeof(uint16_t));

      result = op(left, right);
      result = limit(center, result, max_dev_v);

      left = simd256_load_si256<MemoryMode::SSE2_UNALIGNED>(pSrc + x + 32-sizeof(uint16_t));

      simd256_store_si256<mem_mode>(pDst + x, result);
    }

    //right border
    if (mod32_width == byte_width) {
      center = simd256_load_si256<mem_mode>(pSrc + mod32_width - 32);
      right = _mm256_or_si256(_mm256_and_si256(center, right_mask), _MM256_SRLI_SI256<2>(center));

      result = op(left, right);
      result = limit(center, result, max_dev_v);

      simd256_store_si256<mem_mode>(pDst + mod32_width - 32, result);
    }
    else { //some stuff left
      uint16_t l = _mm256_cvtsi256_si32(left) & 0xFFFF;

      int x;
      for (x = mod32_width / sizeof(uint16_t); x < nWidth - 1; ++x) {
        uint16_t temp = c_core(l, reinterpret_cast<const uint16_t *>(pSrc)[x], reinterpret_cast<const uint16_t *>(pSrc)[x + 1], max_dev);
        l = reinterpret_cast<const uint16_t *>(pSrc)[x];
        reinterpret_cast<uint16_t *>(pDst)[x] = temp;
      }
      reinterpret_cast<uint16_t *>(pDst)[x] = c_core(l, reinterpret_cast<const uint16_t *>(pSrc)[x], reinterpret_cast<const uint16_t *>(pSrc)[x], max_dev);
    }
    pSrc += nSrcPitch;
    pDst += nDstPitch;
  }
}

/* Square mt_xxpand */

// dst = limit(src, dst, deviation); the 3x3 result is limited by the source pixel, not by the vertical pass
template<Limit16 limit>
static void limit_by_source_avx2_16(uint16_t *pDst, ptrdiff_t nDstPitch, const uint16_t *pSrc, ptrdiff_t nSrcPitch, int nMaxDeviation, int nWidth, int nHeight) {
  auto max_dev_v = _mm256_set1_epi16(uint16_t(nMaxDeviation));
  for (int y = 0; y < nHeight; y++) {
    for (int x = 0; x < nWidth; x += 16) {
      int xx = x + 16 > nWidth ? nWidth - 16 : x; // the last vector overlaps, the limit is idempotent
      auto src = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(pSrc + xx));
      auto dst = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(pDst + xx));
      _mm256_storeu_si256(reinterpret_cast<__m256i *>(pDst + xx), limit(src, dst, max_dev_v));
    }
    pDst += nDstPitch;
    pSrc += nSrcPitch;
  }
}

template<decltype(_mm256_max_epu16) op, Limit16 limit, MemoryMode mem_mode, decltype(expand_c_horizontal_core_16_avx2) c_core>
static void xxpand_avx2_square_16(uint16_t *pDst, ptrdiff_t nDstPitch, const uint16_t *pSrc, ptrdiff_t nSrcPitch, int nMaxDeviation, const int *pCoordinates, int nCoordinates, int nWidth, int nHeight, int nOrigHeight) {
  // a limit in both passes would allow up to 2 * deviation: the passes unlimited, then the limit by the source
  const int no_limit = 65535;
  xxpand_avx2_vertical_16<op, limit, mem_mode>(pDst, nDstPitch, pSrc, nSrcPitch, no_limit, pCoordinates, nCoordinates, nWidth, nHeight, nOrigHeight);
  xxpand_avx2_horizontal_16<op, limit, mem_mode, c_core>(pDst, nDstPitch, pDst, nDstPitch, no_limit, pCoordinates, nCoordinates, nWidth, nHeight, nOrigHeight);
  if (nMaxDeviation < no_limit)
    limit_by_source_avx2_16<limit>(pDst, nDstPitch, pSrc, nSrcPitch, nMaxDeviation, nWidth, nHeight);
}


template<ProcessLine16Avx2 process_line_left, ProcessLine16Avx2 process_line, ProcessLine16Avx2 process_line_right>
static void generic_16_avx2(uint16_t *pDst16, ptrdiff_t nDstPitch, const uint16_t *pSrc16, ptrdiff_t nSrcPitch, int nMaxDeviation, const int *pCoordinates, int nCoordinates, int nWidth, int nHeight, int nOrigHeight) {
  // back to byte level
  const BYTE* pSrc = reinterpret_cast<const BYTE *>(pSrc16);
  BYTE* pDst = reinterpret_cast<BYTE *>(pDst16);
  nSrcPitch *= sizeof(uint16_t);
  nDstPitch *= sizeof(uint16_t);

  const uint8_t *pSrcp = pSrc - nSrcPitch;
  const uint8_t *pSrcn = pSrc + nSrcPitch;

  UNUSED(nCoordinates); UNUSED(pCoordinates); UNUSED(nOrigHeight);
  auto max_dev_v = _mm256_set1_epi16(uint16_t(nMaxDeviation));
  int byte_width = nWidth * sizeof(uint16_t);
  int mod32_width = (byte_width - sizeof(uint16_t) - 32) / 32 * 32 + 32;
  /* top-left */
  process_line_left(pDst, pSrc, pSrc, pSrcn, max_dev_v, 32);
  /* top */
  process_line(pDst + 32, pSrc + 32, pSrc + 32, pSrcn + 32, max_dev_v, mod32_width - 32);

  /* top-right */
  process_line_right(pDst + byte_width - 32, pSrc + byte_width - 32, pSrc + byte_width - 32, pSrcn + byte_width - 32, max_dev_v, 32);

  pDst += nDstPitch;
  pSrcp += nSrcPitch;
  pSrc += nSrcPitch;
  pSrcn += nSrcPitch;

  for (int y = 1; y < nHeight - 1; y++)
  {
    /* left */
    process_line_left(pDst, pSrcp, pSrc, pSrcn, max_dev_v, 32);
    /* center */
    process_line(pDst + 32, pSrcp + 32, pSrc + 32, pSrcn + 32, max_dev_v, mod32_width - 32);
    /* right */
    process_line_right(pDst + byte_width - 32, pSrcp + byte_width - 32, pSrc + byte_width - 32, pSrcn + byte_width - 32, max_dev_v, 32);

    pDst += nDstPitch;
    pSrcp += nSrcPitch;
    pSrc += nSrcPitch;
    pSrcn += nSrcPitch;
  }

  /* bottom-left */
  process_line_left(pDst, pSrcp, pSrc, pSrc, max_dev_v, 32);
  /* bottom */
  process_line(pDst + 32, pSrcp + 32, pSrc + 32, pSrc + 32, max_dev_v, mod32_width - 32);
  /* bottom-right */
  process_line_right(pDst + byte_width - 32, pSrcp + byte_width - 32, pSrc + byte_width - 32, pSrc + byte_width - 32, max_dev_v, 32);
}
//-----------------------------------------------------------------



template<class T>
void generic_custom_16_avx2_c(uint16_t *pDst, ptrdiff_t nDstPitch, const uint16_t *pSrc, ptrdiff_t nSrcPitch, int nMaxDeviation, const int *pCoordinates, int nCoordinates, int nWidth, int nHeight, int nOrigHeight)
{
    UNUSED(nOrigHeight);

    for ( int j = 0; j < nHeight; j++ )
    {
        for ( int i = 0; i < nWidth; i++ )
        {
            T new_value(pSrc[i], nMaxDeviation);
            for ( int k = 0; k < nCoordinates; k+=2 )
            {
                if ( pCoordinates[k] + i >= 0 && pCoordinates[k] + i < nWidth &&
                    pCoordinates[k+1] + j >= 0 && pCoordinates[k+1] + j < nHeight ) {
                        new_value.add(pSrc[i + pCoordinates[k] + pCoordinates[k+1] * nSrcPitch]);
                }
            }
            pDst[i] = new_value.finalize();
        }
        pSrc += nSrcPitch;
        pDst += nDstPitch;
    }
}


} // namespace

} } } } // namespace Morphologic, Filters, MaskTools, Filtering

#endif
