#ifndef __Mt_functions_avx2_H__
#define __Mt_functions_avx2_H__

#include "functions.h"
#include "../../common/simd_avx2.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Morphologic {
// Anonymous namespace: this header is included from files built with different per file compiler flags.
// 8 bit AVX2: functions_sse.h with 32 byte vectors (whole 256 bit shifts in the horizontal pass)
namespace {

typedef __m256i (LimitAvx2)(__m256i source, __m256i sum, __m256i deviation);
typedef void (ProcessLineAvx2)(uint8_t *pDst, const uint8_t *pSrcp, const uint8_t *pSrc, const uint8_t *pSrcn, const __m256i &maxDeviation, int width);

static MT_FORCEINLINE __m256i limit_up_avx2(__m256i source, __m256i sum, __m256i deviation) {
    auto limit = _mm256_adds_epu8(source, deviation);
    return _mm256_min_epu8(limit, _mm256_max_epu8(source, sum));
}

static MT_FORCEINLINE __m256i limit_down_avx2(__m256i source, __m256i sum, __m256i deviation) {
    auto limit = _mm256_subs_epu8(source, deviation);
    return _mm256_max_epu8(limit, _mm256_min_epu8(source, sum));
}

template<Border borderMode, LimitAvx2 limit>
static MT_FORCEINLINE void process_line_xxflate_avx2(uint8_t *pDst, const uint8_t *pSrcp, const uint8_t *pSrc, const uint8_t *pSrcn, const __m256i &maxDeviation, int width) {
    auto zero = _mm256_setzero_si256();
    for ( int x = 0; x < width; x+=32 ) {
        auto up_left = load_one_to_left_si256<borderMode>(pSrcp+x);
        auto up_center = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pSrcp + x));
        auto up_right = load_one_to_right_si256<borderMode>(pSrcp+x);
        
        auto middle_left = load_one_to_left_si256<borderMode>(pSrc+x);
        auto middle_right = load_one_to_right_si256<borderMode>(pSrc+x);
        
        auto down_left = load_one_to_left_si256<borderMode>(pSrcn+x);
        auto down_center = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pSrcn + x));
        auto down_right = load_one_to_right_si256<borderMode>(pSrcn+x);

        auto up_left_lo = _mm256_unpacklo_epi8(up_left, zero);
        auto up_left_hi = _mm256_unpackhi_epi8(up_left, zero);

        auto up_center_lo = _mm256_unpacklo_epi8(up_center, zero);
        auto up_center_hi = _mm256_unpackhi_epi8(up_center, zero);

        auto up_right_lo = _mm256_unpacklo_epi8(up_right, zero);
        auto up_right_hi = _mm256_unpackhi_epi8(up_right, zero);

        auto middle_left_lo = _mm256_unpacklo_epi8(middle_left, zero);
        auto middle_left_hi = _mm256_unpackhi_epi8(middle_left, zero);

        auto middle_right_lo = _mm256_unpacklo_epi8(middle_right, zero);
        auto middle_right_hi = _mm256_unpackhi_epi8(middle_right, zero);

        auto down_left_lo = _mm256_unpacklo_epi8(down_left, zero);
        auto down_left_hi = _mm256_unpackhi_epi8(down_left, zero);

        auto down_center_lo = _mm256_unpacklo_epi8(down_center, zero);
        auto down_center_hi = _mm256_unpackhi_epi8(down_center, zero);

        auto down_right_lo = _mm256_unpacklo_epi8(down_right, zero);
        auto down_right_hi = _mm256_unpackhi_epi8(down_right, zero);

        auto sum_lo = _mm256_add_epi16(up_left_lo, up_center_lo);
        sum_lo = _mm256_add_epi16(sum_lo, up_right_lo);
        sum_lo = _mm256_add_epi16(sum_lo, middle_left_lo);
        sum_lo = _mm256_add_epi16(sum_lo, middle_right_lo);
        sum_lo = _mm256_add_epi16(sum_lo, down_left_lo);
        sum_lo = _mm256_add_epi16(sum_lo, down_center_lo);
        sum_lo = _mm256_add_epi16(sum_lo, down_right_lo);

        auto sum_hi = _mm256_add_epi16(up_left_hi, up_center_hi);
        sum_hi = _mm256_add_epi16(sum_hi, up_right_hi);
        sum_hi = _mm256_add_epi16(sum_hi, middle_left_hi);
        sum_hi = _mm256_add_epi16(sum_hi, middle_right_hi);
        sum_hi = _mm256_add_epi16(sum_hi, down_left_hi);
        sum_hi = _mm256_add_epi16(sum_hi, down_center_hi);
        sum_hi = _mm256_add_epi16(sum_hi, down_right_hi);

        sum_lo = _mm256_srai_epi16(sum_lo, 3);
        sum_hi = _mm256_srai_epi16(sum_hi, 3);

        auto result = _mm256_packus_epi16(sum_lo, sum_hi);

        auto middle_center = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pSrc + x));
        
        result = limit(middle_center, result, maxDeviation);

        _mm256_storeu_si256(reinterpret_cast<__m256i*>(pDst+x), result);
    }
}

template<Border borderMode, decltype(_mm256_max_epu8) op, LimitAvx2 limit>
static MT_FORCEINLINE void process_line_xxpand_both_avx2(uint8_t *pDst, const uint8_t *pSrcp, const uint8_t *pSrc, const uint8_t *pSrcn, const __m256i &maxDeviation, int width) {
    for (int x = 0; x < width; x += 32) {
        __m256i up_center = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pSrcp+x));
        __m256i middle_center = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pSrc+x));
        __m256i down_center = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pSrcn+x));
        __m256i middle_left = load_one_to_left_si256<borderMode>(pSrc+x);
        __m256i middle_right = load_one_to_right_si256<borderMode>(pSrc+x);

        __m256i acc = op(up_center, middle_left);
        acc = op(acc, middle_right);
        acc = op(acc, down_center);

        __m256i result = limit(middle_center, acc, maxDeviation);
        _mm256_storeu_si256(reinterpret_cast<__m256i*>(pDst+x), result);
    }
}

/* Vertical mt_xxpand */

template<decltype(_mm256_max_epu8) op, LimitAvx2 limit>
static MT_FORCEINLINE void process_line_xxpand_vertical_avx2(uint8_t *pDst, const uint8_t *pSrcp, const uint8_t *pSrc, const uint8_t *pSrcn, const __m256i &maxDeviation, int width) {
    for (int x = 0; x < width; x += 32) {
        auto up_center = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pSrcp+x));
        auto down_center = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pSrcn+x));
        auto middle_center = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pSrc+x));
        auto acc = op(up_center, down_center);
        auto result = limit(middle_center, acc, maxDeviation);
        _mm256_storeu_si256(reinterpret_cast<__m256i*>(pDst+x), result);
    }
}

// nHeight >= 2: separate top and bottom row passes (smaller planes go to C, filter.h plane_flags)
template<decltype(_mm256_max_epu8) op, LimitAvx2 limit>
static void xxpand_avx2_vertical(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int nMaxDeviation, const int *pCoordinates, int nCoordinates, int nWidth, int nHeight) {
    const uint8_t *pSrcp = pSrc - nSrcPitch;
    const uint8_t *pSrcn = pSrc + nSrcPitch;

    UNUSED(nCoordinates); UNUSED(pCoordinates);
    auto max_dev_v = _mm256_set1_epi8(uint8_t(nMaxDeviation));
    int mod32_width = nWidth / 32 * 32;
    bool not_mod32 = nWidth != mod32_width;
   
    process_line_xxpand_vertical_avx2<op, limit>(pDst, pSrc, pSrc, pSrcn, max_dev_v, mod32_width);

    if (not_mod32) {
        process_line_xxpand_vertical_avx2<op, limit>(pDst + nWidth - 32, pSrc+nWidth-32, pSrc+nWidth-32, pSrcn+nWidth-32, max_dev_v, 32);
    }

    pDst += nDstPitch;
    pSrcp += nSrcPitch;
    pSrc += nSrcPitch;
    pSrcn += nSrcPitch;

    for (int y = 1; y < nHeight-1; y++)
    {
        process_line_xxpand_vertical_avx2<op, limit>(pDst, pSrcp, pSrc, pSrcn, max_dev_v, mod32_width);

        if (not_mod32) {
            process_line_xxpand_vertical_avx2<op, limit>(pDst + nWidth - 32, pSrcp+nWidth - 32, pSrc+nWidth - 32, pSrcn+nWidth - 32, max_dev_v, 32);
        }

        pDst += nDstPitch;
        pSrcp += nSrcPitch;
        pSrc += nSrcPitch;
        pSrcn += nSrcPitch;
    }
    
    process_line_xxpand_vertical_avx2<op, limit>(pDst, pSrcp, pSrc, pSrc, max_dev_v, mod32_width);

    if (not_mod32) {
        process_line_xxpand_vertical_avx2<op, limit>(pDst + nWidth - 32, pSrcp+nWidth - 32, pSrc+nWidth - 32, pSrc+nWidth - 32, max_dev_v, 32);
    }
}

// nWidth >= 33 (one vector + 1; processor constraint): the first block reads pSrc+1 .. one vector; with one vector
// the right border would redo x=0 from the stored result (pDst may be pSrc)
//this implemented in a somewhat retarded way to allow pDst and pSrc be the same pointer (used in 3x3 expand)
template<decltype(_mm256_max_epu8) op, LimitAvx2 limit, decltype(expand_c_horizontal_core) c_core>
static void xxpand_avx2_horizontal(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int nMaxDeviation, const int *pCoordinates, int nCoordinates, int nWidth, int nHeight) {
    UNUSED(nCoordinates); UNUSED(pCoordinates);

    int mod32_width = (nWidth / 32) * 32;
    int sse_loop_limit = nWidth == mod32_width ? mod32_width - 32 : mod32_width;
    
    uint8_t max_dev = uint8_t(nMaxDeviation);
    __m256i max_dev_v = _mm256_set1_epi8(max_dev);
    __m256i left_mask = _mm256_setr_epi32(0xFF, 0, 0, 0, 0, 0, 0, 0);
    __m256i right_mask = _mm256_setr_epi32(0, 0, 0, 0, 0, 0, 0, int(0xFF000000));

    for (int y = 0; y < nHeight; ++y) {
        //left border
        __m256i center = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pSrc));
        __m256i right = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pSrc+1));
        __m256i left = _mm256_or_si256(_mm256_and_si256(center, left_mask), _MM256_SLLI_SI256<1>(center));

        __m256i result = op(left, right);
        result = limit(center, result, max_dev_v);

        // the left neighbour of the next block, read before the store (pDst may be pSrc); a vector load here
        // read past the row end before the C tail
        uint8_t prev = pSrc[31];
        _mm256_storeu_si256(reinterpret_cast<__m256i*>(pDst), result);

        //main processing loop
        for (int x = 32; x < sse_loop_limit; x += 32) {
            center = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pSrc+x));
            right = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pSrc+x+1));
            left = _mm256_or_si256(_MM256_SLLI_SI256<1>(center), _mm256_setr_epi32(prev, 0, 0, 0, 0, 0, 0, 0));

            result = op(left, right);
            result = limit(center, result, max_dev_v);

            prev = pSrc[x+31];
            _mm256_storeu_si256(reinterpret_cast<__m256i*>(pDst+x), result);
        }

        //right border
        if (mod32_width == nWidth) {
            center = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pSrc+mod32_width-32));
            right = _mm256_or_si256(_mm256_and_si256(center, right_mask), _MM256_SRLI_SI256<1>(center));
            left = _mm256_or_si256(_MM256_SLLI_SI256<1>(center), _mm256_setr_epi32(prev, 0, 0, 0, 0, 0, 0, 0));

            result = op(left, right);
            result = limit(center, result, max_dev_v);

            _mm256_storeu_si256(reinterpret_cast<__m256i*>(pDst+mod32_width-32), result);
        } else { //some stuff left
             uint8_t l = prev;

             int x;
             for (x = mod32_width; x < nWidth-1; ++x) {
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
template<LimitAvx2 limit>
static void limit_by_source_avx2(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int nMaxDeviation, int nWidth, int nHeight) {
    auto max_dev_v = _mm256_set1_epi8(uint8_t(nMaxDeviation));
    for (int y = 0; y < nHeight; y++) {
        for (int x = 0; x < nWidth; x += 32) {
            int xx = x + 32 > nWidth ? nWidth - 32 : x; // the last vector overlaps, the limit is idempotent
            auto src = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pSrc + xx));
            auto dst = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pDst + xx));
            _mm256_storeu_si256(reinterpret_cast<__m256i*>(pDst + xx), limit(src, dst, max_dev_v));
        }
        pDst += nDstPitch;
        pSrc += nSrcPitch;
    }
}

// nWidth >= 33: the horizontal pass
template<decltype(_mm256_max_epu8) op, LimitAvx2 limit, decltype(expand_c_horizontal_core) c_core>
static void xxpand_avx2_square(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int nMaxDeviation, const int *pCoordinates, int nCoordinates, int nWidth, int nHeight) {
    // a limit in both passes would allow up to 2 * deviation: the passes unlimited, then the limit by the source
    const int no_limit = 255;
    xxpand_avx2_vertical<op, limit>(pDst, nDstPitch, pSrc, nSrcPitch, no_limit, pCoordinates, nCoordinates, nWidth, nHeight);
    xxpand_avx2_horizontal<op, limit, c_core>(pDst, nDstPitch, pDst, nDstPitch, no_limit, pCoordinates, nCoordinates, nWidth, nHeight);
    if (nMaxDeviation < no_limit)
        limit_by_source_avx2<limit>(pDst, nDstPitch, pSrc, nSrcPitch, nMaxDeviation, nWidth, nHeight);
}

// nWidth >= 33 (one vector + 1; processor constraint): with one vector the right pass recomputes x=0, reading pSrc-1
// nHeight >= 2: separate top and bottom row passes (smaller planes go to C, filter.h plane_flags)
template<ProcessLineAvx2 process_line_left, ProcessLineAvx2 process_line, ProcessLineAvx2 process_line_right>
static void generic_avx2(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int nMaxDeviation, const int *pCoordinates, int nCoordinates, int nWidth, int nHeight) {
    const uint8_t *pSrcp = pSrc - nSrcPitch;
    const uint8_t *pSrcn = pSrc + nSrcPitch;

    UNUSED(nCoordinates); UNUSED(pCoordinates);
    auto max_dev_v = _mm256_set1_epi8(uint8_t(nMaxDeviation));
    int mod32_width = (nWidth - 1 - 32) / 32 * 32 + 32;
    /* top-left */
    process_line_left(pDst, pSrc, pSrc, pSrcn, max_dev_v, 32);
    /* top */
    process_line(pDst + 32, pSrc+32, pSrc+32, pSrcn+32, max_dev_v, mod32_width - 32);

    /* top-right */
    process_line_right(pDst + nWidth - 32, pSrc + nWidth - 32, pSrc + nWidth - 32, pSrcn + nWidth - 32, max_dev_v, 32);

    pDst  += nDstPitch;
    pSrcp += nSrcPitch;
    pSrc  += nSrcPitch;
    pSrcn += nSrcPitch;

    for ( int y = 1; y < nHeight-1; y++ )
    {
        /* left */
        process_line_left(pDst, pSrcp, pSrc, pSrcn, max_dev_v, 32);
        /* center */
        process_line(pDst + 32, pSrcp+32, pSrc+32, pSrcn+32, max_dev_v, mod32_width - 32);
        /* right */
        process_line_right(pDst + nWidth - 32, pSrcp + nWidth - 32, pSrc + nWidth - 32, pSrcn + nWidth - 32, max_dev_v, 32);

        pDst  += nDstPitch;
        pSrcp += nSrcPitch;
        pSrc  += nSrcPitch;
        pSrcn += nSrcPitch;
    }

    /* bottom-left */
    process_line_left(pDst, pSrcp, pSrc, pSrc, max_dev_v, 32);
    /* bottom */
    process_line(pDst + 32, pSrcp+32, pSrc+32, pSrc+32, max_dev_v, mod32_width - 32);
    /* bottom-right */
    process_line_right(pDst + nWidth - 32, pSrcp + nWidth - 32, pSrc + nWidth - 32, pSrc + nWidth - 32, max_dev_v, 32);
}

} // namespace

} } } } // namespace Morphologic, Filters, MaskTools, Filtering

#endif // __Mt_functions_avx2_H__
