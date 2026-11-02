#ifndef __Mt_logic16_SSE_H__
#define __Mt_logic16_SSE_H__

// logic16: x86 kernels, instantiated in logic16_sse2.cpp, logic16_sse41.cpp

#include "../../common/simd.h"
#include "../../common/common.h"
#include "logic16_c.h"

namespace {

/* sse2 */

// lessthan16bits: the sum is clamped to max_pixel_value (no overflow: both operands are <= max < 32768)
template<bool lessthan16bits>
static MT_FORCEINLINE __m128i add16_sse2(__m128i a, __m128i b, __m128i max_pixel_value) {
  return lessthan16bits ? _mm_min_epu16(_mm_adds_epu16(a, b), max_pixel_value) : _mm_adds_epu16(a, b); // min_epu16: SSE4.1
}
template<bool lessthan16bits>
static MT_FORCEINLINE __m128i sub16_sse2(__m128i a, __m128i b, __m128i) { return _mm_subs_epu16(a, b); }
template<bool lessthan16bits>
static MT_FORCEINLINE __m128i nop16_sse2(__m128i a, __m128i, __m128i) { return a; }

static MT_FORCEINLINE __m128i and16_sse2(__m128i a, __m128i b, __m128i, __m128i, __m128i) { return _mm_and_si128(a, b); }
static MT_FORCEINLINE __m128i or16_sse2(__m128i a, __m128i b, __m128i, __m128i, __m128i) { return _mm_or_si128(a, b); }
static MT_FORCEINLINE __m128i andn16_sse2(__m128i a, __m128i b, __m128i, __m128i, __m128i) { return _mm_andnot_si128(a, b); }
static MT_FORCEINLINE __m128i xor16_sse2(__m128i a, __m128i b, __m128i, __m128i, __m128i) { return _mm_xor_si128(a, b); }

template <decltype(nop16_sse2<false>) opa, decltype(nop16_sse2<false>) opb>
static MT_FORCEINLINE __m128i min_t_sse2(__m128i a, __m128i b, __m128i th1, __m128i th2, __m128i max_pixel_value) {
    return _mm_min_epu16(opa(a, th1, max_pixel_value), opb(b, th2, max_pixel_value)); // SSE4.1
}

template <decltype(nop16_sse2<false>) opa, decltype(nop16_sse2<false>) opb>
static MT_FORCEINLINE __m128i max_t_sse2(__m128i a, __m128i b, __m128i th1, __m128i th2, __m128i max_pixel_value) {
    return _mm_max_epu16(opa(a, th1, max_pixel_value), opb(b, th2, max_pixel_value)); // SSE4.1
}

template<decltype(and16_sse2) op, decltype(and16_c) op_c>
void logic16_native_t_sse2(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int nWidth, int nHeight, int bits_per_pixel, uint16_t nThresholdDestination, uint16_t nThresholdSource)
{
    const int wMod8 = (nWidth / 8) * 8; // pixels
    auto pDst2 = pDst;
    auto pSrc2 = pSrc;
    auto tDest = _mm_set1_epi16(short(nThresholdDestination));
    auto tSource = _mm_set1_epi16(short(nThresholdSource));
    auto max_pixel_value = _mm_set1_epi16(short((1 << bits_per_pixel) - 1));

    for (int j = 0; j < nHeight; ++j) {
        auto d = reinterpret_cast<uint16_t *>(pDst);
        auto s = reinterpret_cast<const uint16_t *>(pSrc);
        for (int i = 0; i < wMod8; i += 8) {
            auto dst = _mm_loadu_si128(reinterpret_cast<const __m128i *>(d + i));
            auto src = _mm_loadu_si128(reinterpret_cast<const __m128i *>(s + i));
            _mm_storeu_si128(reinterpret_cast<__m128i *>(d + i), op(dst, src, tDest, tSource, max_pixel_value));
        }
        pDst += nDstPitch;
        pSrc += nSrcPitch;
    }
    if (nWidth > wMod8) {
        logic16_native_t<op_c>(pDst2 + wMod8 * sizeof(uint16_t), nDstPitch, pSrc2 + wMod8 * sizeof(uint16_t), nSrcPitch, nWidth - wMod8, nHeight, bits_per_pixel, nThresholdDestination, nThresholdSource);
    }
}

// one template switch per call: 16 bit needs no clamping to max_pixel_value
template<decltype(and16_sse2) op16, decltype(and16_sse2) op, decltype(and16_c) op_c>
void logic16_native_sse2(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int nWidth, int nHeight, int bits_per_pixel, uint16_t nThresholdDestination, uint16_t nThresholdSource)
{
    if (bits_per_pixel == 16)
        logic16_native_t_sse2<op16, op_c>(pDst, nDstPitch, pSrc, nSrcPitch, nWidth, nHeight, bits_per_pixel, nThresholdDestination, nThresholdSource);
    else
        logic16_native_t_sse2<op, op_c>(pDst, nDstPitch, pSrc, nSrcPitch, nWidth, nHeight, bits_per_pixel, nThresholdDestination, nThresholdSource);
}

} // namespace

#endif // __Mt_logic16_SSE_H__
