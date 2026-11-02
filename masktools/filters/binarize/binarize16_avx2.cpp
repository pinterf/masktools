#include "binarize.h"

#ifdef INTEL_INTRINSICS

#include "../../common/simd_avx2.h"
#include "../../common/common.h"
#include "binarize16_c.h"

namespace {

// 10-16 bit AVX2 ops. lessthan16bits: the all-ones results are masked to max_pixel_value
template<bool lessthan16bits>
static MT_FORCEINLINE __m256i binarize_upper_avx2_op(__m256i x, __m256i t, __m256i, __m256i max_pixel_value) {
    auto r = _mm256_cmpeq_epi16(_mm256_subs_epu16(x, t), _mm256_setzero_si256()); // FFFF where x <= t
    return lessthan16bits ? _mm256_and_si256(r, max_pixel_value) : r;
}

template<bool lessthan16bits>
static MT_FORCEINLINE __m256i binarize_lower_avx2_op(__m256i x, __m256i, __m256i halfrange, __m256i max_pixel_value) {
    auto r = _mm256_cmpgt_epi16(_mm256_add_epi16(x, _mm256_set1_epi16(-32768)), halfrange); // FFFF where x > t (signed cmp)
    return lessthan16bits ? _mm256_and_si256(r, max_pixel_value) : r;
}

template<bool lessthan16bits>
static MT_FORCEINLINE __m256i binarize_0_x_avx2_op(__m256i x, __m256i t, __m256i halfrange, __m256i max_pixel_value) {
    return _mm256_and_si256(binarize_upper_avx2_op<false>(x, t, halfrange, max_pixel_value), x);
}

template<bool lessthan16bits>
static MT_FORCEINLINE __m256i binarize_t_x_avx2_op(__m256i x, __m256i t, __m256i, __m256i) {
    return _mm256_min_epu16(t, x);
}

template<bool lessthan16bits>
static MT_FORCEINLINE __m256i binarize_x_0_avx2_op(__m256i x, __m256i t, __m256i halfrange, __m256i max_pixel_value) {
    return _mm256_and_si256(binarize_lower_avx2_op<false>(x, t, halfrange, max_pixel_value), x);
}

template<bool lessthan16bits>
static MT_FORCEINLINE __m256i binarize_x_t_avx2_op(__m256i x, __m256i t, __m256i, __m256i) {
    return _mm256_max_epu16(t, x);
}

template<bool lessthan16bits>
static MT_FORCEINLINE __m256i binarize_t_0_avx2_op(__m256i x, __m256i t, __m256i halfrange, __m256i max_pixel_value) {
    return _mm256_and_si256(binarize_lower_avx2_op<false>(x, t, halfrange, max_pixel_value), t);
}

template<bool lessthan16bits>
static MT_FORCEINLINE __m256i binarize_0_t_avx2_op(__m256i x, __m256i t, __m256i halfrange, __m256i max_pixel_value) {
    return _mm256_and_si256(binarize_upper_avx2_op<false>(x, t, halfrange, max_pixel_value), t);
}

template<bool lessthan16bits>
static MT_FORCEINLINE __m256i binarize_x_255_avx2_op(__m256i x, __m256i t, __m256i halfrange, __m256i max_pixel_value) {
    return _mm256_or_si256(binarize_upper_avx2_op<lessthan16bits>(x, t, halfrange, max_pixel_value), x);
}

template<bool lessthan16bits>
static MT_FORCEINLINE __m256i binarize_255_x_avx2_op(__m256i x, __m256i t, __m256i halfrange, __m256i max_pixel_value) {
    return _mm256_or_si256(binarize_lower_avx2_op<lessthan16bits>(x, t, halfrange, max_pixel_value), x);
}

template<bool lessthan16bits>
static MT_FORCEINLINE __m256i binarize_t_255_avx2_op(__m256i x, __m256i t, __m256i halfrange, __m256i max_pixel_value) {
    return _mm256_or_si256(binarize_upper_avx2_op<lessthan16bits>(x, t, halfrange, max_pixel_value), t);
}

template<bool lessthan16bits>
static MT_FORCEINLINE __m256i binarize_255_t_avx2_op(__m256i x, __m256i t, __m256i halfrange, __m256i max_pixel_value) {
    return _mm256_or_si256(binarize_lower_avx2_op<lessthan16bits>(x, t, halfrange, max_pixel_value), t);
}

template<bool lessthan16bits, decltype(binarize_upper_avx2_op<false>) op, Operator op_c>
void binarize_avx2_native_t(uint8_t *pDst, ptrdiff_t nDstPitch, uint16_t nThreshold, int nWidth, int nHeight, int bits_per_pixel)
{
    const int wMod16 = (nWidth / 16) * 16; // pixels
    auto pDst2 = pDst;
    auto t = _mm256_set1_epi16(short(nThreshold));
    auto halfrange = _mm256_add_epi16(t, _mm256_set1_epi16(-32768)); // shifted for the signed compare
    auto max_pixel_value = _mm256_set1_epi16(short((1 << bits_per_pixel) - 1));

    for (int j = 0; j < nHeight; ++j) {
        auto p = reinterpret_cast<uint16_t *>(pDst);
        for (int i = 0; i < wMod16; i += 16) {
            auto src = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(p + i));
            _mm256_storeu_si256(reinterpret_cast<__m256i *>(p + i), op(src, t, halfrange, max_pixel_value));
        }
        pDst += nDstPitch;
    }
    if (nWidth > wMod16)
        binarize_native_t<op_c>(pDst2 + wMod16 * sizeof(uint16_t), nDstPitch, nThreshold, nWidth - wMod16, nHeight, bits_per_pixel);
}

// one template switch per call: 16 bit needs no masking with max_pixel_value
template<decltype(binarize_upper_avx2_op<false>) op16, decltype(binarize_upper_avx2_op<true>) op, Operator op_c>
void binarize_avx2_native(uint8_t *pDst, ptrdiff_t nDstPitch, uint16_t nThreshold, int nWidth, int nHeight, int bits_per_pixel)
{
    if (bits_per_pixel == 16)
        binarize_avx2_native_t<false, op16, op_c>(pDst, nDstPitch, nThreshold, nWidth, nHeight, bits_per_pixel);
    else
        binarize_avx2_native_t<true, op, op_c>(pDst, nDstPitch, nThreshold, nWidth, nHeight, bits_per_pixel);
}

} // namespace

namespace Filtering { namespace MaskTools { namespace Filters { namespace Binarize {

#define DEFINE_PROCESSORS(name) \
Processor16 *binarize_##name##_native_avx2 = &binarize_avx2_native<binarize_##name##_avx2_op<false>, binarize_##name##_avx2_op<true>, binarize_##name>;

DEFINE_PROCESSORS(upper)
DEFINE_PROCESSORS(lower)
DEFINE_PROCESSORS(0_x)
DEFINE_PROCESSORS(t_x)
DEFINE_PROCESSORS(x_0)
DEFINE_PROCESSORS(x_t)
DEFINE_PROCESSORS(t_0)
DEFINE_PROCESSORS(0_t)
DEFINE_PROCESSORS(x_255)
DEFINE_PROCESSORS(t_255)
DEFINE_PROCESSORS(255_x)
DEFINE_PROCESSORS(255_t)
#undef DEFINE_PROCESSORS

} } } }

#endif // INTEL_INTRINSICS
