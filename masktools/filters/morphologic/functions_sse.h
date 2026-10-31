#ifndef __Mt_functions_sse_H__
#define __Mt_functions_sse_H__

#include "functions.h"
#include "../../common/simd.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Morphologic {
// Anonymous namespace: this header is included from files built with different per file compiler flags.
namespace {

typedef __m128i (Limit)(__m128i source, __m128i sum, __m128i deviation);
typedef void (ProcessLineSse2)(uint8_t *pDst, const uint8_t *pSrcp, const uint8_t *pSrc, const uint8_t *pSrcn, const __m128i &maxDeviation, int width);

static MT_FORCEINLINE __m128i limit_up_sse2(__m128i source, __m128i sum, __m128i deviation) {
    auto limit = _mm_adds_epu8(source, deviation);
    return _mm_min_epu8(limit, _mm_max_epu8(source, sum));
}

static MT_FORCEINLINE __m128i limit_down_sse2(__m128i source, __m128i sum, __m128i deviation) {
    auto limit = _mm_subs_epu8(source, deviation);
    return _mm_max_epu8(limit, _mm_min_epu8(source, sum));
}

template<Border borderMode, Limit limit, MemoryMode mem_mode>
static MT_FORCEINLINE void process_line_xxflate(uint8_t *pDst, const uint8_t *pSrcp, const uint8_t *pSrc, const uint8_t *pSrcn, const __m128i &maxDeviation, int width) {
    auto zero = _mm_setzero_si128();
    for ( int x = 0; x < width; x+=16 ) {
        auto up_left = load_one_to_left<borderMode, mem_mode>(pSrcp+x);
        auto up_center = simd_load_si128<mem_mode>(pSrcp + x);
        auto up_right = load_one_to_right<borderMode, mem_mode>(pSrcp+x);
        
        auto middle_left = load_one_to_left<borderMode, mem_mode>(pSrc+x);
        auto middle_right = load_one_to_right<borderMode, mem_mode>(pSrc+x);
        
        auto down_left = load_one_to_left<borderMode, mem_mode>(pSrcn+x);
        auto down_center = simd_load_si128<mem_mode>(pSrcn + x);
        auto down_right = load_one_to_right<borderMode, mem_mode>(pSrcn+x);

        auto up_left_lo = _mm_unpacklo_epi8(up_left, zero);
        auto up_left_hi = _mm_unpackhi_epi8(up_left, zero);

        auto up_center_lo = _mm_unpacklo_epi8(up_center, zero);
        auto up_center_hi = _mm_unpackhi_epi8(up_center, zero);

        auto up_right_lo = _mm_unpacklo_epi8(up_right, zero);
        auto up_right_hi = _mm_unpackhi_epi8(up_right, zero);

        auto middle_left_lo = _mm_unpacklo_epi8(middle_left, zero);
        auto middle_left_hi = _mm_unpackhi_epi8(middle_left, zero);

        auto middle_right_lo = _mm_unpacklo_epi8(middle_right, zero);
        auto middle_right_hi = _mm_unpackhi_epi8(middle_right, zero);

        auto down_left_lo = _mm_unpacklo_epi8(down_left, zero);
        auto down_left_hi = _mm_unpackhi_epi8(down_left, zero);

        auto down_center_lo = _mm_unpacklo_epi8(down_center, zero);
        auto down_center_hi = _mm_unpackhi_epi8(down_center, zero);

        auto down_right_lo = _mm_unpacklo_epi8(down_right, zero);
        auto down_right_hi = _mm_unpackhi_epi8(down_right, zero);

        auto sum_lo = _mm_add_epi16(up_left_lo, up_center_lo);
        sum_lo = _mm_add_epi16(sum_lo, up_right_lo);
        sum_lo = _mm_add_epi16(sum_lo, middle_left_lo);
        sum_lo = _mm_add_epi16(sum_lo, middle_right_lo);
        sum_lo = _mm_add_epi16(sum_lo, down_left_lo);
        sum_lo = _mm_add_epi16(sum_lo, down_center_lo);
        sum_lo = _mm_add_epi16(sum_lo, down_right_lo);

        auto sum_hi = _mm_add_epi16(up_left_hi, up_center_hi);
        sum_hi = _mm_add_epi16(sum_hi, up_right_hi);
        sum_hi = _mm_add_epi16(sum_hi, middle_left_hi);
        sum_hi = _mm_add_epi16(sum_hi, middle_right_hi);
        sum_hi = _mm_add_epi16(sum_hi, down_left_hi);
        sum_hi = _mm_add_epi16(sum_hi, down_center_hi);
        sum_hi = _mm_add_epi16(sum_hi, down_right_hi);

        sum_lo = _mm_srai_epi16(sum_lo, 3);
        sum_hi = _mm_srai_epi16(sum_hi, 3);

        auto result = _mm_packus_epi16(sum_lo, sum_hi);

        auto middle_center = simd_load_si128<mem_mode>(pSrc + x);
        
        result = limit(middle_center, result, maxDeviation);

        simd_store_si128<mem_mode>(pDst+x, result);
    }
}

template<Border borderMode, decltype(_mm_max_epu8) op, Limit limit, MemoryMode mem_mode>
static MT_FORCEINLINE void process_line_xxpand_both(uint8_t *pDst, const uint8_t *pSrcp, const uint8_t *pSrc, const uint8_t *pSrcn, const __m128i &maxDeviation, int width) {
    for (int x = 0; x < width; x += 16) {
        __m128i up_center = simd_load_si128<mem_mode>(pSrcp+x);
        __m128i middle_center = simd_load_si128<mem_mode>(pSrc+x);
        __m128i down_center = simd_load_si128<mem_mode>(pSrcn+x);
        __m128i middle_left = load_one_to_left<borderMode, mem_mode>(pSrc+x);
        __m128i middle_right = load_one_to_right<borderMode, mem_mode>(pSrc+x);

        __m128i acc = op(up_center, middle_left);
        acc = op(acc, middle_right);
        acc = op(acc, down_center);

        __m128i result = limit(middle_center, acc, maxDeviation);
        simd_store_si128<mem_mode>(pDst+x, result);
    }
}

/* Vertical mt_xxpand */

template<decltype(_mm_max_epu8) op, Limit limit, MemoryMode mem_mode>
static MT_FORCEINLINE void process_line_xxpand_vertical(uint8_t *pDst, const uint8_t *pSrcp, const uint8_t *pSrc, const uint8_t *pSrcn, const __m128i &maxDeviation, int width) {
    for (int x = 0; x < width; x += 16) {
        auto up_center = simd_load_si128<mem_mode>(pSrcp+x);
        auto down_center = simd_load_si128<mem_mode>(pSrcn+x);
        auto middle_center = simd_load_si128<mem_mode>(pSrc+x);
        auto acc = op(up_center, down_center);
        auto result = limit(middle_center, acc, maxDeviation);
        simd_store_si128<mem_mode>(pDst+x, result);
    }
}

template<decltype(_mm_max_epu8) op, Limit limit, MemoryMode mem_mode>
static void xxpand_sse2_vertical(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int nMaxDeviation, const int *pCoordinates, int nCoordinates, int nWidth, int nHeight) {
    const uint8_t *pSrcp = pSrc - nSrcPitch;
    const uint8_t *pSrcn = pSrc + nSrcPitch;

    UNUSED(nCoordinates); UNUSED(pCoordinates);
    auto max_dev_v = _mm_set1_epi8(uint8_t(nMaxDeviation));
    int mod16_width = nWidth / 16 * 16;
    bool not_mod16 = nWidth != mod16_width;
   
    process_line_xxpand_vertical<op, limit, mem_mode>(pDst, pSrc, pSrc, pSrcn, max_dev_v, mod16_width);

    if (not_mod16) {
        process_line_xxpand_vertical<op, limit, MemoryMode::SSE2_UNALIGNED>(pDst + nWidth - 16, pSrc+nWidth-16, pSrc+nWidth-16, pSrcn+nWidth-16, max_dev_v, 16);
    }

    pDst += nDstPitch;
    pSrcp += nSrcPitch;
    pSrc += nSrcPitch;
    pSrcn += nSrcPitch;

    for (int y = 1; y < nHeight-1; y++)
    {
        process_line_xxpand_vertical<op, limit, mem_mode>(pDst, pSrcp, pSrc, pSrcn, max_dev_v, mod16_width);

        if (not_mod16) {
            process_line_xxpand_vertical<op, limit, MemoryMode::SSE2_UNALIGNED>(pDst + nWidth - 16, pSrcp+nWidth - 16, pSrc+nWidth - 16, pSrcn+nWidth - 16, max_dev_v, 16);
        }

        pDst += nDstPitch;
        pSrcp += nSrcPitch;
        pSrc += nSrcPitch;
        pSrcn += nSrcPitch;
    }
    
    process_line_xxpand_vertical<op, limit, mem_mode>(pDst, pSrcp, pSrc, pSrc, max_dev_v, mod16_width);

    if (not_mod16) {
        process_line_xxpand_vertical<op, limit, MemoryMode::SSE2_UNALIGNED>(pDst + nWidth - 16, pSrcp+nWidth - 16, pSrc+nWidth - 16, pSrc+nWidth - 16, max_dev_v, 16);
    }
}

//this implemented in a somewhat retarded way to allow pDst and pSrc be the same pointer (used in 3x3 expand)
template<decltype(_mm_max_epu8) op, Limit limit, MemoryMode mem_mode, decltype(expand_c_horizontal_core) c_core>
static void xxpand_sse2_horizontal(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int nMaxDeviation, const int *pCoordinates, int nCoordinates, int nWidth, int nHeight) {
    UNUSED(nCoordinates); UNUSED(pCoordinates);

    int mod16_width = (nWidth / 16) * 16;
    int sse_loop_limit = nWidth == mod16_width ? mod16_width - 16 : mod16_width;
    
    uint8_t max_dev = uint8_t(nMaxDeviation);
    __m128i max_dev_v = _mm_set1_epi8(max_dev);
    __m128i left_mask = _mm_set_epi32(0, 0, 0, 0xFF);
#pragma warning(disable: 4309)
    __m128i right_mask = _mm_set_epi8(0xFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
#pragma warning(default: 4309)

    __m128i left;

    for (int y = 0; y < nHeight; ++y) {
        //left border
        __m128i center = simd_load_si128<mem_mode>(pSrc);
        __m128i right = simd_load_si128<MemoryMode::SSE2_UNALIGNED>(pSrc+1);
        left = _mm_or_si128(_mm_and_si128(center, left_mask), _mm_slli_si128(center, 1));

        __m128i result = op(left, right);
        result = limit(center, result, max_dev_v);

        left = simd_load_si128<MemoryMode::SSE2_UNALIGNED>(pSrc+15);
        simd_store_si128<mem_mode>(pDst, result);

        //main processing loop
        for (int x = 16; x < sse_loop_limit; x += 16) {
            center = simd_load_si128<mem_mode>(pSrc+x);
            right = simd_load_si128<MemoryMode::SSE2_UNALIGNED>(pSrc+x+1);

            result = op(left, right);
            result = limit(center, result, max_dev_v);

            left = simd_load_si128<MemoryMode::SSE2_UNALIGNED>(pSrc+x+15);

            simd_store_si128<mem_mode>(pDst+x, result);
        }

        //right border
        if (mod16_width == nWidth) {
            center = simd_load_si128<mem_mode>(pSrc+mod16_width-16);
            right = _mm_or_si128(_mm_and_si128(center, right_mask), _mm_srli_si128(center, 1));

            result = op(left, right);
            result = limit(center, result, max_dev_v);

            simd_store_si128<mem_mode>(pDst+mod16_width-16, result);
        } else { //some stuff left
             uint8_t l = _mm_cvtsi128_si32(left) & 0xFF;

             int x;
             for (x = mod16_width; x < nWidth-1; ++x) {
                 uint8_t temp = c_core(l, pSrc[x], pSrc[x+1], max_dev);
                 l = pSrc[x];
                 pDst[x] = temp;
             }
             pDst[x] = c_core(l, pSrc[x], pSrc[x], max_dev);
        }
        pSrc += nSrcPitch;
        pDst += nDstPitch;
    }
}

/* Square mt_xxpand */

// dst = limit(src, dst, deviation); the 3x3 result is limited by the source pixel, not by the vertical pass
template<Limit limit>
static void limit_by_source_sse2(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int nMaxDeviation, int nWidth, int nHeight) {
    auto max_dev_v = _mm_set1_epi8(uint8_t(nMaxDeviation));
    for (int y = 0; y < nHeight; y++) {
        for (int x = 0; x < nWidth; x += 16) {
            int xx = x + 16 > nWidth ? nWidth - 16 : x; // the last vector overlaps, the limit is idempotent
            auto src = simd_load_si128<MemoryMode::SSE2_UNALIGNED>(pSrc + xx);
            auto dst = simd_load_si128<MemoryMode::SSE2_UNALIGNED>(pDst + xx);
            simd_store_si128<MemoryMode::SSE2_UNALIGNED>(pDst + xx, limit(src, dst, max_dev_v));
        }
        pDst += nDstPitch;
        pSrc += nSrcPitch;
    }
}

template<decltype(_mm_max_epu8) op, Limit limit, MemoryMode mem_mode, decltype(expand_c_horizontal_core) c_core>
static void xxpand_sse2_square(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int nMaxDeviation, const int *pCoordinates, int nCoordinates, int nWidth, int nHeight) {
    // a limit in both passes would allow up to 2 * deviation: the passes unlimited, then the limit by the source
    const int no_limit = 255;
    xxpand_sse2_vertical<op, limit, mem_mode>(pDst, nDstPitch, pSrc, nSrcPitch, no_limit, pCoordinates, nCoordinates, nWidth, nHeight);
    xxpand_sse2_horizontal<op, limit, mem_mode, c_core>(pDst, nDstPitch, pDst, nDstPitch, no_limit, pCoordinates, nCoordinates, nWidth, nHeight);
    if (nMaxDeviation < no_limit)
        limit_by_source_sse2<limit>(pDst, nDstPitch, pSrc, nSrcPitch, nMaxDeviation, nWidth, nHeight);
}

template<ProcessLineSse2 process_line_left, ProcessLineSse2 process_line, ProcessLineSse2 process_line_right>
static void generic_sse2(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int nMaxDeviation, const int *pCoordinates, int nCoordinates, int nWidth, int nHeight) {
    const uint8_t *pSrcp = pSrc - nSrcPitch;
    const uint8_t *pSrcn = pSrc + nSrcPitch;

    UNUSED(nCoordinates); UNUSED(pCoordinates);
    auto max_dev_v = _mm_set1_epi8(uint8_t(nMaxDeviation));
    int mod16_width = (nWidth - 1 - 16) / 16 * 16 + 16;
    /* top-left */
    process_line_left(pDst, pSrc, pSrc, pSrcn, max_dev_v, 16);
    /* top */
    process_line(pDst + 16, pSrc+16, pSrc+16, pSrcn+16, max_dev_v, mod16_width - 16);

    /* top-right */
    process_line_right(pDst + nWidth - 16, pSrc + nWidth - 16, pSrc + nWidth - 16, pSrcn + nWidth - 16, max_dev_v, 16);

    pDst  += nDstPitch;
    pSrcp += nSrcPitch;
    pSrc  += nSrcPitch;
    pSrcn += nSrcPitch;

    for ( int y = 1; y < nHeight-1; y++ )
    {
        /* left */
        process_line_left(pDst, pSrcp, pSrc, pSrcn, max_dev_v, 16);
        /* center */
        process_line(pDst + 16, pSrcp+16, pSrc+16, pSrcn+16, max_dev_v, mod16_width - 16);
        /* right */
        process_line_right(pDst + nWidth - 16, pSrcp + nWidth - 16, pSrc + nWidth - 16, pSrcn + nWidth - 16, max_dev_v, 16);

        pDst  += nDstPitch;
        pSrcp += nSrcPitch;
        pSrc  += nSrcPitch;
        pSrcn += nSrcPitch;
    }

    /* bottom-left */
    process_line_left(pDst, pSrcp, pSrc, pSrc, max_dev_v, 16);
    /* bottom */
    process_line(pDst + 16, pSrcp+16, pSrc+16, pSrc+16, max_dev_v, mod16_width - 16);
    /* bottom-right */
    process_line_right(pDst + nWidth - 16, pSrcp + nWidth - 16, pSrc + nWidth - 16, pSrc + nWidth - 16, max_dev_v, 16);
}

} // namespace

} } } } // namespace Morphologic, Filters, MaskTools, Filtering

#endif // __Mt_functions_sse_H__
