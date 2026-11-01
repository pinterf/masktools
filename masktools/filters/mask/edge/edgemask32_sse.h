#ifndef __Mt_edgemask32_SSE_H__
#define __Mt_edgemask32_SSE_H__

// edgemask32: x86 kernels, instantiated in edgemask32_sse2.cpp

#include "edgemask.h"
#include "../functions32_sse.h"
#include "../../../common/simd.h"

using namespace Filtering;

namespace Filtering { namespace MaskTools { namespace Filters { namespace Mask { namespace Edge {

namespace {

template<CpuFlags flags, Border borderMode>
static MT_FORCEINLINE void process_line_convolution_sse2(uint8_t *pDst, const uint8_t *pSrcp, const uint8_t *pSrc, const uint8_t *pSrcn, const float matrix[10], const __m128 &lowThresh, const __m128 &highThresh, int width) {
    UNUSED(pSrcp);
    auto coef0 = _mm_set1_ps(matrix[0]);
    auto coef1 = _mm_set1_ps(matrix[1]);
    auto coef2 = _mm_set1_ps(matrix[2]);
    auto coef3 = _mm_set1_ps(matrix[3]);
    auto coef4 = _mm_set1_ps(matrix[4]);
    auto coef5 = _mm_set1_ps(matrix[5]);
    auto coef6 = _mm_set1_ps(matrix[6]);
    auto coef7 = _mm_set1_ps(matrix[7]);
    auto coef8 = _mm_set1_ps(matrix[8]);
    
    auto divisor = _mm_set1_ps(1.0f/matrix[9]);

    for (int x = 0; x < width; x+=16) {
        auto up_left = load32_one_to_left<borderMode>(pSrcp+x);
        auto up_center = _mm_loadu_ps(reinterpret_cast<const float*>(pSrcp+x));
        auto up_right = load32_one_to_right<borderMode>(pSrcp+x);

        auto middle_left = load32_one_to_left<borderMode>(pSrc+x);
        auto middle_center = _mm_loadu_ps(reinterpret_cast<const float*>(pSrc+x));
        auto middle_right = load32_one_to_right<borderMode>(pSrc+x);

        auto down_left = load32_one_to_left<borderMode>(pSrcn+x);
        auto down_center = _mm_loadu_ps(reinterpret_cast<const float*>(pSrcn+x));
        auto down_right = load32_one_to_right<borderMode>(pSrcn+x);

        auto acc = _mm_mul_ps(up_left, coef0);
        acc = _mm_add_ps(acc, _mm_mul_ps(up_center, coef1));
        acc = _mm_add_ps(acc, _mm_mul_ps(up_right, coef2));
        acc = _mm_add_ps(acc, _mm_mul_ps(middle_left, coef3));
        acc = _mm_add_ps(acc, _mm_mul_ps(middle_center, coef4));
        acc = _mm_add_ps(acc, _mm_mul_ps(middle_right, coef5));
        acc = _mm_add_ps(acc, _mm_mul_ps(down_left, coef6));
        acc = _mm_add_ps(acc, _mm_mul_ps(down_center, coef7));
        acc = _mm_add_ps(acc, _mm_mul_ps(down_right, coef8));

        acc = simd_abs_ps(acc);
        acc = _mm_mul_ps(acc, divisor);

        auto result = threshold32_sse2<flags>(acc, lowThresh, highThresh);

        _mm_storeu_ps(reinterpret_cast<float*>(pDst+x), result);
    }
}

template<CpuFlags flags, Border borderMode>
static MT_FORCEINLINE void process_line_sobel_sse2(uint8_t *pDst, const uint8_t *pSrcp, const uint8_t *pSrc, const uint8_t *pSrcn, const float matrix[10], const __m128 &lowThresh, const __m128 &highThresh, int width) {
    UNUSED(matrix);
    auto oneHalf = _mm_set1_ps(0.5f);

    for (int x = 0; x < width; x+=16) {
        auto up_center = _mm_loadu_ps(reinterpret_cast<const float*>(pSrcp+x));

        auto middle_left = load32_one_to_left<borderMode>(pSrc+x);
        auto middle_right = load32_one_to_right<borderMode>(pSrc+x);

        auto down_center = _mm_loadu_ps(reinterpret_cast<const float*>(pSrcn+x));

        auto pos = _mm_add_ps(middle_right, down_center);

        auto neg = _mm_add_ps(middle_left, up_center);
        
        auto diff = simd_abs_diff_ps(pos, neg);
        diff = _mm_mul_ps(diff, oneHalf); //  (abs(a32 + a23 - a12 - a21) / 2.0f
        
        auto result = threshold32_sse2<flags>(diff, lowThresh, highThresh);

        _mm_storeu_ps(reinterpret_cast<float*>(pDst+x), result);
    }
}

template<CpuFlags flags, Border borderMode>
static MT_FORCEINLINE void process_line_roberts_sse2(uint8_t *pDst, const uint8_t *pSrcp, const uint8_t *pSrc, const uint8_t *pSrcn, const float matrix[10], const __m128 &lowThresh, const __m128 &highThresh, int width) {
    UNUSED(pSrcp);
    UNUSED(matrix);
    auto oneHalf = _mm_set1_ps(0.5f);

    for (int x = 0; x < width; x+=16) {
        auto middle_center = _mm_loadu_ps(reinterpret_cast<const float*>(pSrc+x));
        auto middle_right = load32_one_to_right<borderMode>(pSrc+x);
        auto down_center = _mm_loadu_ps(reinterpret_cast<const float*>(pSrcn+x));

        auto pos = _mm_add_ps(middle_center, middle_center);

        auto neg = _mm_add_ps(middle_right, down_center);

        auto diff = simd_abs_diff_ps(pos, neg);

        diff = _mm_mul_ps(diff, oneHalf); 

        auto result = threshold32_sse2<flags>(diff, lowThresh, highThresh);

        _mm_storeu_ps(reinterpret_cast<float*>(pDst+x), result);
    }
}

template<CpuFlags flags, Border borderMode>
static MT_FORCEINLINE void process_line_laplace_sse2(uint8_t *pDst, const uint8_t *pSrcp, const uint8_t *pSrc, const uint8_t *pSrcn, const float matrix[10], const __m128 &lowThresh, const __m128 &highThresh, int width) {
    UNUSED(pSrcp);
    UNUSED(matrix);
    auto oneEightth = _mm_set1_ps(1.0f/8);
    auto Eight = _mm_set1_ps(8.0f);
    // abs( (a22 * 8.0f) - a32 - a23 - a11 - a21 - a31 - a12 - a13 - a33 ) / 8.0f
    for (int x = 0; x < width; x+=16) {
        auto up_left = load32_one_to_left<borderMode>(pSrcp+x);
        auto up_center = _mm_loadu_ps(reinterpret_cast<const float*>(pSrcp+x));
        auto up_right = load32_one_to_right<borderMode>(pSrcp+x);

        auto middle_left = load32_one_to_left<borderMode>(pSrc+x);
        auto middle_center = _mm_loadu_ps(reinterpret_cast<const float*>(pSrc+x));
        auto middle_right = load32_one_to_right<borderMode>(pSrc+x);

        auto down_left = load32_one_to_left<borderMode>(pSrcn+x);
        auto down_center = _mm_loadu_ps(reinterpret_cast<const float*>(pSrcn+x));
        auto down_right = load32_one_to_right<borderMode>(pSrcn+x);

        auto acc = _mm_add_ps(up_left, up_center);
        acc = _mm_add_ps(acc, up_right);
        acc = _mm_add_ps(acc, middle_left);
        acc = _mm_add_ps(acc, middle_right);
        acc = _mm_add_ps(acc, down_left);
        acc = _mm_add_ps(acc, down_center);
        acc = _mm_add_ps(acc, down_right);

        auto pos = _mm_mul_ps(middle_center, Eight);

        auto diff = simd_abs_diff_ps(pos, acc);
        diff = _mm_mul_ps(diff, oneEightth);

        auto result = threshold32_sse2<flags>(diff, lowThresh, highThresh);

        _mm_storeu_ps(reinterpret_cast<float*>(pDst+x), result);
    }
}

template<CpuFlags flags, Border borderMode>
static MT_FORCEINLINE void process_line_morpho_sse2(uint8_t *pDst, const uint8_t *pSrcp, const uint8_t *pSrc, const uint8_t *pSrcn, const float matrix[10], const __m128 &lowThresh, const __m128 &highThresh, int width) {
    UNUSED(matrix);

    for (int x = 0; x < width; x+=16) {
        auto up_left = load32_one_to_left<borderMode>(pSrcp+x);
        auto up_center = _mm_loadu_ps(reinterpret_cast<const float*>(pSrcp+x));
        auto up_right = load32_one_to_right<borderMode>(pSrcp+x);

        auto middle_left = load32_one_to_left<borderMode>(pSrc+x);
        auto middle_center = _mm_loadu_ps(reinterpret_cast<const float*>(pSrc+x));
        auto middle_right = load32_one_to_right<borderMode>(pSrc+x);

        auto down_left = load32_one_to_left<borderMode>(pSrcn+x);
        auto down_center = _mm_loadu_ps(reinterpret_cast<const float*>(pSrcn+x));
        auto down_right = load32_one_to_right<borderMode>(pSrcn+x);

        auto maxv = _mm_max_ps(middle_right, up_right);
        maxv = _mm_max_ps(maxv, down_center);
        maxv = _mm_max_ps(maxv, down_right);
        maxv = _mm_max_ps(maxv, middle_center);
        maxv = _mm_max_ps(maxv, up_left);
        maxv = _mm_max_ps(maxv, down_left);
        maxv = _mm_max_ps(maxv, up_center);
        maxv = _mm_max_ps(maxv, middle_left);

        auto minv = _mm_min_ps(middle_right, up_right);
        minv = _mm_min_ps(minv, down_center);
        minv = _mm_min_ps(minv, down_right);
        minv = _mm_min_ps(minv, middle_center);
        minv = _mm_min_ps(minv, up_left);
        minv = _mm_min_ps(minv, down_left);
        minv = _mm_min_ps(minv, up_center);
        minv = _mm_min_ps(minv, middle_left);
        
        auto diff = _mm_sub_ps(maxv, minv);
        auto result = threshold32_sse2<flags>(diff, lowThresh, highThresh);

        _mm_storeu_ps(reinterpret_cast<float*>(pDst+x), result);
    }
}

template<CpuFlags flags, Border borderMode>
static MT_FORCEINLINE void process_line_cartoon_sse2(uint8_t *pDst, const uint8_t *pSrcp, const uint8_t *pSrc, const uint8_t *pSrcn, const float matrix[10], const __m128 &lowThresh, const __m128 &highThresh, int width) {
    UNUSED(matrix); UNUSED(pSrcn);
    /*
    float val = (a21 * 2.0f) - a22 - a31;
    return val > 0 ? 0 : threshold<float, float>(-val, nLowThreshold, nHighThreshold);
    */
    for (int x = 0; x < width; x+=16) {
        auto up_center = _mm_loadu_ps(reinterpret_cast<const float*>(pSrcp+x));
        auto up_right = load32_one_to_right<borderMode>(pSrcp+x);
        auto middle_center = _mm_loadu_ps(reinterpret_cast<const float*>(pSrc+x));
        
        auto acc = _mm_add_ps(up_right, middle_center);

        acc = _mm_sub_ps(acc, up_center); // -up_center*2
        acc = _mm_sub_ps(acc, up_center);

        auto result = threshold32_sse2<flags>(acc, lowThresh, highThresh);

        _mm_storeu_ps(reinterpret_cast<float*>(pDst+x), result);
    }
}

template<CpuFlags flags, Border borderMode>
static MT_FORCEINLINE void process_line_prewitt_sse2(uint8_t *pDst, const uint8_t *pSrcp, const uint8_t *pSrc, const uint8_t *pSrcn, const float matrix[10], const __m128 &lowThresh, const __m128 &highThresh, int width) {
    UNUSED(matrix);

    for (int x = 0; x < width; x+=16) {
        auto up_left = load32_one_to_left<borderMode>(pSrcp+x);
        auto up_center = _mm_loadu_ps(reinterpret_cast<const float*>(pSrcp+x));
        auto up_right = load32_one_to_right<borderMode>(pSrcp+x);

        auto middle_left = load32_one_to_left<borderMode>(pSrc+x);
        auto middle_right = load32_one_to_right<borderMode>(pSrc+x);

        auto down_left = load32_one_to_left<borderMode>(pSrcn+x);
        auto down_center = _mm_loadu_ps(reinterpret_cast<const float*>(pSrcn+x));
        auto down_right = load32_one_to_right<borderMode>(pSrcn+x);

        auto a21_minus_a23 = _mm_sub_ps(up_center, down_center); // a21 - a23
        
        auto a11_minus_a33 = _mm_sub_ps(up_left, down_right); // a11 - a33

        auto t1 = _mm_add_ps(a21_minus_a23, a11_minus_a33); // a11 + a21 - a23 - a33

        auto a12_minus_a32 = _mm_sub_ps(middle_left, middle_right); // a12 - a32

        auto a13_minus_a31 = _mm_sub_ps(down_left, up_right); // a13 - a31

        auto t2 = _mm_add_ps(a12_minus_a32, a13_minus_a31); //a13 + a12 - a31 - a32

        auto p135 = _mm_sub_ps(t2, a21_minus_a23); //a13 + a12 + a23 - a31 - a32 - a21

        auto p180 = _mm_add_ps(t2, a11_minus_a33); //a11+ a12+ a13 - a31 - a32 - a33

        auto p90 = _mm_sub_ps(a13_minus_a31, t1); // a13 - a31 - a11 - a21 + a23 + a33 //negative

        auto p45 = _mm_add_ps(t1, a12_minus_a32); // a12 + a11 + a21 - a33 - a32 - a23

        p45 = simd_abs_ps(p45);
        p90 = simd_abs_ps(p90);
        p135 = simd_abs_ps(p135);
        p180 = simd_abs_ps(p180);

        auto max1 = _mm_max_ps(p45, p90);
        auto max2 = _mm_max_ps(p135, p180);

        auto result = _mm_max_ps(max1, max2);

        result = threshold32_sse2<flags>(result, lowThresh, highThresh);

        _mm_storeu_ps(reinterpret_cast<float*>(pDst+x), result);
    }
}

template<CpuFlags flags, Border borderMode>
static MT_FORCEINLINE void process_line_half_prewitt_sse2(uint8_t *pDst, const uint8_t *pSrcp, const uint8_t *pSrc, const uint8_t *pSrcn, const float matrix[10], const __m128 &lowThresh, const __m128 &highThresh, int width) {
    UNUSED(matrix);

    for (int x = 0; x < width; x+=16) {
        auto up_left = load32_one_to_left<borderMode>(pSrcp+x);
        auto up_center = _mm_loadu_ps(reinterpret_cast<const float*>(pSrcp+x));
        auto up_right = load32_one_to_right<borderMode>(pSrcp+x);

        auto middle_left = load32_one_to_left<borderMode>(pSrc+x);
        auto middle_right = load32_one_to_right<borderMode>(pSrc+x);

        auto down_left = load32_one_to_left<borderMode>(pSrcn+x);
        auto down_center = _mm_loadu_ps(reinterpret_cast<const float*>(pSrcn+x));
        auto down_right = load32_one_to_right<borderMode>(pSrcn+x);

        //a11 + 2 * (a21 - a23) + a31 - a13 - a33
        auto t1 = _mm_sub_ps(up_center, down_center); //2 * (a21 - a23)
        t1 = _mm_add_ps(t1, t1); // *2
        
        auto t2 = _mm_sub_ps(up_left, down_left); //a11 - a13
        
        auto t3 = _mm_sub_ps(up_right, down_right); //a31 - a33

        t1 = _mm_add_ps(t1, t2);

        auto p90 = _mm_add_ps(t1, t3);

        //a11 + 2 * (a12 - a32) + a13 - a31 - a33
        t1 = _mm_sub_ps(middle_left, middle_right); //2 * (a12 - a32)
        t1 = _mm_add_ps(t1, t2); // 2*

        t2 = _mm_sub_ps(up_left, up_right); //a11 - a31

        t3 = _mm_sub_ps(down_left, down_right); //a13 - a33

        t1 = _mm_add_ps(t1, t2);

        auto p180 = _mm_add_ps(t1, t3);

        p90 = simd_abs_ps(p90);
        p180 = simd_abs_ps(p180);

        auto result = _mm_max_ps(p90, p180);

        result = threshold32_sse2<flags>(result, lowThresh, highThresh);

        _mm_storeu_ps(reinterpret_cast<float*>(pDst+x), result);
    }
}

} // namespace

} } } } }

#endif // __Mt_edgemask32_SSE_H__
