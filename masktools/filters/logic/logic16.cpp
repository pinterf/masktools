#include "logic.h"
#include "../../common/simd.h"
#include "../../common/16bit.h"

using namespace Filtering;

// remarks:
// modes containing min, max, add require SSE4
// bits_per_pixel templating for add16: needs because of clamping to a value less than max(uint16_t)

template<int bits_per_pixel>
static MT_FORCEINLINE uint16_t add16_c(uint16_t a, uint16_t b) { return (uint16_t)min(a + (int)b, (1 << bits_per_pixel) - 1); }
static MT_FORCEINLINE uint16_t sub16_c(uint16_t a, uint16_t b) { return clip<uint16_t, int>(a - (int)b); }
static MT_FORCEINLINE uint16_t nop16_c(uint16_t a, uint16_t b) { UNUSED(b); return a; }

static MT_FORCEINLINE uint16_t and16_c(uint16_t a, uint16_t b, uint16_t th1, uint16_t th2) { UNUSED(th1); UNUSED(th2); return a & b; }
static MT_FORCEINLINE uint16_t or16_c(uint16_t a, uint16_t b, uint16_t th1, uint16_t th2) { UNUSED(th1); UNUSED(th2); return a | b; }
static MT_FORCEINLINE uint16_t andn16_c(uint16_t a, uint16_t b, uint16_t th1, uint16_t th2) { UNUSED(th1); UNUSED(th2); return a & ~b; }
static MT_FORCEINLINE uint16_t xor16_c(uint16_t a, uint16_t b, uint16_t th1, uint16_t th2) { UNUSED(th1); UNUSED(th2); return a ^ b; }

template <decltype(nop16_c) opa, decltype(nop16_c) opb>
static MT_FORCEINLINE uint16_t min_t(uint16_t a, uint16_t b, uint16_t th1, uint16_t th2) { 
    return min<uint16_t>(opa(a, th1), opb(b, th2)); 
}

template <decltype(nop16_c) opa, decltype(nop16_c) opb>
static MT_FORCEINLINE uint16_t max_t(uint16_t a, uint16_t b, uint16_t th1, uint16_t th2) { 
    return max<uint16_t>(opa(a, th1), opb(b, th2)); 
}

template <decltype(and16_c) op>
static void logic16_stacked_t(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int nWidth, int nHeight, int nOrigHeight, uint16_t nThresholdDestination, uint16_t nThresholdSource)
{
    auto pDstLsb = pDst + nDstPitch * nOrigHeight / 2;
    auto pSrcLsb = pSrc + nSrcPitch * nOrigHeight / 2;

    for (int y = 0; y < nHeight / 2; y++) {
        for (int x = 0; x < nWidth; x++) {
            uint16_t dst = read_word_stacked(pDst, pDstLsb, x);
            uint16_t src = read_word_stacked(pSrc, pSrcLsb, x);

            uint16_t avg = op(dst, src, nThresholdDestination, nThresholdSource);

            write_word_stacked(pDst, pDstLsb, x, avg);
        }
        pDst += nDstPitch;
        pDstLsb += nDstPitch;
        pSrc += nSrcPitch;
        pSrcLsb += nSrcPitch;
    }
}

template <decltype(and16_c) op>
static void logic16_native_t(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int nWidth, int nHeight, int nOrigHeight, uint16_t nThresholdDestination, uint16_t nThresholdSource)
{
    UNUSED(nOrigHeight);
    for (int y = 0; y < nHeight; y++) {
        auto pDstWord = reinterpret_cast<uint16_t*>(pDst);
        auto pSrcWord = reinterpret_cast<const uint16_t*>(pSrc);

        for (int x = 0; x < nWidth; x++) {
            pDstWord[x] = op(pDstWord[x], pSrcWord[x], nThresholdDestination, nThresholdSource);
        }
        pDst += nDstPitch;
        pSrc += nSrcPitch;
    }
}

/* sse2 */

template<int bits_per_pixel>
#if defined(GCC) || defined(CLANG)
__attribute__((__target__("sse4.1")))
#endif
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
#if defined(GCC) || defined(CLANG)
__attribute__((__target__("sse4.1")))
#endif
static MT_FORCEINLINE __m128i min_t_sse2(const __m128i &a, const __m128i &b, const __m128i& th1, const __m128i& th2) {
    return _mm_min_epu16(opa(a, th1), opb(b, th2)); // !!min_epu16: SSE4
}

template <decltype(nop16_sse2) opa, decltype(nop16_sse2) opb>
#if defined(GCC) || defined(CLANG)
__attribute__((__target__("sse4.1")))
#endif
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


namespace Filtering { namespace MaskTools { namespace Filters { namespace Logic {

Processor16 *and16_stacked_c      = &logic16_stacked_t<and16_c>;
Processor16 *or16_stacked_c       = &logic16_stacked_t<or16_c>;
Processor16 *andn16_stacked_c     = &logic16_stacked_t<andn16_c>;
Processor16 *xor16_stacked_c      = &logic16_stacked_t<xor16_c>;

Processor16 *and16_native_c  = &logic16_native_t<and16_c>;
Processor16 *or16_native_c   = &logic16_native_t<or16_c>;
Processor16 *andn16_native_c = &logic16_native_t<andn16_c>;
Processor16 *xor16_native_c  = &logic16_native_t<xor16_c>;


#define DEFINE_SILLY_C_MODE(mode, layout) \
    Processor16 *mode##_##layout##_c         = &logic16_##layout##_t<mode##_t<nop16_c, nop16_c>>;   \
    Processor16 *mode##sub_##layout##_c      = &logic16_##layout##_t<mode##_t<nop16_c, sub16_c>>;   \
    Processor16 *mode##add10_##layout##_c   = &logic16_##layout##_t<mode##_t<nop16_c, add16_c<10>>>;   \
    Processor16 *mode##add12_##layout##_c   = &logic16_##layout##_t<mode##_t<nop16_c, add16_c<12>>>;   \
    Processor16 *mode##add14_##layout##_c   = &logic16_##layout##_t<mode##_t<nop16_c, add16_c<14>>>;   \
    Processor16 *mode##add16_##layout##_c   = &logic16_##layout##_t<mode##_t<nop16_c, add16_c<16>>>;   \
    Processor16 *sub##mode##_##layout##_c    = &logic16_##layout##_t<mode##_t<sub16_c, nop16_c>>;   \
    Processor16 *sub##mode##sub_##layout##_c = &logic16_##layout##_t<mode##_t<sub16_c, sub16_c>>;   \
    Processor16 *sub##mode##add10_##layout##_c = &logic16_##layout##_t<mode##_t<sub16_c, add16_c<10>>>;   \
    Processor16 *sub##mode##add12_##layout##_c = &logic16_##layout##_t<mode##_t<sub16_c, add16_c<12>>>;   \
    Processor16 *sub##mode##add14_##layout##_c = &logic16_##layout##_t<mode##_t<sub16_c, add16_c<14>>>;   \
    Processor16 *sub##mode##add16_##layout##_c = &logic16_##layout##_t<mode##_t<sub16_c, add16_c<16>>>;   \
    Processor16 *add10##mode##_##layout##_c    = &logic16_##layout##_t<mode##_t<add16_c<10>, nop16_c>>;   \
    Processor16 *add12##mode##_##layout##_c    = &logic16_##layout##_t<mode##_t<add16_c<12>, nop16_c>>;   \
    Processor16 *add14##mode##_##layout##_c    = &logic16_##layout##_t<mode##_t<add16_c<14>, nop16_c>>;   \
    Processor16 *add16##mode##_##layout##_c    = &logic16_##layout##_t<mode##_t<add16_c<16>, nop16_c>>;   \
    Processor16 *add10##mode##sub_##layout##_c = &logic16_##layout##_t<mode##_t<add16_c<10>, sub16_c>>;   \
    Processor16 *add12##mode##sub_##layout##_c = &logic16_##layout##_t<mode##_t<add16_c<12>, sub16_c>>;   \
    Processor16 *add14##mode##sub_##layout##_c = &logic16_##layout##_t<mode##_t<add16_c<14>, sub16_c>>;   \
    Processor16 *add16##mode##sub_##layout##_c = &logic16_##layout##_t<mode##_t<add16_c<16>, sub16_c>>;   \
    Processor16 *add10##mode##add10_##layout##_c = &logic16_##layout##_t<mode##_t<add16_c<10>, add16_c<10>>>; \
    Processor16 *add12##mode##add12_##layout##_c = &logic16_##layout##_t<mode##_t<add16_c<12>, add16_c<12>>>; \
    Processor16 *add14##mode##add14_##layout##_c = &logic16_##layout##_t<mode##_t<add16_c<14>, add16_c<14>>>; \
    Processor16 *add16##mode##add16_##layout##_c = &logic16_##layout##_t<mode##_t<add16_c<16>, add16_c<16>>>;

DEFINE_SILLY_C_MODE(min, stacked)
DEFINE_SILLY_C_MODE(min, native)
DEFINE_SILLY_C_MODE(max, stacked)
DEFINE_SILLY_C_MODE(max, native)


Processor16 *and16_stacked_sse2      = &logic16_stacked_t_sse2<and16_sse2, and16_c>;
Processor16 *or16_stacked_sse2       = &logic16_stacked_t_sse2<or16_sse2, or16_c>;
Processor16 *andn16_stacked_sse2     = &logic16_stacked_t_sse2<andn16_sse2, andn16_c>;
Processor16 *xor16_stacked_sse2      = &logic16_stacked_t_sse2<xor16_sse2, xor16_c>;

Processor16 *and16_native_sse2  = &logic16_native_t_sse2<and16_sse2, and16_c>;
Processor16 *or16_native_sse2   = &logic16_native_t_sse2<or16_sse2, or16_c>;
Processor16 *andn16_native_sse2 = &logic16_native_t_sse2<andn16_sse2, andn16_c>;
Processor16 *xor16_native_sse2  = &logic16_native_t_sse2<xor16_sse2, xor16_c>;

#define DEFINE_SILLY_SSE2_VERSIONS(mode, layout) \
    Processor16 *mode##_##layout##_sse2         = &logic16_##layout##_t_sse2<mode##_t_sse2<nop16_sse2, nop16_sse2>, mode##_t<nop16_c, nop16_c>>;   \
    Processor16 *mode##sub_##layout##_sse2      = &logic16_##layout##_t_sse2<mode##_t_sse2<nop16_sse2, sub16_sse2>, mode##_t<nop16_c, sub16_c>>;   \
    Processor16 *mode##add10_##layout##_sse2   = &logic16_##layout##_t_sse2<mode##_t_sse2<nop16_sse2, add16_sse2<10>>, mode##_t<nop16_c, add16_c<10>>>;   \
    Processor16 *mode##add12_##layout##_sse2   = &logic16_##layout##_t_sse2<mode##_t_sse2<nop16_sse2, add16_sse2<12>>, mode##_t<nop16_c, add16_c<12>>>;   \
    Processor16 *mode##add14_##layout##_sse2   = &logic16_##layout##_t_sse2<mode##_t_sse2<nop16_sse2, add16_sse2<14>>, mode##_t<nop16_c, add16_c<14>>>;   \
    Processor16 *mode##add16_##layout##_sse2   = &logic16_##layout##_t_sse2<mode##_t_sse2<nop16_sse2, add16_sse2<16>>, mode##_t<nop16_c, add16_c<16>>>;   \
    Processor16 *sub##mode##_##layout##_sse2    = &logic16_##layout##_t_sse2<mode##_t_sse2<sub16_sse2, nop16_sse2>, mode##_t<sub16_c, nop16_c>>;   \
    Processor16 *sub##mode##sub_##layout##_sse2 = &logic16_##layout##_t_sse2<mode##_t_sse2<sub16_sse2, sub16_sse2>, mode##_t<sub16_c, sub16_c>>;   \
    Processor16 *sub##mode##add10_##layout##_sse2 = &logic16_##layout##_t_sse2<mode##_t_sse2<sub16_sse2, add16_sse2<10>>, mode##_t<sub16_c, add16_c<10>>>;   \
    Processor16 *sub##mode##add12_##layout##_sse2 = &logic16_##layout##_t_sse2<mode##_t_sse2<sub16_sse2, add16_sse2<12>>, mode##_t<sub16_c, add16_c<12>>>;   \
    Processor16 *sub##mode##add14_##layout##_sse2 = &logic16_##layout##_t_sse2<mode##_t_sse2<sub16_sse2, add16_sse2<14>>, mode##_t<sub16_c, add16_c<14>>>;   \
    Processor16 *sub##mode##add16_##layout##_sse2 = &logic16_##layout##_t_sse2<mode##_t_sse2<sub16_sse2, add16_sse2<16>>, mode##_t<sub16_c, add16_c<16>>>;   \
    Processor16 *add10##mode##_##layout##_sse2    = &logic16_##layout##_t_sse2<mode##_t_sse2<add16_sse2<10>, nop16_sse2>, mode##_t<add16_c<10>, nop16_c>>;   \
    Processor16 *add12##mode##_##layout##_sse2    = &logic16_##layout##_t_sse2<mode##_t_sse2<add16_sse2<12>, nop16_sse2>, mode##_t<add16_c<12>, nop16_c>>;   \
    Processor16 *add14##mode##_##layout##_sse2    = &logic16_##layout##_t_sse2<mode##_t_sse2<add16_sse2<14>, nop16_sse2>, mode##_t<add16_c<14>, nop16_c>>;   \
    Processor16 *add16##mode##_##layout##_sse2    = &logic16_##layout##_t_sse2<mode##_t_sse2<add16_sse2<16>, nop16_sse2>, mode##_t<add16_c<16>, nop16_c>>;   \
    Processor16 *add10##mode##sub_##layout##_sse2 = &logic16_##layout##_t_sse2<mode##_t_sse2<add16_sse2<10>, sub16_sse2>, mode##_t<add16_c<10>, sub16_c>>;   \
    Processor16 *add12##mode##sub_##layout##_sse2 = &logic16_##layout##_t_sse2<mode##_t_sse2<add16_sse2<12>, sub16_sse2>, mode##_t<add16_c<12>, sub16_c>>;   \
    Processor16 *add14##mode##sub_##layout##_sse2 = &logic16_##layout##_t_sse2<mode##_t_sse2<add16_sse2<14>, sub16_sse2>, mode##_t<add16_c<14>, sub16_c>>;   \
    Processor16 *add16##mode##sub_##layout##_sse2 = &logic16_##layout##_t_sse2<mode##_t_sse2<add16_sse2<16>, sub16_sse2>, mode##_t<add16_c<16>, sub16_c>>;   \
    Processor16 *add10##mode##add10_##layout##_sse2 = &logic16_##layout##_t_sse2<mode##_t_sse2<add16_sse2<10>, add16_sse2<10>>, mode##_t<add16_c<10>, add16_c<10>>>; \
    Processor16 *add12##mode##add12_##layout##_sse2 = &logic16_##layout##_t_sse2<mode##_t_sse2<add16_sse2<12>, add16_sse2<12>>, mode##_t<add16_c<12>, add16_c<12>>>; \
    Processor16 *add14##mode##add14_##layout##_sse2 = &logic16_##layout##_t_sse2<mode##_t_sse2<add16_sse2<14>, add16_sse2<14>>, mode##_t<add16_c<14>, add16_c<14>>>; \
    Processor16 *add16##mode##add16_##layout##_sse2 = &logic16_##layout##_t_sse2<mode##_t_sse2<add16_sse2<16>, add16_sse2<16>>, mode##_t<add16_c<16>, add16_c<16>>>;

DEFINE_SILLY_SSE2_VERSIONS(min, stacked)
DEFINE_SILLY_SSE2_VERSIONS(max, stacked)
DEFINE_SILLY_SSE2_VERSIONS(min, native)
DEFINE_SILLY_SSE2_VERSIONS(max, native)

#undef DEFINE_SILLY_SSE2_VERSIONS
#undef DEFINE_SILLY_C_MODE


} } } }