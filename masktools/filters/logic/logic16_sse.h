#ifndef __Mt_logic16_SSE_H__
#define __Mt_logic16_SSE_H__

// logic16: x86 kernels, instantiated in logic16_sse2.cpp, logic16_sse41.cpp

#include "../../common/simd.h"
#include "../../common/16bit.h"
#include "logic16_c.h"

namespace {

/* sse2 */

template<int bits_per_pixel>
static MT_FORCEINLINE __m128i add16_sse2(__m128i a, __m128i b)
{ 
#pragma warning(disable: 4310)
  return bits_per_pixel==16 ? _mm_adds_epu16(a, b) : _mm_min_epu16(_mm_adds_epu16(a, b),_mm_set1_epi16((short)((1 << bits_per_pixel) - 1)));
#pragma warning(default: 4310)
}
static MT_FORCEINLINE __m128i sub16_sse2(__m128i a, __m128i b) { return _mm_subs_epu16(a, b); }
static MT_FORCEINLINE __m128i nop16_sse2(__m128i a, __m128i) { return a; }

static MT_FORCEINLINE __m128i and16_sse2(const __m128i &a, const __m128i &b, const __m128i&, const __m128i&) { 
    return _mm_and_si128(a, b); 
}

static MT_FORCEINLINE __m128i or16_sse2(const __m128i &a, const __m128i &b, const __m128i&, const __m128i&) { 
    return _mm_or_si128(a, b); 
}

static MT_FORCEINLINE __m128i andn16_sse2(const __m128i &a, const __m128i &b, const __m128i&, const __m128i&) { 
    return _mm_andnot_si128(a, b); 
}

static MT_FORCEINLINE __m128i xor16_sse2(const __m128i &a, const __m128i &b, const __m128i&, const __m128i&) { 
    return _mm_xor_si128(a, b); 
}

template <decltype(nop16_sse2) opa, decltype(nop16_sse2) opb>
static MT_FORCEINLINE __m128i min_t_sse2(const __m128i &a, const __m128i &b, const __m128i& th1, const __m128i& th2) {
    return _mm_min_epu16(opa(a, th1), opb(b, th2)); // !!min_epu16: SSE4
}

template <decltype(nop16_sse2) opa, decltype(nop16_sse2) opb>
static MT_FORCEINLINE __m128i max_t_sse2(const __m128i &a, const __m128i &b, const __m128i& th1, const __m128i& th2) {
    return _mm_max_epu16(opa(a, th1), opb(b, th2)); // !!max_epu16: SSE4
}

template<decltype(and16_sse2) op, decltype(and16_c) op_c>
    void logic16_stacked_t_sse2(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int nWidth, int nHeight, int nOrigHeight, uint16_t nThresholdDestination, uint16_t nThresholdSource)
{
    int wMod8 = (nWidth / 8) * 8;
    auto pDst2 = pDst;
    auto pSrc2 = pSrc;
    auto pDstLsb = pDst + nDstPitch * nOrigHeight / 2;
    auto pSrcLsb = pSrc + nSrcPitch * nOrigHeight / 2;

    auto tDest = _mm_set1_epi16(nThresholdDestination);
    auto tSource = _mm_set1_epi16(nThresholdSource);
    auto ff = _mm_set1_epi16(0x00FF);
    auto zero = _mm_setzero_si128();

    for ( int j = 0; j < nHeight / 2; ++j ) {
        for ( int i = 0; i < wMod8; i+=8 ) {
            auto dst = read_word_stacked_simd(pDst, pDstLsb, i);
            auto src = read_word_stacked_simd(pSrc, pSrcLsb, i);

            auto result = op(dst, src, tDest, tSource);

            write_word_stacked_simd(pDst, pDstLsb, i, result, ff, zero);
        }
        pDst += nDstPitch;
        pDstLsb += nDstPitch;
        pSrc += nSrcPitch;
        pSrcLsb += nSrcPitch;
    }
    if (nWidth > wMod8) {
        logic16_stacked_t<op_c>(pDst2 + wMod8, nDstPitch, pSrc2 + wMod8, nSrcPitch, nWidth - wMod8, nHeight, nOrigHeight, nThresholdDestination, nThresholdSource);
    }
}

template<decltype(and16_sse2) op, decltype(and16_c) op_c>
void logic16_native_t_sse2(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int nWidth, int nHeight, int nOrigHeight, uint16_t nThresholdDestination, uint16_t nThresholdSource)
{
    nWidth *= 2; // really rowsize: width * sizeof(uint16), see also division at C trailer
  
    int wMod16 = (nWidth / 16) * 16;
    auto pDst2 = pDst;
    auto pSrc2 = pSrc;
    auto tDest = _mm_set1_epi16(nThresholdDestination);
    auto tSource = _mm_set1_epi16(nThresholdSource);

    for (int j = 0; j < nHeight; ++j) {
        for (int i = 0; i < wMod16; i+=16) {
            auto dst = simd_load_si128<MemoryMode::SSE2_UNALIGNED>(reinterpret_cast<const __m128i*>(pDst+i));
            auto src = simd_load_si128<MemoryMode::SSE2_UNALIGNED>(reinterpret_cast<const __m128i*>(pSrc+i));

            auto result = op(dst, src, tDest, tSource);

            simd_store_si128<MemoryMode::SSE2_UNALIGNED>(reinterpret_cast<__m128i*>(pDst+i), result);
        }
        pDst += nDstPitch;
        pSrc += nSrcPitch;
    }
    if (nWidth > wMod16) {
        logic16_native_t<op_c>(pDst2 + wMod16, nDstPitch, pSrc2 + wMod16, nSrcPitch, (nWidth - wMod16) / sizeof(uint16_t), nHeight, nOrigHeight, nThresholdDestination, nThresholdSource);
    }
}

} // namespace

#endif // __Mt_logic16_SSE_H__
