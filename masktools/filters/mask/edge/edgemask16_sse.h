#ifndef __Mt_edgemask16_SSE_H__
#define __Mt_edgemask16_SSE_H__

// edgemask16: x86 kernels, instantiated in edgemask16_sse2.cpp, edgemask16_sse41.cpp

#include "edgemask.h"
#include "../functions16_sse.h"
#include "../../../common/simd.h"
#include <limits.h>

using namespace Filtering;

namespace Filtering { namespace MaskTools { namespace Filters { namespace Mask { namespace Edge {

namespace {

template <CpuFlags flags>
static MT_FORCEINLINE __m128i simd_packed_abs_epi32(__m128i a, __m128i b) {
    if (flags >= CPU_SSE4_1) {
        auto absa = _mm_abs_epi32(a);
        auto absb = _mm_abs_epi32(b);
        return _mm_packus_epi32(absa, absb);
    } else {
        auto suba = _mm_sub_epi32(_mm_setzero_si128(), a);
        auto subb = _mm_sub_epi32(_mm_setzero_si128(), b);
        auto t1 = simd_packus_epi32<flags>(a, b);
        auto t2 = simd_packus_epi32<flags>(suba, subb);
        /*auto t1 = _mm_packus_epi32(a, b);
        auto t2 = _mm_packus_epi32(suba, subb);*/
        return simd_max_epu16<flags>(t1, t2);
    }
}

template <CpuFlags flags>
static MT_FORCEINLINE __m128i simd_abs_diff_epu32(__m128i a, __m128i b) {
  if (flags >= CPU_SSE4_1) {
    return _mm_sub_epi32(_mm_max_epu32(a, b), _mm_min_epu32(a, b));
  }
  else {
    __m128i big = _mm_set_epi32(INT_MIN, INT_MIN, INT_MIN, INT_MIN);

    a = _mm_add_epi32(a, big); // re-center the variables: send 0 to INT_MIN,
    b = _mm_add_epi32(b, big); // INT_MAX to -1, etc.
    auto diff = _mm_sub_epi32(a, b); // get signed difference
    auto mask = _mm_cmpgt_epi32(b, a); // mask: need to negate difference?
    mask = _mm_andnot_si128(big, mask); // mask = 0x7ffff... if negating
    diff = _mm_xor_si128(diff, mask); // 1's complement except MSB
    diff = _mm_sub_epi32(diff, mask); // add 1 and restore MSB
    return diff;
  }
}

template <CpuFlags flags>
static MT_FORCEINLINE __m128i simd_abs_diff_epi32(__m128i a, __m128i b) {
  if (flags >= CPU_SSSE3) {
    auto diff = _mm_sub_epi32(a, b); // not correct, todo
    return _mm_abs_epi32(diff);
  }
  else {
    auto diff = _mm_sub_epi32(a, b); // not correct, todo
    // sse2 emulation of _mm_abs_epi32()
    __m128i mask = _mm_cmplt_epi32(diff, _mm_setzero_si128()); // FFFF   where diff < 0
    diff = _mm_xor_si128(diff, mask);                         // Invert where diff < 0
    mask = _mm_srli_epi32(mask, 31);                        // 0001   where a < 0
    diff = _mm_add_epi32(a, mask);                             // Add 1  where a < 0
    return diff;
  }
}

template<CpuFlags flags, int bits_per_pixel, Border borderMode>
static MT_FORCEINLINE void process_line_convolution_sse2(uint8_t *pDst, const uint8_t *pSrcp, const uint8_t *pSrc, const uint8_t *pSrcn, const int16_t matrix[10], const __m128i &lowThresh, const __m128i &highThresh, int width) {
    UNUSED(pSrcp);
#pragma warning(disable: 4310)
    auto vMax = _mm_set1_epi16(short((1 << bits_per_pixel)- 1));
#pragma warning(default: 4310)
    auto zero = _mm_setzero_si128();
    auto coef0 = _mm_set1_epi32(matrix[0]);
    auto coef1 = _mm_set1_epi32(matrix[1]);
    auto coef2 = _mm_set1_epi32(matrix[2]);
    auto coef3 = _mm_set1_epi32(matrix[3]);
    auto coef4 = _mm_set1_epi32(matrix[4]);
    auto coef5 = _mm_set1_epi32(matrix[5]);
    auto coef6 = _mm_set1_epi32(matrix[6]);
    auto coef7 = _mm_set1_epi32(matrix[7]);
    auto coef8 = _mm_set1_epi32(matrix[8]);
    
    auto divisor = _mm_set_epi32(0, 0, 0, simd_bit_scan_forward(matrix[9]));

    for (int x = 0; x < width; x+=16) {
        auto up_left = load16_one_to_left<borderMode>(pSrcp+x);
        auto up_center = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pSrcp+x));
        auto up_right = load16_one_to_right<borderMode>(pSrcp+x);

        auto middle_left = load16_one_to_left<borderMode>(pSrc+x);
        auto middle_center = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pSrc+x));
        auto middle_right = load16_one_to_right<borderMode>(pSrc+x);

        auto down_left = load16_one_to_left<borderMode>(pSrcn+x);
        auto down_center = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pSrcn+x));
        auto down_right = load16_one_to_right<borderMode>(pSrcn+x);

        auto up_left_lo = _mm_unpacklo_epi16(up_left, zero);
        auto up_left_hi = _mm_unpackhi_epi16(up_left, zero);

        auto up_center_lo = _mm_unpacklo_epi16(up_center, zero);
        auto up_center_hi = _mm_unpackhi_epi16(up_center, zero);

        auto up_right_lo = _mm_unpacklo_epi16(up_right, zero);
        auto up_right_hi = _mm_unpackhi_epi16(up_right, zero);

        auto middle_left_lo = _mm_unpacklo_epi16(middle_left, zero);
        auto middle_left_hi = _mm_unpackhi_epi16(middle_left, zero);

        auto middle_center_lo = _mm_unpacklo_epi16(middle_center, zero);
        auto middle_center_hi = _mm_unpackhi_epi16(middle_center, zero);

        auto middle_right_lo = _mm_unpacklo_epi16(middle_right, zero);
        auto middle_right_hi = _mm_unpackhi_epi16(middle_right, zero);

        auto down_left_lo = _mm_unpacklo_epi16(down_left, zero);
        auto down_left_hi = _mm_unpackhi_epi16(down_left, zero);

        auto down_center_lo = _mm_unpacklo_epi16(down_center, zero);
        auto down_center_hi = _mm_unpackhi_epi16(down_center, zero);

        auto down_right_lo = _mm_unpacklo_epi16(down_right, zero);
        auto down_right_hi = _mm_unpackhi_epi16(down_right, zero);

        auto acc_lo = simd_mullo_epi32<flags>(up_left_lo, coef0);
        acc_lo = _mm_add_epi32(acc_lo, simd_mullo_epi32<flags>(up_center_lo, coef1));
        acc_lo = _mm_add_epi32(acc_lo, simd_mullo_epi32<flags>(up_right_lo, coef2));
        acc_lo = _mm_add_epi32(acc_lo, simd_mullo_epi32<flags>(middle_left_lo, coef3));
        acc_lo = _mm_add_epi32(acc_lo, simd_mullo_epi32<flags>(middle_center_lo, coef4));
        acc_lo = _mm_add_epi32(acc_lo, simd_mullo_epi32<flags>(middle_right_lo, coef5));
        acc_lo = _mm_add_epi32(acc_lo, simd_mullo_epi32<flags>(down_left_lo, coef6));
        acc_lo = _mm_add_epi32(acc_lo, simd_mullo_epi32<flags>(down_center_lo, coef7));
        acc_lo = _mm_add_epi32(acc_lo, simd_mullo_epi32<flags>(down_right_lo, coef8));

        auto acc_hi = simd_mullo_epi32<flags>(up_left_hi, coef0);
        acc_hi = _mm_add_epi32(acc_hi, simd_mullo_epi32<flags>(up_center_hi, coef1));
        acc_hi = _mm_add_epi32(acc_hi, simd_mullo_epi32<flags>(up_right_hi, coef2));
        acc_hi = _mm_add_epi32(acc_hi, simd_mullo_epi32<flags>(middle_left_hi, coef3));
        acc_hi = _mm_add_epi32(acc_hi, simd_mullo_epi32<flags>(middle_center_hi, coef4));
        acc_hi = _mm_add_epi32(acc_hi, simd_mullo_epi32<flags>(middle_right_hi, coef5));
        acc_hi = _mm_add_epi32(acc_hi, simd_mullo_epi32<flags>(down_left_hi, coef6));
        acc_hi = _mm_add_epi32(acc_hi, simd_mullo_epi32<flags>(down_center_hi, coef7));
        acc_hi = _mm_add_epi32(acc_hi, simd_mullo_epi32<flags>(down_right_hi, coef8));

        auto shift_lo = _mm_srai_epi32(acc_lo, 31);
        auto shift_hi = _mm_srai_epi32(acc_hi, 31);
        
        acc_lo = _mm_xor_si128(acc_lo, shift_lo);
        acc_hi = _mm_xor_si128(acc_hi, shift_hi);

        acc_lo = _mm_sub_epi32(acc_lo, shift_lo);
        acc_hi = _mm_sub_epi32(acc_hi, shift_hi);

        acc_lo = _mm_srl_epi32(acc_lo, divisor);
        acc_hi = _mm_srl_epi32(acc_hi, divisor);

        auto acc = simd_packus_epi32<flags>(acc_lo, acc_hi);
        auto result = threshold16_sse2(acc, lowThresh, highThresh, vMax);

        _mm_storeu_si128(reinterpret_cast<__m128i*>(pDst+x), result);
    }
}

template<CpuFlags flags, int bits_per_pixel, Border borderMode>
static MT_FORCEINLINE void process_line_sobel_sse2(uint8_t *pDst, const uint8_t *pSrcp, const uint8_t *pSrc, const uint8_t *pSrcn, const int16_t matrix[10], const __m128i &lowThresh, const __m128i &highThresh, int width) {
    UNUSED(matrix);
#pragma warning(disable: 4310)
    auto vMax = _mm_set1_epi16(short((1 << bits_per_pixel) - 1));
#pragma warning(default: 4310)
    auto zero = _mm_setzero_si128();

    for (int x = 0; x < width; x+=16) {
        auto up_center = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pSrcp+x));

        auto middle_left = load16_one_to_left<borderMode>(pSrc+x);
        auto middle_right = load16_one_to_right<borderMode>(pSrc+x);

        auto down_center = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pSrcn+x));

        auto up_center_lo = _mm_unpacklo_epi16(up_center, zero);
        auto up_center_hi = _mm_unpackhi_epi16(up_center, zero);

        auto middle_left_lo = _mm_unpacklo_epi16(middle_left, zero);
        auto middle_left_hi = _mm_unpackhi_epi16(middle_left, zero);

        auto middle_right_lo = _mm_unpacklo_epi16(middle_right, zero);
        auto middle_right_hi = _mm_unpackhi_epi16(middle_right, zero);

        auto down_center_lo = _mm_unpacklo_epi16(down_center, zero);
        auto down_center_hi = _mm_unpackhi_epi16(down_center, zero);

        auto pos_lo = _mm_add_epi32(middle_right_lo, down_center_lo);
        auto pos_hi = _mm_add_epi32(middle_right_hi, down_center_hi);

        auto neg_lo = _mm_add_epi32(middle_left_lo, up_center_lo);
        auto neg_hi = _mm_add_epi32(middle_left_hi, up_center_hi);

        auto diff_lo = simd_abs_diff_epu32<flags>(pos_lo, neg_lo);
        auto diff_hi = simd_abs_diff_epu32<flags>(pos_hi, neg_hi);

        diff_lo = _mm_srai_epi32(diff_lo, 1);
        diff_hi = _mm_srai_epi32(diff_hi, 1);
        
        auto diff = simd_packus_epi32<flags>(diff_lo, diff_hi);
        auto result = threshold16_sse2(diff, lowThresh, highThresh, vMax);

        _mm_storeu_si128(reinterpret_cast<__m128i*>(pDst+x), result);
    }
}

template<CpuFlags flags, int bits_per_pixel, Border borderMode>
static MT_FORCEINLINE void process_line_roberts_sse2(uint8_t *pDst, const uint8_t *pSrcp, const uint8_t *pSrc, const uint8_t *pSrcn, const int16_t matrix[10], const __m128i &lowThresh, const __m128i &highThresh, int width) {
    UNUSED(pSrcp);
    UNUSED(matrix);
#pragma warning(disable: 4310)
    auto vMax = _mm_set1_epi16(short((1 << bits_per_pixel) - 1));
#pragma warning(default: 4310)
    auto zero = _mm_setzero_si128();

    for (int x = 0; x < width; x+=16) {
        auto middle_center = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pSrc+x));
        auto middle_right = load16_one_to_right<borderMode>(pSrc+x);

        auto down_center = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pSrcn+x));

        auto middle_center_lo = _mm_unpacklo_epi16(middle_center, zero);
        auto middle_center_hi = _mm_unpackhi_epi16(middle_center, zero);

        auto middle_right_lo = _mm_unpacklo_epi16(middle_right, zero);
        auto middle_right_hi = _mm_unpackhi_epi16(middle_right, zero);

        auto down_center_lo = _mm_unpacklo_epi16(down_center, zero);
        auto down_center_hi = _mm_unpackhi_epi16(down_center, zero);

        auto pos_lo = _mm_add_epi32(middle_center_lo, middle_center_lo);
        auto pos_hi = _mm_add_epi32(middle_center_hi, middle_center_hi);

        auto neg_lo = _mm_add_epi32(middle_right_lo, down_center_lo);
        auto neg_hi = _mm_add_epi32(middle_right_hi, down_center_hi);

        auto diff_lo = simd_abs_diff_epu32<flags>(pos_lo, neg_lo);
        auto diff_hi = simd_abs_diff_epu32<flags>(pos_hi, neg_hi);

        diff_lo = _mm_srai_epi32(diff_lo, 1);
        diff_hi = _mm_srai_epi32(diff_hi, 1);

        auto diff = simd_packus_epi32<flags>(diff_lo, diff_hi);
        auto result = threshold16_sse2(diff, lowThresh, highThresh, vMax);

        _mm_storeu_si128(reinterpret_cast<__m128i*>(pDst+x), result);
    }
}

template<CpuFlags flags, int bits_per_pixel, Border borderMode>
static MT_FORCEINLINE void process_line_laplace_sse2(uint8_t *pDst, const uint8_t *pSrcp, const uint8_t *pSrc, const uint8_t *pSrcn, const int16_t matrix[10], const __m128i &lowThresh, const __m128i &highThresh, int width) {
    UNUSED(pSrcp);
    UNUSED(matrix);
#pragma warning(disable: 4310)
    auto vMax = _mm_set1_epi16(short((1 << bits_per_pixel) - 1));
#pragma warning(default: 4310)
    auto zero = _mm_setzero_si128();

    for (int x = 0; x < width; x+=16) {
        auto up_left = load16_one_to_left<borderMode>(pSrcp+x);
        auto up_center = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pSrcp+x));
        auto up_right = load16_one_to_right<borderMode>(pSrcp+x);

        auto middle_left = load16_one_to_left<borderMode>(pSrc+x);
        auto middle_center = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pSrc+x));
        auto middle_right = load16_one_to_right<borderMode>(pSrc+x);

        auto down_left = load16_one_to_left<borderMode>(pSrcn+x);
        auto down_center = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pSrcn+x));
        auto down_right = load16_one_to_right<borderMode>(pSrcn+x);

        auto up_left_lo = _mm_unpacklo_epi16(up_left, zero);
        auto up_left_hi = _mm_unpackhi_epi16(up_left, zero);

        auto up_center_lo = _mm_unpacklo_epi16(up_center, zero);
        auto up_center_hi = _mm_unpackhi_epi16(up_center, zero);

        auto up_right_lo = _mm_unpacklo_epi16(up_right, zero);
        auto up_right_hi = _mm_unpackhi_epi16(up_right, zero);

        auto middle_left_lo = _mm_unpacklo_epi16(middle_left, zero);
        auto middle_left_hi = _mm_unpackhi_epi16(middle_left, zero);

        auto middle_center_lo = _mm_unpacklo_epi16(middle_center, zero);
        auto middle_center_hi = _mm_unpackhi_epi16(middle_center, zero);

        auto middle_right_lo = _mm_unpacklo_epi16(middle_right, zero);
        auto middle_right_hi = _mm_unpackhi_epi16(middle_right, zero);

        auto down_left_lo = _mm_unpacklo_epi16(down_left, zero);
        auto down_left_hi = _mm_unpackhi_epi16(down_left, zero);

        auto down_center_lo = _mm_unpacklo_epi16(down_center, zero);
        auto down_center_hi = _mm_unpackhi_epi16(down_center, zero);

        auto down_right_lo = _mm_unpacklo_epi16(down_right, zero);
        auto down_right_hi = _mm_unpackhi_epi16(down_right, zero);

        auto acc_lo = _mm_add_epi32(up_left_lo, up_center_lo);
        acc_lo = _mm_add_epi32(acc_lo, up_right_lo);
        acc_lo = _mm_add_epi32(acc_lo, middle_left_lo);
        acc_lo = _mm_add_epi32(acc_lo, middle_right_lo);
        acc_lo = _mm_add_epi32(acc_lo, down_left_lo);
        acc_lo = _mm_add_epi32(acc_lo, down_center_lo);
        acc_lo = _mm_add_epi32(acc_lo, down_right_lo);

        auto acc_hi = _mm_add_epi32(up_left_hi, up_center_hi);
        acc_hi = _mm_add_epi32(acc_hi, up_right_hi);
        acc_hi = _mm_add_epi32(acc_hi, middle_left_hi);
        acc_hi = _mm_add_epi32(acc_hi, middle_right_hi);
        acc_hi = _mm_add_epi32(acc_hi, down_left_hi);
        acc_hi = _mm_add_epi32(acc_hi, down_center_hi);
        acc_hi = _mm_add_epi32(acc_hi, down_right_hi);

        auto pos_lo = _mm_slli_epi32(middle_center_lo, 3);
        auto pos_hi = _mm_slli_epi32(middle_center_hi, 3);

        auto diff_lo = simd_abs_diff_epu32<flags>(pos_lo, acc_lo);
        auto diff_hi = simd_abs_diff_epu32<flags>(pos_hi, acc_hi);
        
        diff_lo = _mm_srai_epi32(diff_lo, 3);
        diff_hi = _mm_srai_epi32(diff_hi, 3);

        auto diff = simd_packus_epi32<flags>(diff_lo, diff_hi);
        auto result = threshold16_sse2(diff, lowThresh, highThresh, vMax);

        _mm_storeu_si128(reinterpret_cast<__m128i*>(pDst+x), result);
    }
}

template<CpuFlags flags, int bits_per_pixel, Border borderMode>
static MT_FORCEINLINE void process_line_morpho_sse2(uint8_t *pDst, const uint8_t *pSrcp, const uint8_t *pSrc, const uint8_t *pSrcn, const int16_t matrix[10], const __m128i &lowThresh, const __m128i &highThresh, int width) {
    UNUSED(matrix);
#pragma warning(disable: 4310)
    auto vMax = _mm_set1_epi16(short((1 << bits_per_pixel) - 1));
#pragma warning(default: 4310)

    for (int x = 0; x < width; x+=16) {
        auto up_left = load16_one_to_left<borderMode>(pSrcp+x);
        auto up_center = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pSrcp+x));
        auto up_right = load16_one_to_right<borderMode>(pSrcp+x);

        auto middle_left = load16_one_to_left<borderMode>(pSrc+x);
        auto middle_center = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pSrc+x));
        auto middle_right = load16_one_to_right<borderMode>(pSrc+x);

        auto down_left = load16_one_to_left<borderMode>(pSrcn+x);
        auto down_center = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pSrcn+x));
        auto down_right = load16_one_to_right<borderMode>(pSrcn+x);

        auto maxv = simd_max_epu16<flags>(middle_right, up_right);
        maxv = simd_max_epu16<flags>(maxv, down_center);
        maxv = simd_max_epu16<flags>(maxv, down_right);
        maxv = simd_max_epu16<flags>(maxv, middle_center);
        maxv = simd_max_epu16<flags>(maxv, up_left);
        maxv = simd_max_epu16<flags>(maxv, down_left);
        maxv = simd_max_epu16<flags>(maxv, up_center);
        maxv = simd_max_epu16<flags>(maxv, middle_left);

        auto minv = simd_min_epu16<flags>(middle_right, up_right);
        minv = simd_min_epu16<flags>(minv, down_center);
        minv = simd_min_epu16<flags>(minv, down_right);
        minv = simd_min_epu16<flags>(minv, middle_center);
        minv = simd_min_epu16<flags>(minv, up_left);
        minv = simd_min_epu16<flags>(minv, down_left);
        minv = simd_min_epu16<flags>(minv, up_center);
        minv = simd_min_epu16<flags>(minv, middle_left);
        
        auto diff = _mm_sub_epi16(maxv, minv);
        auto result = threshold16_sse2(diff, lowThresh, highThresh, vMax);

        _mm_storeu_si128(reinterpret_cast<__m128i*>(pDst+x), result);
    }
}

template<CpuFlags flags, int bits_per_pixel, Border borderMode>
static MT_FORCEINLINE void process_line_cartoon_sse2(uint8_t *pDst, const uint8_t *pSrcp, const uint8_t *pSrc, const uint8_t *pSrcn, const int16_t matrix[10], const __m128i &lowThresh, const __m128i &highThresh, int width) {
    UNUSED(matrix); UNUSED(pSrcn);
#pragma warning(disable: 4310)
    auto vMax = _mm_set1_epi16(short((1 << bits_per_pixel) - 1));
#pragma warning(default: 4310)
    auto zero = _mm_setzero_si128();

    for (int x = 0; x < width; x+=16) {
        auto up_center = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pSrcp+x));
        auto up_right = load16_one_to_right<borderMode>(pSrcp+x);
        auto middle_center = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pSrc+x));
        
        auto up_center_lo = _mm_unpacklo_epi16(up_center, zero);
        auto up_center_hi = _mm_unpackhi_epi16(up_center, zero);

        auto up_right_lo = _mm_unpacklo_epi16(up_right, zero);
        auto up_right_hi = _mm_unpackhi_epi16(up_right, zero);

        auto middle_center_lo = _mm_unpacklo_epi16(middle_center, zero);
        auto middle_center_hi = _mm_unpackhi_epi16(middle_center, zero);

        auto acc_lo = _mm_add_epi32(up_right_lo, middle_center_lo);
        auto acc_hi = _mm_add_epi32(up_right_hi, middle_center_hi);

        // -2*
        acc_lo = _mm_sub_epi32(acc_lo, up_center_lo);
        acc_hi = _mm_sub_epi32(acc_hi, up_center_hi);

        acc_lo = _mm_sub_epi32(acc_lo, up_center_lo);
        acc_hi = _mm_sub_epi32(acc_hi, up_center_hi);

        auto acc = simd_packus_epi32<flags>(acc_lo, acc_hi);
        auto result = threshold16_sse2(acc, lowThresh, highThresh, vMax);

        _mm_storeu_si128(reinterpret_cast<__m128i*>(pDst+x), result);
    }
}

template<CpuFlags flags, int bits_per_pixel, Border borderMode>
static MT_FORCEINLINE void process_line_prewitt_sse2(uint8_t *pDst, const uint8_t *pSrcp, const uint8_t *pSrc, const uint8_t *pSrcn, const int16_t matrix[10], const __m128i &lowThresh, const __m128i &highThresh, int width) {
    UNUSED(matrix);
#pragma warning(disable: 4310)
    auto vMax = _mm_set1_epi16(short((1 << bits_per_pixel) - 1));
#pragma warning(default: 4310)
    auto zero = _mm_setzero_si128();

    for (int x = 0; x < width; x+=16) {
        auto up_left = load16_one_to_left<borderMode>(pSrcp+x);
        auto up_center = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pSrcp+x));
        auto up_right = load16_one_to_right<borderMode>(pSrcp+x);

        auto middle_left = load16_one_to_left<borderMode>(pSrc+x);
        auto middle_right = load16_one_to_right<borderMode>(pSrc+x);

        auto down_left = load16_one_to_left<borderMode>(pSrcn+x);
        auto down_center = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pSrcn+x));
        auto down_right = load16_one_to_right<borderMode>(pSrcn+x);

        auto up_left_lo = _mm_unpacklo_epi16(up_left, zero);
        auto up_left_hi = _mm_unpackhi_epi16(up_left, zero);

        auto up_center_lo = _mm_unpacklo_epi16(up_center, zero);
        auto up_center_hi = _mm_unpackhi_epi16(up_center, zero);

        auto up_right_lo = _mm_unpacklo_epi16(up_right, zero);
        auto up_right_hi = _mm_unpackhi_epi16(up_right, zero);

        auto middle_left_lo = _mm_unpacklo_epi16(middle_left, zero);
        auto middle_left_hi = _mm_unpackhi_epi16(middle_left, zero);

        auto middle_right_lo = _mm_unpacklo_epi16(middle_right, zero);
        auto middle_right_hi = _mm_unpackhi_epi16(middle_right, zero);

        auto down_left_lo = _mm_unpacklo_epi16(down_left, zero);
        auto down_left_hi = _mm_unpackhi_epi16(down_left, zero);

        auto down_center_lo = _mm_unpacklo_epi16(down_center, zero);
        auto down_center_hi = _mm_unpackhi_epi16(down_center, zero);

        auto down_right_lo = _mm_unpacklo_epi16(down_right, zero);
        auto down_right_hi = _mm_unpackhi_epi16(down_right, zero);

        auto a21_minus_a23_lo = _mm_sub_epi32(up_center_lo, down_center_lo); // a21 - a23
        auto a21_minus_a23_hi = _mm_sub_epi32(up_center_hi, down_center_hi);
        
        auto a11_minus_a33_lo = _mm_sub_epi32(up_left_lo, down_right_lo); // a11 - a33
        auto a11_minus_a33_hi = _mm_sub_epi32(up_left_hi, down_right_hi);

        auto t1_lo = _mm_add_epi32(a21_minus_a23_lo, a11_minus_a33_lo); // a11 + a21 - a23 - a33
        auto t1_hi = _mm_add_epi32(a21_minus_a23_hi, a11_minus_a33_hi);

        auto a12_minus_a32_lo = _mm_sub_epi32(middle_left_lo, middle_right_lo); // a12 - a32
        auto a12_minus_a32_hi = _mm_sub_epi32(middle_left_hi, middle_right_hi);

        auto a13_minus_a31_lo = _mm_sub_epi32(down_left_lo, up_right_lo); // a13 - a31
        auto a13_minus_a31_hi = _mm_sub_epi32(down_left_hi, up_right_hi);

        auto t2_lo = _mm_add_epi32(a12_minus_a32_lo, a13_minus_a31_lo); //a13 + a12 - a31 - a32
        auto t2_hi = _mm_add_epi32(a12_minus_a32_hi, a13_minus_a31_hi);

        auto p135_lo = _mm_sub_epi32(t2_lo, a21_minus_a23_lo); //a13 + a12 + a23 - a31 - a32 - a21
        auto p135_hi = _mm_sub_epi32(t2_hi, a21_minus_a23_hi);

        auto p180_lo = _mm_add_epi32(t2_lo, a11_minus_a33_lo); //a11+ a12+ a13 - a31 - a32 - a33
        auto p180_hi = _mm_add_epi32(t2_hi, a11_minus_a33_hi);

        auto p90_lo = _mm_sub_epi32(a13_minus_a31_lo, t1_lo); // a13 - a31 - a11 - a21 + a23 + a33 //negative
        auto p90_hi = _mm_sub_epi32(a13_minus_a31_hi, t1_hi);

        auto p45_lo = _mm_add_epi32(t1_lo, a12_minus_a32_lo); // a12 + a11 + a21 - a33 - a32 - a23
        auto p45_hi = _mm_add_epi32(t1_hi, a12_minus_a32_hi);
        /*
        auto p45 = simd_packed_abs_epi32<CPU_SSE4_1>(p45_lo, p45_hi);
        auto p90 = simd_packed_abs_epi32<CPU_SSE4_1>(p90_lo, p90_hi);
        auto p135 = simd_packed_abs_epi32<CPU_SSE4_1>(p135_lo, p135_hi);
        auto p180 = simd_packed_abs_epi32<CPU_SSE4_1>(p180_lo, p180_hi);
        */
        
        auto p45 = simd_packed_abs_epi32<flags>(p45_lo, p45_hi);
        auto p90 = simd_packed_abs_epi32<flags>(p90_lo, p90_hi);
        auto p135 = simd_packed_abs_epi32<flags>(p135_lo, p135_hi);
        auto p180 = simd_packed_abs_epi32<flags>(p180_lo, p180_hi);
        
        auto max1 = simd_max_epu16<flags>(p45, p90);
        auto max2 = simd_max_epu16<flags>(p135, p180);

        auto result = simd_max_epu16<flags>(max1, max2);

        result = threshold16_sse2(result, lowThresh, highThresh, vMax);

        _mm_storeu_si128(reinterpret_cast<__m128i*>(pDst+x), result);
    }
}

template<CpuFlags flags, int bits_per_pixel, Border borderMode>
static MT_FORCEINLINE void process_line_half_prewitt_sse2(uint8_t *pDst, const uint8_t *pSrcp, const uint8_t *pSrc, const uint8_t *pSrcn, const int16_t matrix[10], const __m128i &lowThresh, const __m128i &highThresh, int width) {
    UNUSED(matrix);
#pragma warning(disable: 4310)
    auto vMax = _mm_set1_epi16(short((1 << bits_per_pixel) - 1));
#pragma warning(default: 4310)
    auto zero = _mm_setzero_si128();

    for (int x = 0; x < width; x+=16) {
        auto up_left = load16_one_to_left<borderMode>(pSrcp+x);
        auto up_center = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pSrcp+x));
        auto up_right = load16_one_to_right<borderMode>(pSrcp+x);

        auto middle_left = load16_one_to_left<borderMode>(pSrc+x);
        auto middle_right = load16_one_to_right<borderMode>(pSrc+x);

        auto down_left = load16_one_to_left<borderMode>(pSrcn+x);
        auto down_center = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pSrcn+x));
        auto down_right = load16_one_to_right<borderMode>(pSrcn+x);

        auto up_left_lo = _mm_unpacklo_epi16(up_left, zero);
        auto up_left_hi = _mm_unpackhi_epi16(up_left, zero);

        auto up_center_lo = _mm_unpacklo_epi16(up_center, zero);
        auto up_center_hi = _mm_unpackhi_epi16(up_center, zero);

        auto up_right_lo = _mm_unpacklo_epi16(up_right, zero);
        auto up_right_hi = _mm_unpackhi_epi16(up_right, zero);

        auto middle_left_lo = _mm_unpacklo_epi16(middle_left, zero);
        auto middle_left_hi = _mm_unpackhi_epi16(middle_left, zero);

        auto middle_right_lo = _mm_unpacklo_epi16(middle_right, zero);
        auto middle_right_hi = _mm_unpackhi_epi16(middle_right, zero);

        auto down_left_lo = _mm_unpacklo_epi16(down_left, zero);
        auto down_left_hi = _mm_unpackhi_epi16(down_left, zero);

        auto down_center_lo = _mm_unpacklo_epi16(down_center, zero);
        auto down_center_hi = _mm_unpackhi_epi16(down_center, zero);

        auto down_right_lo = _mm_unpacklo_epi16(down_right, zero);
        auto down_right_hi = _mm_unpackhi_epi16(down_right, zero);

        //a11 + 2 * (a21 - a23) + a31 - a13 - a33
        auto t1_lo = _mm_sub_epi32(up_center_lo, down_center_lo); //2 * (a21 - a23)
        auto t1_hi = _mm_sub_epi32(up_center_hi, down_center_hi);
        t1_lo = _mm_slli_epi32(t1_lo, 1);
        t1_hi = _mm_slli_epi32(t1_hi, 1);
        
        auto t2_lo = _mm_sub_epi32(up_left_lo, down_left_lo); //a11 - a13
        auto t2_hi = _mm_sub_epi32(up_left_hi, down_left_hi);
        
        auto t3_lo = _mm_sub_epi32(up_right_lo, down_right_lo); //a31 - a33
        auto t3_hi = _mm_sub_epi32(up_right_hi, down_right_hi);

        t1_lo = _mm_add_epi32(t1_lo, t2_lo);
        t1_hi = _mm_add_epi32(t1_hi, t2_hi);

        auto p90_lo = _mm_add_epi32(t1_lo, t3_lo);
        auto p90_hi = _mm_add_epi32(t1_hi, t3_hi);

        //a11 + 2 * (a12 - a32) + a13 - a31 - a33
        t1_lo = _mm_sub_epi32(middle_left_lo, middle_right_lo); //2 * (a12 - a32)
        t1_hi = _mm_sub_epi32(middle_left_hi, middle_right_hi);
        t1_lo = _mm_slli_epi32(t1_lo, 1);
        t1_hi = _mm_slli_epi32(t1_hi, 1);

        t2_lo = _mm_sub_epi32(up_left_lo, up_right_lo); //a11 - a31
        t2_hi = _mm_sub_epi32(up_left_hi, up_right_hi);

        t3_lo = _mm_sub_epi32(down_left_lo, down_right_lo); //a13 - a33
        t3_hi = _mm_sub_epi32(down_left_hi, down_right_hi);

        t1_lo = _mm_add_epi32(t1_lo, t2_lo);
        t1_hi = _mm_add_epi32(t1_hi, t2_hi);

        auto p180_lo = _mm_add_epi32(t1_lo, t3_lo);
        auto p180_hi = _mm_add_epi32(t1_hi, t3_hi);

        auto p90 = simd_packed_abs_epi32<flags>(p90_lo, p90_hi);
        auto p180 = simd_packed_abs_epi32<flags>(p180_lo, p180_hi);

        auto result = simd_max_epu16<flags>(p90, p180);

        result = threshold16_sse2(result, lowThresh, highThresh, vMax);

        _mm_storeu_si128(reinterpret_cast<__m128i*>(pDst+x), result);
    }
}

} // namespace

} } } } }

#endif // __Mt_edgemask16_SSE_H__
