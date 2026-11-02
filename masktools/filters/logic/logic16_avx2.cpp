#include "logic.h"

#ifdef INTEL_INTRINSICS

#include "../../common/simd_avx2.h"
#include "../../common/common.h"
#include "logic16_c.h"

namespace {

// lessthan16bits: the sum is clamped to max_pixel_value (no overflow: both operands are <= max < 32768)
template<bool lessthan16bits>
static MT_FORCEINLINE __m256i add16_avx2(__m256i a, __m256i b, __m256i max_pixel_value) {
  return lessthan16bits ? _mm256_min_epu16(_mm256_adds_epu16(a, b), max_pixel_value) : _mm256_adds_epu16(a, b);
}
template<bool lessthan16bits>
static MT_FORCEINLINE __m256i sub16_avx2(__m256i a, __m256i b, __m256i) { return _mm256_subs_epu16(a, b); }
template<bool lessthan16bits>
static MT_FORCEINLINE __m256i nop16_avx2(__m256i a, __m256i, __m256i) { return a; }

static MT_FORCEINLINE __m256i and16_avx2(__m256i a, __m256i b, __m256i, __m256i, __m256i) { return _mm256_and_si256(a, b); }
static MT_FORCEINLINE __m256i or16_avx2(__m256i a, __m256i b, __m256i, __m256i, __m256i) { return _mm256_or_si256(a, b); }
static MT_FORCEINLINE __m256i andn16_avx2(__m256i a, __m256i b, __m256i, __m256i, __m256i) { return _mm256_andnot_si256(a, b); }
static MT_FORCEINLINE __m256i xor16_avx2(__m256i a, __m256i b, __m256i, __m256i, __m256i) { return _mm256_xor_si256(a, b); }

template <decltype(nop16_avx2<false>) opa, decltype(nop16_avx2<false>) opb>
static MT_FORCEINLINE __m256i min_t_avx2(__m256i a, __m256i b, __m256i th1, __m256i th2, __m256i max_pixel_value) {
    return _mm256_min_epu16(opa(a, th1, max_pixel_value), opb(b, th2, max_pixel_value));
}

template <decltype(nop16_avx2<false>) opa, decltype(nop16_avx2<false>) opb>
static MT_FORCEINLINE __m256i max_t_avx2(__m256i a, __m256i b, __m256i th1, __m256i th2, __m256i max_pixel_value) {
    return _mm256_max_epu16(opa(a, th1, max_pixel_value), opb(b, th2, max_pixel_value));
}

template<decltype(and16_avx2) op, decltype(and16_c) op_c>
static void logic16_native_t_avx2(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int nWidth, int nHeight, int bits_per_pixel, uint16_t nThresholdDestination, uint16_t nThresholdSource)
{
    const int wMod16 = (nWidth / 16) * 16; // pixels
    auto pDst2 = pDst;
    auto pSrc2 = pSrc;
    auto tDest = _mm256_set1_epi16(short(nThresholdDestination));
    auto tSource = _mm256_set1_epi16(short(nThresholdSource));
    auto max_pixel_value = _mm256_set1_epi16(short((1 << bits_per_pixel) - 1));

    for (int j = 0; j < nHeight; ++j) {
        auto d = reinterpret_cast<uint16_t *>(pDst);
        auto s = reinterpret_cast<const uint16_t *>(pSrc);
        for (int i = 0; i < wMod16; i += 16) {
            auto dst = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(d + i));
            auto src = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(s + i));
            _mm256_storeu_si256(reinterpret_cast<__m256i *>(d + i), op(dst, src, tDest, tSource, max_pixel_value));
        }
        pDst += nDstPitch;
        pSrc += nSrcPitch;
    }
    if (nWidth > wMod16) {
        logic16_native_t<op_c>(pDst2 + wMod16 * sizeof(uint16_t), nDstPitch, pSrc2 + wMod16 * sizeof(uint16_t), nSrcPitch, nWidth - wMod16, nHeight, bits_per_pixel, nThresholdDestination, nThresholdSource);
    }
}

// one template switch per call: 16 bit needs no clamping to max_pixel_value
template<decltype(and16_avx2) op16, decltype(and16_avx2) op, decltype(and16_c) op_c>
static void logic16_native_avx2(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int nWidth, int nHeight, int bits_per_pixel, uint16_t nThresholdDestination, uint16_t nThresholdSource)
{
    if (bits_per_pixel == 16)
        logic16_native_t_avx2<op16, op_c>(pDst, nDstPitch, pSrc, nSrcPitch, nWidth, nHeight, bits_per_pixel, nThresholdDestination, nThresholdSource);
    else
        logic16_native_t_avx2<op, op_c>(pDst, nDstPitch, pSrc, nSrcPitch, nWidth, nHeight, bits_per_pixel, nThresholdDestination, nThresholdSource);
}

} // namespace

namespace Filtering { namespace MaskTools { namespace Filters { namespace Logic {

Processor16 *and16_native_avx2  = &logic16_native_t_avx2<and16_avx2, and16_c>;
Processor16 *or16_native_avx2   = &logic16_native_t_avx2<or16_avx2, or16_c>;
Processor16 *andn16_native_avx2 = &logic16_native_t_avx2<andn16_avx2, andn16_c>;
Processor16 *xor16_native_avx2  = &logic16_native_t_avx2<xor16_avx2, xor16_c>;

#define LOGIC16_AVX2(name, mode, A, B) \
    Processor16 *name##_native_avx2 = &logic16_native_avx2<mode##_t_avx2<A##16_avx2<false>, B##16_avx2<false>>, \
                                                           mode##_t_avx2<A##16_avx2<true>, B##16_avx2<true>>, mode##_t<A##16_c, B##16_c>>;

#define DEFINE_AVX2_MODES(mode) \
    LOGIC16_AVX2(mode, mode, nop, nop) \
    LOGIC16_AVX2(mode##sub, mode, nop, sub) \
    LOGIC16_AVX2(mode##add, mode, nop, add) \
    LOGIC16_AVX2(sub##mode, mode, sub, nop) \
    LOGIC16_AVX2(sub##mode##sub, mode, sub, sub) \
    LOGIC16_AVX2(sub##mode##add, mode, sub, add) \
    LOGIC16_AVX2(add##mode, mode, add, nop) \
    LOGIC16_AVX2(add##mode##sub, mode, add, sub) \
    LOGIC16_AVX2(add##mode##add, mode, add, add)

DEFINE_AVX2_MODES(min)
DEFINE_AVX2_MODES(max)
#undef DEFINE_AVX2_MODES
#undef LOGIC16_AVX2

} } } }

#endif // INTEL_INTRINSICS
