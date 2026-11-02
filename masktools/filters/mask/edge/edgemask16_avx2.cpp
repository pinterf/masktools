#include "edgemask.h"

#ifdef INTEL_INTRINSICS

#include "../functions16_avx2.h"
#include "../../../common/simd_avx2.h"

using namespace Filtering;


namespace Filtering { namespace MaskTools { namespace Filters { namespace Mask { namespace Edge {

namespace {

template<int bits_per_pixel>
inline uint16_t convolution(uint16_t a11, uint16_t a21, uint16_t a31, uint16_t a12, uint16_t a22, uint16_t a32, uint16_t a13, uint16_t a23, uint16_t a33, const int16_t matrix[10], int nLowThreshold, int nHighThreshold)
{
   return threshold16<bits_per_pixel>(abs((a11 * matrix[0] + a21 * matrix[1] + a31 * matrix[2] + 
                                    a12 * matrix[3] + a22 * matrix[4] + a32 * matrix[5] +
                                    a13 * matrix[6] + a23 * matrix[7] + a33 * matrix[8]) / matrix[9]), nLowThreshold, nHighThreshold);
}

template<int bits_per_pixel>
inline uint16_t sobel(uint16_t a11, uint16_t a21, uint16_t a31, uint16_t a12, uint16_t a22, uint16_t a32, uint16_t a13, uint16_t a23, uint16_t a33, const int16_t matrix[10], int nLowThreshold, int nHighThreshold)
{
   UNUSED(a11); UNUSED(a13); UNUSED(a22); UNUSED(a31); UNUSED(a33); UNUSED(matrix); 
   return threshold16<bits_per_pixel>(abs( (int)a32 + a23 - a12 - a21 ) >> 1, nLowThreshold, nHighThreshold);
}

template<int bits_per_pixel>
inline uint16_t roberts(uint16_t a11, uint16_t a21, uint16_t a31, uint16_t a12, uint16_t a22, uint16_t a32, uint16_t a13, uint16_t a23, uint16_t a33, const int16_t matrix[10], int nLowThreshold, int nHighThreshold)
{
   UNUSED(a11); UNUSED(a12); UNUSED(a13); UNUSED(a21); UNUSED(a31); UNUSED(a33); UNUSED(matrix); 
   return threshold16<bits_per_pixel>(abs( ((int)a22 << 1) - a32 - a23 ) >> 1, nLowThreshold, nHighThreshold);
}

template<int bits_per_pixel>
inline uint16_t laplace(uint16_t a11, uint16_t a21, uint16_t a31, uint16_t a12, uint16_t a22, uint16_t a32, uint16_t a13, uint16_t a23, uint16_t a33, const int16_t matrix[10], int nLowThreshold, int nHighThreshold)
{
   UNUSED(matrix); 
   return threshold16<bits_per_pixel>(abs( ((int)a22 << 3) - a32 - a23 - a11 - a21 - a31 - a12 - a13 - a33 ) >> 3, nLowThreshold, nHighThreshold);
}

template<int bits_per_pixel>
inline uint16_t morpho(uint16_t a11, uint16_t a21, uint16_t a31, uint16_t a12, uint16_t a22, uint16_t a32, uint16_t a13, uint16_t a23, uint16_t a33, const int16_t matrix[10], int nLowThreshold, int nHighThreshold)
{
   int nMin = a11, nMax = a11;

   UNUSED(matrix); 

   nMin = min<int>( nMin, a21 );
   nMax = max<int>( nMax, a21 );
   nMin = min<int>( nMin, a31 );
   nMax = max<int>( nMax, a31 );
   nMin = min<int>( nMin, a12 );
   nMax = max<int>( nMax, a12 );
   nMin = min<int>( nMin, a22 );
   nMax = max<int>( nMax, a22 );
   nMin = min<int>( nMin, a32 );
   nMax = max<int>( nMax, a32 );
   nMin = min<int>( nMin, a13 );
   nMax = max<int>( nMax, a13 );
   nMin = min<int>( nMin, a23 );
   nMax = max<int>( nMax, a23 );
   nMin = min<int>( nMin, a33 );
   nMax = max<int>( nMax, a33 );

   return threshold16<bits_per_pixel>( nMax - nMin, nLowThreshold, nHighThreshold );
}

template<int bits_per_pixel>
inline uint16_t cartoon(uint16_t a11, uint16_t a21, uint16_t a31, uint16_t a12, uint16_t a22, uint16_t a32, uint16_t a13, uint16_t a23, uint16_t a33, const int16_t matrix[10], int nLowThreshold, int nHighThreshold)
{
   int val = ((int)a21 << 1) - a22 - a31;

   UNUSED(a11); UNUSED(a12); UNUSED(a13); UNUSED(a23); UNUSED(a32); UNUSED(a33); UNUSED(matrix); 

   return val > 0 ? 0 : threshold16<bits_per_pixel>( -val, nLowThreshold, nHighThreshold );
}

template<int bits_per_pixel>
inline uint16_t prewitt(uint16_t a11, uint16_t a21, uint16_t a31, uint16_t a12, uint16_t a22, uint16_t a32, uint16_t a13, uint16_t a23, uint16_t a33, const int16_t matrix[10], int nLowThreshold, int nHighThreshold)
{
   const int p90 = a11 + a21 + a31 - a13 - a23 - a33;
   const int p180 = a11 + a12 + a13 - a31 - a32 - a33;
   const int p45 = a12 + a11 + a21 - a33 - a32 - a23;
   const int p135 = a13 + a12 + a23 - a31 - a32 - a21;

   const int max1 = max<int>( abs<int>( p90 ), abs<int>( p180 ) );
   const int max2 = max<int>( abs<int>( p45 ), abs<int>( p135 ) );
   const int maxv = max<int>( max1, max2 );

   UNUSED(a22); UNUSED(matrix); 

   return threshold16<bits_per_pixel>( maxv, nLowThreshold, nHighThreshold );
}

template<int bits_per_pixel>
inline uint16_t half_prewitt(uint16_t a11, uint16_t a21, uint16_t a31, uint16_t a12, uint16_t a22, uint16_t a32, uint16_t a13, uint16_t a23, uint16_t a33, const int16_t matrix[10], int nLowThreshold, int nHighThreshold)
{
   const int p90 = a11 + 2 * a21 + a31 - a13 - 2 * a23 - a33;
   const int p180 = a11 + 2 * a12 + a13 - a31 - 2 * a32 - a33;
   const int maxv = max<int>( abs<int>( p90 ), abs<int>( p180 ) );

   UNUSED(a22); UNUSED(matrix);
   
   return threshold16<bits_per_pixel>( maxv, nLowThreshold, nHighThreshold );
}

class Thresholds {
   uint16_t nMinThreshold, nMaxThreshold;
public:
   Thresholds(uint16_t nMinThreshold, uint16_t nMaxThreshold) :
   nMinThreshold(nMinThreshold), nMaxThreshold(nMaxThreshold)
   {
   }

   int minpitch() const { return 0; }
   int maxpitch() const { return 0; }
   void nextRow() { }
   uint16_t min(int x) const { UNUSED(x); return nMinThreshold; }
   uint16_t max(int x) const { UNUSED(x); return nMaxThreshold; }
};

template<Filters::Mask::Operator op>
void mask16_avx2_t(uint16_t *pDst, ptrdiff_t nDstPitch, const uint16_t *pSrc, ptrdiff_t nSrcPitch, const int16_t matrix[10], int nLowThreshold, int nHighThreshold, int nWidth, int nHeight)
{
   Thresholds thresholds(static_cast<uint16_t>(nLowThreshold), static_cast<uint16_t>(nHighThreshold));

   Filters::Mask::generic16_avx2_c<op, Thresholds>(pDst, nDstPitch, pSrc, nSrcPitch, thresholds, matrix, nWidth, nHeight);
}

static MT_FORCEINLINE __m256i simd256_packed_abs_epi32(__m256i a, __m256i b) {
    auto absa = _mm256_abs_epi32(a);
    auto absb = _mm256_abs_epi32(b);
    return _mm256_packus_epi32(absa, absb);
}

static MT_FORCEINLINE __m256i simd256_abs_diff_epu32(__m256i a, __m256i b) {
    return _mm256_sub_epi32(_mm256_max_epu32(a, b), _mm256_min_epu32(a, b));
}


static MT_FORCEINLINE __m256i simd_abs_diff_epi32(__m256i a, __m256i b) {
    auto diff = _mm256_sub_epi32(a, b); // not correct, todo
    return _mm256_abs_epi32(diff);
}

template<int bits_per_pixel, Border borderMode>
static MT_FORCEINLINE void process_line_convolution_avx2(uint8_t *pDst, const uint8_t *pSrcp, const uint8_t *pSrc, const uint8_t *pSrcn, const int16_t matrix[10], const __m256i &lowThresh, const __m256i &highThresh, int width) {
    UNUSED(pSrcp);
#pragma warning(disable: 4310)
    auto vMax = _mm256_set1_epi16(short((1 << bits_per_pixel)- 1));
#pragma warning(default: 4310)
    auto zero = _mm256_setzero_si256();
    auto coef0 = _mm256_set1_epi32(matrix[0]);
    auto coef1 = _mm256_set1_epi32(matrix[1]);
    auto coef2 = _mm256_set1_epi32(matrix[2]);
    auto coef3 = _mm256_set1_epi32(matrix[3]);
    auto coef4 = _mm256_set1_epi32(matrix[4]);
    auto coef5 = _mm256_set1_epi32(matrix[5]);
    auto coef6 = _mm256_set1_epi32(matrix[6]);
    auto coef7 = _mm256_set1_epi32(matrix[7]);
    auto coef8 = _mm256_set1_epi32(matrix[8]);
    
    __m128i divisor = _mm_set_epi32(0, 0, 0, simd_bit_scan_forward(matrix[9]));

    for (int x = 0; x < width; x+=32) {
        auto up_left = load16_one_to_left_si256<borderMode>(pSrcp+x);
        auto up_center = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pSrcp+x));
        auto up_right = load16_one_to_right_si256<borderMode>(pSrcp+x);

        auto middle_left = load16_one_to_left_si256<borderMode>(pSrc+x);
        auto middle_center = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pSrc+x));
        auto middle_right = load16_one_to_right_si256<borderMode>(pSrc+x);

        auto down_left = load16_one_to_left_si256<borderMode>(pSrcn+x);
        auto down_center = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pSrcn+x));
        auto down_right = load16_one_to_right_si256<borderMode>(pSrcn+x);

        auto up_left_lo = _mm256_unpacklo_epi16(up_left, zero);
        auto up_left_hi = _mm256_unpackhi_epi16(up_left, zero);

        auto up_center_lo = _mm256_unpacklo_epi16(up_center, zero);
        auto up_center_hi = _mm256_unpackhi_epi16(up_center, zero);

        auto up_right_lo = _mm256_unpacklo_epi16(up_right, zero);
        auto up_right_hi = _mm256_unpackhi_epi16(up_right, zero);

        auto middle_left_lo = _mm256_unpacklo_epi16(middle_left, zero);
        auto middle_left_hi = _mm256_unpackhi_epi16(middle_left, zero);

        auto middle_center_lo = _mm256_unpacklo_epi16(middle_center, zero);
        auto middle_center_hi = _mm256_unpackhi_epi16(middle_center, zero);

        auto middle_right_lo = _mm256_unpacklo_epi16(middle_right, zero);
        auto middle_right_hi = _mm256_unpackhi_epi16(middle_right, zero);

        auto down_left_lo = _mm256_unpacklo_epi16(down_left, zero);
        auto down_left_hi = _mm256_unpackhi_epi16(down_left, zero);

        auto down_center_lo = _mm256_unpacklo_epi16(down_center, zero);
        auto down_center_hi = _mm256_unpackhi_epi16(down_center, zero);

        auto down_right_lo = _mm256_unpacklo_epi16(down_right, zero);
        auto down_right_hi = _mm256_unpackhi_epi16(down_right, zero);

        auto acc_lo = _mm256_mullo_epi32(up_left_lo, coef0);
        acc_lo = _mm256_add_epi32(acc_lo, _mm256_mullo_epi32(up_center_lo, coef1));
        acc_lo = _mm256_add_epi32(acc_lo, _mm256_mullo_epi32(up_right_lo, coef2));
        acc_lo = _mm256_add_epi32(acc_lo, _mm256_mullo_epi32(middle_left_lo, coef3));
        acc_lo = _mm256_add_epi32(acc_lo, _mm256_mullo_epi32(middle_center_lo, coef4));
        acc_lo = _mm256_add_epi32(acc_lo, _mm256_mullo_epi32(middle_right_lo, coef5));
        acc_lo = _mm256_add_epi32(acc_lo, _mm256_mullo_epi32(down_left_lo, coef6));
        acc_lo = _mm256_add_epi32(acc_lo, _mm256_mullo_epi32(down_center_lo, coef7));
        acc_lo = _mm256_add_epi32(acc_lo, _mm256_mullo_epi32(down_right_lo, coef8));

        auto acc_hi = _mm256_mullo_epi32(up_left_hi, coef0);
        acc_hi = _mm256_add_epi32(acc_hi, _mm256_mullo_epi32(up_center_hi, coef1));
        acc_hi = _mm256_add_epi32(acc_hi, _mm256_mullo_epi32(up_right_hi, coef2));
        acc_hi = _mm256_add_epi32(acc_hi, _mm256_mullo_epi32(middle_left_hi, coef3));
        acc_hi = _mm256_add_epi32(acc_hi, _mm256_mullo_epi32(middle_center_hi, coef4));
        acc_hi = _mm256_add_epi32(acc_hi, _mm256_mullo_epi32(middle_right_hi, coef5));
        acc_hi = _mm256_add_epi32(acc_hi, _mm256_mullo_epi32(down_left_hi, coef6));
        acc_hi = _mm256_add_epi32(acc_hi, _mm256_mullo_epi32(down_center_hi, coef7));
        acc_hi = _mm256_add_epi32(acc_hi, _mm256_mullo_epi32(down_right_hi, coef8));

        auto shift_lo = _mm256_srai_epi32(acc_lo, 31);
        auto shift_hi = _mm256_srai_epi32(acc_hi, 31);
        
        acc_lo = _mm256_xor_si256(acc_lo, shift_lo);
        acc_hi = _mm256_xor_si256(acc_hi, shift_hi);

        acc_lo = _mm256_sub_epi32(acc_lo, shift_lo);
        acc_hi = _mm256_sub_epi32(acc_hi, shift_hi);

        acc_lo = _mm256_srl_epi32(acc_lo, divisor);
        acc_hi = _mm256_srl_epi32(acc_hi, divisor);

        auto acc = _mm256_packus_epi32(acc_lo, acc_hi);
        auto result = threshold16_avx2(acc, lowThresh, highThresh, vMax);

        _mm256_storeu_si256(reinterpret_cast<__m256i*>(pDst+x), result);
    }
}

template<int bits_per_pixel, Border borderMode>
static MT_FORCEINLINE void process_line_sobel_avx2(uint8_t *pDst, const uint8_t *pSrcp, const uint8_t *pSrc, const uint8_t *pSrcn, const int16_t matrix[10], const __m256i &lowThresh, const __m256i &highThresh, int width) {
    UNUSED(matrix);
#pragma warning(disable: 4310)
    auto vMax = _mm256_set1_epi16(short((1 << bits_per_pixel) - 1));
#pragma warning(default: 4310)
    auto zero = _mm256_setzero_si256();

    for (int x = 0; x < width; x+=32) {
        auto up_center = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pSrcp+x));

        auto middle_left = load16_one_to_left_si256<borderMode>(pSrc+x);
        auto middle_right = load16_one_to_right_si256<borderMode>(pSrc+x);

        auto down_center = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pSrcn+x));

        auto up_center_lo = _mm256_unpacklo_epi16(up_center, zero);
        auto up_center_hi = _mm256_unpackhi_epi16(up_center, zero);

        auto middle_left_lo = _mm256_unpacklo_epi16(middle_left, zero);
        auto middle_left_hi = _mm256_unpackhi_epi16(middle_left, zero);

        auto middle_right_lo = _mm256_unpacklo_epi16(middle_right, zero);
        auto middle_right_hi = _mm256_unpackhi_epi16(middle_right, zero);

        auto down_center_lo = _mm256_unpacklo_epi16(down_center, zero);
        auto down_center_hi = _mm256_unpackhi_epi16(down_center, zero);

        auto pos_lo = _mm256_add_epi32(middle_right_lo, down_center_lo);
        auto pos_hi = _mm256_add_epi32(middle_right_hi, down_center_hi);

        auto neg_lo = _mm256_add_epi32(middle_left_lo, up_center_lo);
        auto neg_hi = _mm256_add_epi32(middle_left_hi, up_center_hi);

        auto diff_lo = simd256_abs_diff_epu32(pos_lo, neg_lo);
        auto diff_hi = simd256_abs_diff_epu32(pos_hi, neg_hi);

        diff_lo = _mm256_srai_epi32(diff_lo, 1);
        diff_hi = _mm256_srai_epi32(diff_hi, 1);
        
        auto diff = _mm256_packus_epi32(diff_lo, diff_hi);
        auto result = threshold16_avx2(diff, lowThresh, highThresh, vMax);

        _mm256_storeu_si256(reinterpret_cast<__m256i*>(pDst+x), result);
    }
}

template<int bits_per_pixel, Border borderMode>
static MT_FORCEINLINE void process_line_roberts_avx2(uint8_t *pDst, const uint8_t *pSrcp, const uint8_t *pSrc, const uint8_t *pSrcn, const int16_t matrix[10], const __m256i &lowThresh, const __m256i &highThresh, int width) {
    UNUSED(pSrcp);
    UNUSED(matrix);
#pragma warning(disable: 4310)
    auto vMax = _mm256_set1_epi16(short((1 << bits_per_pixel) - 1));
#pragma warning(default: 4310)
    auto zero = _mm256_setzero_si256();

    for (int x = 0; x < width; x+=32) {
        auto middle_center = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pSrc+x));
        auto middle_right = load16_one_to_right_si256<borderMode>(pSrc+x);

        auto down_center = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pSrcn+x));

        auto middle_center_lo = _mm256_unpacklo_epi16(middle_center, zero);
        auto middle_center_hi = _mm256_unpackhi_epi16(middle_center, zero);

        auto middle_right_lo = _mm256_unpacklo_epi16(middle_right, zero);
        auto middle_right_hi = _mm256_unpackhi_epi16(middle_right, zero);

        auto down_center_lo = _mm256_unpacklo_epi16(down_center, zero);
        auto down_center_hi = _mm256_unpackhi_epi16(down_center, zero);

        auto pos_lo = _mm256_add_epi32(middle_center_lo, middle_center_lo);
        auto pos_hi = _mm256_add_epi32(middle_center_hi, middle_center_hi);

        auto neg_lo = _mm256_add_epi32(middle_right_lo, down_center_lo);
        auto neg_hi = _mm256_add_epi32(middle_right_hi, down_center_hi);

        auto diff_lo = simd256_abs_diff_epu32(pos_lo, neg_lo);
        auto diff_hi = simd256_abs_diff_epu32(pos_hi, neg_hi);

        diff_lo = _mm256_srai_epi32(diff_lo, 1);
        diff_hi = _mm256_srai_epi32(diff_hi, 1);

        auto diff = _mm256_packus_epi32(diff_lo, diff_hi);
        auto result = threshold16_avx2(diff, lowThresh, highThresh, vMax);

        _mm256_storeu_si256(reinterpret_cast<__m256i*>(pDst+x), result);
    }
}

template<int bits_per_pixel, Border borderMode>
static MT_FORCEINLINE void process_line_laplace_avx2(uint8_t *pDst, const uint8_t *pSrcp, const uint8_t *pSrc, const uint8_t *pSrcn, const int16_t matrix[10], const __m256i &lowThresh, const __m256i &highThresh, int width) {
    UNUSED(pSrcp);
    UNUSED(matrix);
#pragma warning(disable: 4310)
    auto vMax = _mm256_set1_epi16(short((1 << bits_per_pixel) - 1));
#pragma warning(default: 4310)
    auto zero = _mm256_setzero_si256();

    for (int x = 0; x < width; x+=32) {
        auto up_left = load16_one_to_left_si256<borderMode>(pSrcp+x);
        auto up_center = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pSrcp+x));
        auto up_right = load16_one_to_right_si256<borderMode>(pSrcp+x);

        auto middle_left = load16_one_to_left_si256<borderMode>(pSrc+x);
        auto middle_center = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pSrc+x));
        auto middle_right = load16_one_to_right_si256<borderMode>(pSrc+x);

        auto down_left = load16_one_to_left_si256<borderMode>(pSrcn+x);
        auto down_center = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pSrcn+x));
        auto down_right = load16_one_to_right_si256<borderMode>(pSrcn+x);

        auto up_left_lo = _mm256_unpacklo_epi16(up_left, zero);
        auto up_left_hi = _mm256_unpackhi_epi16(up_left, zero);

        auto up_center_lo = _mm256_unpacklo_epi16(up_center, zero);
        auto up_center_hi = _mm256_unpackhi_epi16(up_center, zero);

        auto up_right_lo = _mm256_unpacklo_epi16(up_right, zero);
        auto up_right_hi = _mm256_unpackhi_epi16(up_right, zero);

        auto middle_left_lo = _mm256_unpacklo_epi16(middle_left, zero);
        auto middle_left_hi = _mm256_unpackhi_epi16(middle_left, zero);

        auto middle_center_lo = _mm256_unpacklo_epi16(middle_center, zero);
        auto middle_center_hi = _mm256_unpackhi_epi16(middle_center, zero);

        auto middle_right_lo = _mm256_unpacklo_epi16(middle_right, zero);
        auto middle_right_hi = _mm256_unpackhi_epi16(middle_right, zero);

        auto down_left_lo = _mm256_unpacklo_epi16(down_left, zero);
        auto down_left_hi = _mm256_unpackhi_epi16(down_left, zero);

        auto down_center_lo = _mm256_unpacklo_epi16(down_center, zero);
        auto down_center_hi = _mm256_unpackhi_epi16(down_center, zero);

        auto down_right_lo = _mm256_unpacklo_epi16(down_right, zero);
        auto down_right_hi = _mm256_unpackhi_epi16(down_right, zero);

        auto acc_lo = _mm256_add_epi32(up_left_lo, up_center_lo);
        acc_lo = _mm256_add_epi32(acc_lo, up_right_lo);
        acc_lo = _mm256_add_epi32(acc_lo, middle_left_lo);
        acc_lo = _mm256_add_epi32(acc_lo, middle_right_lo);
        acc_lo = _mm256_add_epi32(acc_lo, down_left_lo);
        acc_lo = _mm256_add_epi32(acc_lo, down_center_lo);
        acc_lo = _mm256_add_epi32(acc_lo, down_right_lo);

        auto acc_hi = _mm256_add_epi32(up_left_hi, up_center_hi);
        acc_hi = _mm256_add_epi32(acc_hi, up_right_hi);
        acc_hi = _mm256_add_epi32(acc_hi, middle_left_hi);
        acc_hi = _mm256_add_epi32(acc_hi, middle_right_hi);
        acc_hi = _mm256_add_epi32(acc_hi, down_left_hi);
        acc_hi = _mm256_add_epi32(acc_hi, down_center_hi);
        acc_hi = _mm256_add_epi32(acc_hi, down_right_hi);

        auto pos_lo = _mm256_slli_epi32(middle_center_lo, 3);
        auto pos_hi = _mm256_slli_epi32(middle_center_hi, 3);

        auto diff_lo = simd256_abs_diff_epu32(pos_lo, acc_lo);
        auto diff_hi = simd256_abs_diff_epu32(pos_hi, acc_hi);
        
        diff_lo = _mm256_srai_epi32(diff_lo, 3);
        diff_hi = _mm256_srai_epi32(diff_hi, 3);

        auto diff = _mm256_packus_epi32(diff_lo, diff_hi);
        auto result = threshold16_avx2(diff, lowThresh, highThresh, vMax);

        _mm256_storeu_si256(reinterpret_cast<__m256i*>(pDst+x), result);
    }
}

template<int bits_per_pixel, Border borderMode>
static MT_FORCEINLINE void process_line_morpho_avx2(uint8_t *pDst, const uint8_t *pSrcp, const uint8_t *pSrc, const uint8_t *pSrcn, const int16_t matrix[10], const __m256i &lowThresh, const __m256i &highThresh, int width) {
    UNUSED(matrix);
#pragma warning(disable: 4310)
    auto vMax = _mm256_set1_epi16(short((1 << bits_per_pixel) - 1));
#pragma warning(default: 4310)

    for (int x = 0; x < width; x+=32) {
        auto up_left = load16_one_to_left_si256<borderMode>(pSrcp+x);
        auto up_center = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pSrcp+x));
        auto up_right = load16_one_to_right_si256<borderMode>(pSrcp+x);

        auto middle_left = load16_one_to_left_si256<borderMode>(pSrc+x);
        auto middle_center = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pSrc+x));
        auto middle_right = load16_one_to_right_si256<borderMode>(pSrc+x);

        auto down_left = load16_one_to_left_si256<borderMode>(pSrcn+x);
        auto down_center = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pSrcn+x));
        auto down_right = load16_one_to_right_si256<borderMode>(pSrcn+x);

        auto maxv = _mm256_max_epu16(middle_right, up_right);
        maxv = _mm256_max_epu16(maxv, down_center);
        maxv = _mm256_max_epu16(maxv, down_right);
        maxv = _mm256_max_epu16(maxv, middle_center);
        maxv = _mm256_max_epu16(maxv, up_left);
        maxv = _mm256_max_epu16(maxv, down_left);
        maxv = _mm256_max_epu16(maxv, up_center);
        maxv = _mm256_max_epu16(maxv, middle_left);

        auto minv = _mm256_min_epu16(middle_right, up_right);
        minv = _mm256_min_epu16(minv, down_center);
        minv = _mm256_min_epu16(minv, down_right);
        minv = _mm256_min_epu16(minv, middle_center);
        minv = _mm256_min_epu16(minv, up_left);
        minv = _mm256_min_epu16(minv, down_left);
        minv = _mm256_min_epu16(minv, up_center);
        minv = _mm256_min_epu16(minv, middle_left);
        
        auto diff = _mm256_sub_epi16(maxv, minv);
        auto result = threshold16_avx2(diff, lowThresh, highThresh, vMax);

        _mm256_storeu_si256(reinterpret_cast<__m256i*>(pDst+x), result);
    }
}

template<int bits_per_pixel, Border borderMode>
static MT_FORCEINLINE void process_line_cartoon_avx2(uint8_t *pDst, const uint8_t *pSrcp, const uint8_t *pSrc, const uint8_t *pSrcn, const int16_t matrix[10], const __m256i &lowThresh, const __m256i &highThresh, int width) {
    UNUSED(matrix); UNUSED(pSrcn);
#pragma warning(disable: 4310)
    auto vMax = _mm256_set1_epi16(short((1 << bits_per_pixel) - 1));
#pragma warning(default: 4310)
    auto zero = _mm256_setzero_si256();

    for (int x = 0; x < width; x+=32) {
        auto up_center = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pSrcp+x));
        auto up_right = load16_one_to_right_si256<borderMode>(pSrcp+x);
        auto middle_center = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pSrc+x));
        
        auto up_center_lo = _mm256_unpacklo_epi16(up_center, zero);
        auto up_center_hi = _mm256_unpackhi_epi16(up_center, zero);

        auto up_right_lo = _mm256_unpacklo_epi16(up_right, zero);
        auto up_right_hi = _mm256_unpackhi_epi16(up_right, zero);

        auto middle_center_lo = _mm256_unpacklo_epi16(middle_center, zero);
        auto middle_center_hi = _mm256_unpackhi_epi16(middle_center, zero);

        auto acc_lo = _mm256_add_epi32(up_right_lo, middle_center_lo);
        auto acc_hi = _mm256_add_epi32(up_right_hi, middle_center_hi);

        // -2*
        acc_lo = _mm256_sub_epi32(acc_lo, up_center_lo);
        acc_hi = _mm256_sub_epi32(acc_hi, up_center_hi);

        acc_lo = _mm256_sub_epi32(acc_lo, up_center_lo);
        acc_hi = _mm256_sub_epi32(acc_hi, up_center_hi);

        auto acc = _mm256_packus_epi32(acc_lo, acc_hi);
        auto result = threshold16_avx2(acc, lowThresh, highThresh, vMax);

        _mm256_storeu_si256(reinterpret_cast<__m256i*>(pDst+x), result);
    }
}

template<int bits_per_pixel, Border borderMode>
static MT_FORCEINLINE void process_line_prewitt_avx2(uint8_t *pDst, const uint8_t *pSrcp, const uint8_t *pSrc, const uint8_t *pSrcn, const int16_t matrix[10], const __m256i &lowThresh, const __m256i &highThresh, int width) {
    UNUSED(matrix);
#pragma warning(disable: 4310)
    auto vMax = _mm256_set1_epi16(short((1 << bits_per_pixel) - 1));
#pragma warning(default: 4310)
    auto zero = _mm256_setzero_si256();

    for (int x = 0; x < width; x+=32) {
        auto up_left = load16_one_to_left_si256<borderMode>(pSrcp+x);
        auto up_center = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pSrcp+x));
        auto up_right = load16_one_to_right_si256<borderMode>(pSrcp+x);

        auto middle_left = load16_one_to_left_si256<borderMode>(pSrc+x);
        auto middle_right = load16_one_to_right_si256<borderMode>(pSrc+x);

        auto down_left = load16_one_to_left_si256<borderMode>(pSrcn+x);
        auto down_center = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pSrcn+x));
        auto down_right = load16_one_to_right_si256<borderMode>(pSrcn+x);

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

        auto a21_minus_a23_lo = _mm256_sub_epi32(up_center_lo, down_center_lo); // a21 - a23
        auto a21_minus_a23_hi = _mm256_sub_epi32(up_center_hi, down_center_hi);
        
        auto a11_minus_a33_lo = _mm256_sub_epi32(up_left_lo, down_right_lo); // a11 - a33
        auto a11_minus_a33_hi = _mm256_sub_epi32(up_left_hi, down_right_hi);

        auto t1_lo = _mm256_add_epi32(a21_minus_a23_lo, a11_minus_a33_lo); // a11 + a21 - a23 - a33
        auto t1_hi = _mm256_add_epi32(a21_minus_a23_hi, a11_minus_a33_hi);

        auto a12_minus_a32_lo = _mm256_sub_epi32(middle_left_lo, middle_right_lo); // a12 - a32
        auto a12_minus_a32_hi = _mm256_sub_epi32(middle_left_hi, middle_right_hi);

        auto a13_minus_a31_lo = _mm256_sub_epi32(down_left_lo, up_right_lo); // a13 - a31
        auto a13_minus_a31_hi = _mm256_sub_epi32(down_left_hi, up_right_hi);

        auto t2_lo = _mm256_add_epi32(a12_minus_a32_lo, a13_minus_a31_lo); //a13 + a12 - a31 - a32
        auto t2_hi = _mm256_add_epi32(a12_minus_a32_hi, a13_minus_a31_hi);

        auto p135_lo = _mm256_sub_epi32(t2_lo, a21_minus_a23_lo); //a13 + a12 + a23 - a31 - a32 - a21
        auto p135_hi = _mm256_sub_epi32(t2_hi, a21_minus_a23_hi);

        auto p180_lo = _mm256_add_epi32(t2_lo, a11_minus_a33_lo); //a11+ a12+ a13 - a31 - a32 - a33
        auto p180_hi = _mm256_add_epi32(t2_hi, a11_minus_a33_hi);

        auto p90_lo = _mm256_sub_epi32(a13_minus_a31_lo, t1_lo); // a13 - a31 - a11 - a21 + a23 + a33 //negative
        auto p90_hi = _mm256_sub_epi32(a13_minus_a31_hi, t1_hi);

        auto p45_lo = _mm256_add_epi32(t1_lo, a12_minus_a32_lo); // a12 + a11 + a21 - a33 - a32 - a23
        auto p45_hi = _mm256_add_epi32(t1_hi, a12_minus_a32_hi);
        /*
        auto p45 = simd256_packed_abs_epi32<CPU_SSE4_1>(p45_lo, p45_hi);
        auto p90 = simd256_packed_abs_epi32<CPU_SSE4_1>(p90_lo, p90_hi);
        auto p135 = simd256_packed_abs_epi32<CPU_SSE4_1>(p135_lo, p135_hi);
        auto p180 = simd256_packed_abs_epi32<CPU_SSE4_1>(p180_lo, p180_hi);
        */
        
        auto p45 = simd256_packed_abs_epi32(p45_lo, p45_hi);
        auto p90 = simd256_packed_abs_epi32(p90_lo, p90_hi);
        auto p135 = simd256_packed_abs_epi32(p135_lo, p135_hi);
        auto p180 = simd256_packed_abs_epi32(p180_lo, p180_hi);
        
        auto max1 = _mm256_max_epu16(p45, p90);
        auto max2 = _mm256_max_epu16(p135, p180);

        auto result = _mm256_max_epu16(max1, max2);

        result = threshold16_avx2(result, lowThresh, highThresh, vMax);

        _mm256_storeu_si256(reinterpret_cast<__m256i*>(pDst+x), result);
    }
}

template<int bits_per_pixel, Border borderMode>
static MT_FORCEINLINE void process_line_half_prewitt_avx2(uint8_t *pDst, const uint8_t *pSrcp, const uint8_t *pSrc, const uint8_t *pSrcn, const int16_t matrix[10], const __m256i &lowThresh, const __m256i &highThresh, int width) {
    UNUSED(matrix);
#pragma warning(disable: 4310)
    auto vMax = _mm256_set1_epi16(short((1 << bits_per_pixel) - 1));
#pragma warning(default: 4310)
    auto zero = _mm256_setzero_si256();

    for (int x = 0; x < width; x+=32) {
        auto up_left = load16_one_to_left_si256<borderMode>(pSrcp+x);
        auto up_center = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pSrcp+x));
        auto up_right = load16_one_to_right_si256<borderMode>(pSrcp+x);

        auto middle_left = load16_one_to_left_si256<borderMode>(pSrc+x);
        auto middle_right = load16_one_to_right_si256<borderMode>(pSrc+x);

        auto down_left = load16_one_to_left_si256<borderMode>(pSrcn+x);
        auto down_center = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pSrcn+x));
        auto down_right = load16_one_to_right_si256<borderMode>(pSrcn+x);

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

        //a11 + 2 * (a21 - a23) + a31 - a13 - a33
        auto t1_lo = _mm256_sub_epi32(up_center_lo, down_center_lo); //2 * (a21 - a23)
        auto t1_hi = _mm256_sub_epi32(up_center_hi, down_center_hi);
        t1_lo = _mm256_slli_epi32(t1_lo, 1);
        t1_hi = _mm256_slli_epi32(t1_hi, 1);
        
        auto t2_lo = _mm256_sub_epi32(up_left_lo, down_left_lo); //a11 - a13
        auto t2_hi = _mm256_sub_epi32(up_left_hi, down_left_hi);
        
        auto t3_lo = _mm256_sub_epi32(up_right_lo, down_right_lo); //a31 - a33
        auto t3_hi = _mm256_sub_epi32(up_right_hi, down_right_hi);

        t1_lo = _mm256_add_epi32(t1_lo, t2_lo);
        t1_hi = _mm256_add_epi32(t1_hi, t2_hi);

        auto p90_lo = _mm256_add_epi32(t1_lo, t3_lo);
        auto p90_hi = _mm256_add_epi32(t1_hi, t3_hi);

        //a11 + 2 * (a12 - a32) + a13 - a31 - a33
        t1_lo = _mm256_sub_epi32(middle_left_lo, middle_right_lo); //2 * (a12 - a32)
        t1_hi = _mm256_sub_epi32(middle_left_hi, middle_right_hi);
        t1_lo = _mm256_slli_epi32(t1_lo, 1);
        t1_hi = _mm256_slli_epi32(t1_hi, 1);

        t2_lo = _mm256_sub_epi32(up_left_lo, up_right_lo); //a11 - a31
        t2_hi = _mm256_sub_epi32(up_left_hi, up_right_hi);

        t3_lo = _mm256_sub_epi32(down_left_lo, down_right_lo); //a13 - a33
        t3_hi = _mm256_sub_epi32(down_left_hi, down_right_hi);

        t1_lo = _mm256_add_epi32(t1_lo, t2_lo);
        t1_hi = _mm256_add_epi32(t1_hi, t2_hi);

        auto p180_lo = _mm256_add_epi32(t1_lo, t3_lo);
        auto p180_hi = _mm256_add_epi32(t1_hi, t3_hi);

        auto p90 = simd256_packed_abs_epi32(p90_lo, p90_hi);
        auto p180 = simd256_packed_abs_epi32(p180_lo, p180_hi);

        auto result = _mm256_max_epu16(p90, p180);

        result = threshold16_avx2(result, lowThresh, highThresh, vMax);

        _mm256_storeu_si256(reinterpret_cast<__m256i*>(pDst+x), result);
    }
}

} // namespace

using namespace Filters::Mask;

#define DEFINE_AVX2_VERSIONS(name) \
Processor16 *name##_10_avx2 = &generic16_avx2<10, \
    process_line_##name##_avx2<10, Border::Left>, \
    process_line_##name##_avx2<10, Border::None>, \
    process_line_##name##_avx2<10, Border::Right> \
>; \
Processor16 *name##_12_avx2 = &generic16_avx2<12, \
    process_line_##name##_avx2<12, Border::Left>, \
    process_line_##name##_avx2<12, Border::None>, \
    process_line_##name##_avx2<12, Border::Right> \
>; \
Processor16 *name##_14_avx2 = &generic16_avx2<14, \
    process_line_##name##_avx2<14, Border::Left>, \
    process_line_##name##_avx2<14, Border::None>, \
    process_line_##name##_avx2<14, Border::Right> \
>; \
Processor16 *name##_16_avx2 = &generic16_avx2<16, \
    process_line_##name##_avx2<16, Border::Left>, \
    process_line_##name##_avx2<16, Border::None>, \
    process_line_##name##_avx2<16, Border::Right> \
>; \

DEFINE_AVX2_VERSIONS(sobel)
DEFINE_AVX2_VERSIONS(roberts)
DEFINE_AVX2_VERSIONS(laplace)
DEFINE_AVX2_VERSIONS(prewitt)
DEFINE_AVX2_VERSIONS(half_prewitt)

DEFINE_AVX2_VERSIONS(convolution)
DEFINE_AVX2_VERSIONS(morpho)
DEFINE_AVX2_VERSIONS(cartoon)

#undef DEFINE_AVX2_VERSIONS


} } } } }

#endif // INTEL_INTRINSICS