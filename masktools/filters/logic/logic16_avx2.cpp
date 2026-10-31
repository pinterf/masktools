#include "logic.h"

#ifdef INTEL_INTRINSICS

#include "../../common/simd_avx2.h"
#include "../../common/common.h"

using namespace Filtering;

namespace {

// remarks:
// bits_per_pixel templating for add16: needs because of clamping to a value less than max(uint16_t)

template<int bits_per_pixel>
static MT_FORCEINLINE uint16_t add16_c(uint16_t a, uint16_t b) { return (uint16_t)min(a + (int)b, (1 << bits_per_pixel) - 1); }
static MT_FORCEINLINE uint16_t sub16_c(uint16_t a, uint16_t b) { return clip<uint16_t, int>(a - (int)b); }
static MT_FORCEINLINE uint16_t nop16_c(uint16_t a, uint16_t b) { UNUSED(b); return a; }

static MT_FORCEINLINE uint16_t and16_c(uint16_t a, uint16_t b, uint16_t th1, uint16_t th2) { UNUSED(th1); UNUSED(th2); return a & b; }
static MT_FORCEINLINE uint16_t or16_c(uint16_t a, uint16_t b, uint16_t th1, uint16_t th2) { UNUSED(th1); UNUSED(th2); return a | b; }
static MT_FORCEINLINE uint16_t andn16_c(uint16_t a, uint16_t b, uint16_t th1, uint16_t th2) { UNUSED(th1); UNUSED(th2); return ~a & b; }
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
static void logic16_native_t_c(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int nWidth, int nHeight, int nOrigHeight, uint16_t nThresholdDestination, uint16_t nThresholdSource)
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

/* avx2 */

template<int bits_per_pixel>
static MT_FORCEINLINE __m256i add16_avx2(__m256i a, __m256i b) 
{ 
#pragma warning(disable: 4310)
  return bits_per_pixel==16 ? _mm256_adds_epu16(a, b) : _mm256_min_epu16(_mm256_adds_epu16(a, b),_mm256_set1_epi16((short)((1 << bits_per_pixel) - 1)));
#pragma warning(default: 4310)
}
static MT_FORCEINLINE __m256i sub16_avx2(__m256i a, __m256i b) { return _mm256_subs_epu16(a, b); }
static MT_FORCEINLINE __m256i nop16_avx2(__m256i a, __m256i) { return a; }

static MT_FORCEINLINE __m256i and16_avx2(const __m256i &a, const __m256i &b, const __m256i&, const __m256i&) { 
    return _mm256_and_si256(a, b); 
}

static MT_FORCEINLINE __m256i or16_avx2(const __m256i &a, const __m256i &b, const __m256i&, const __m256i&) { 
    return _mm256_or_si256(a, b); 
}

static MT_FORCEINLINE __m256i andn16_avx2(const __m256i &a, const __m256i &b, const __m256i&, const __m256i&) { 
    return _mm256_andnot_si256(a, b); 
}

static MT_FORCEINLINE __m256i xor16_avx2(const __m256i &a, const __m256i &b, const __m256i&, const __m256i&) { 
    return _mm256_xor_si256(a, b); 
}

template <decltype(nop16_avx2) opa, decltype(nop16_avx2) opb>
static MT_FORCEINLINE __m256i min_t_avx2(const __m256i &a, const __m256i &b, const __m256i& th1, const __m256i& th2) { 
    return _mm256_min_epu16(opa(a, th1), opb(b, th2)); // !!min_epu16: SSE4
}

template <decltype(nop16_avx2) opa, decltype(nop16_avx2) opb>
static MT_FORCEINLINE __m256i max_t_avx2(const __m256i &a, const __m256i &b, const __m256i& th1, const __m256i& th2) { 
    return _mm256_max_epu16(opa(a, th1), opb(b, th2)); 
}


template<decltype(and16_avx2) op, decltype(and16_c) op_c>
static void logic16_native_t_avx2(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int nWidth, int nHeight, int nOrigHeight, uint16_t nThresholdDestination, uint16_t nThresholdSource)
{
    nWidth *= 2; // really rowsize: width * sizeof(uint16), see also division at C trailer
  
    int wMod32 = (nWidth / 32) * 32;
    auto pDst2 = pDst;
    auto pSrc2 = pSrc;
    auto tDest = _mm256_set1_epi16(nThresholdDestination);
    auto tSource = _mm256_set1_epi16(nThresholdSource);

    for (int j = 0; j < nHeight; ++j) {
        for (int i = 0; i < wMod32; i+=32) {
            auto dst = simd256_load_si256<MemoryMode::SSE2_UNALIGNED>(reinterpret_cast<const __m256i*>(pDst+i));
            auto src = simd256_load_si256<MemoryMode::SSE2_UNALIGNED>(reinterpret_cast<const __m256i*>(pSrc+i));

            auto result = op(dst, src, tDest, tSource);

            simd256_store_si256<MemoryMode::SSE2_UNALIGNED>(reinterpret_cast<__m256i*>(pDst+i), result);
        }
        pDst += nDstPitch;
        pSrc += nSrcPitch;
    }
    if (nWidth > wMod32) {
        logic16_native_t_c<op_c>(pDst2 + wMod32, nDstPitch, pSrc2 + wMod32, nSrcPitch, (nWidth - wMod32) / sizeof(uint16_t), nHeight, nOrigHeight, nThresholdDestination, nThresholdSource);
    }
}


} // namespace

namespace Filtering { namespace MaskTools { namespace Filters { namespace Logic {

Processor16 *and16_native_avx2  = &logic16_native_t_avx2<and16_avx2, and16_c>;
Processor16 *or16_native_avx2   = &logic16_native_t_avx2<or16_avx2, or16_c>;
Processor16 *andn16_native_avx2 = &logic16_native_t_avx2<andn16_avx2, andn16_c>;
Processor16 *xor16_native_avx2  = &logic16_native_t_avx2<xor16_avx2, xor16_c>;

#define DEFINE_SILLY_AVX2_VERSIONS(mode) \
    Processor16 *mode##_native_avx2         = &logic16_native_t_avx2<mode##_t_avx2<nop16_avx2, nop16_avx2>, mode##_t<nop16_c, nop16_c>>;   \
    Processor16 *mode##sub_native_avx2      = &logic16_native_t_avx2<mode##_t_avx2<nop16_avx2, sub16_avx2>, mode##_t<nop16_c, sub16_c>>;   \
    Processor16 *mode##add10_native_avx2   = &logic16_native_t_avx2<mode##_t_avx2<nop16_avx2, add16_avx2<10>>, mode##_t<nop16_c, add16_c<10>>>;   \
    Processor16 *mode##add12_native_avx2   = &logic16_native_t_avx2<mode##_t_avx2<nop16_avx2, add16_avx2<12>>, mode##_t<nop16_c, add16_c<12>>>;   \
    Processor16 *mode##add14_native_avx2   = &logic16_native_t_avx2<mode##_t_avx2<nop16_avx2, add16_avx2<14>>, mode##_t<nop16_c, add16_c<14>>>;   \
    Processor16 *mode##add16_native_avx2   = &logic16_native_t_avx2<mode##_t_avx2<nop16_avx2, add16_avx2<16>>, mode##_t<nop16_c, add16_c<16>>>;   \
    Processor16 *sub##mode##_native_avx2    = &logic16_native_t_avx2<mode##_t_avx2<sub16_avx2, nop16_avx2>, mode##_t<sub16_c, nop16_c>>;   \
    Processor16 *sub##mode##sub_native_avx2 = &logic16_native_t_avx2<mode##_t_avx2<sub16_avx2, sub16_avx2>, mode##_t<sub16_c, sub16_c>>;   \
    Processor16 *sub##mode##add10_native_avx2 = &logic16_native_t_avx2<mode##_t_avx2<sub16_avx2, add16_avx2<10>>, mode##_t<sub16_c, add16_c<10>>>;   \
    Processor16 *sub##mode##add12_native_avx2 = &logic16_native_t_avx2<mode##_t_avx2<sub16_avx2, add16_avx2<12>>, mode##_t<sub16_c, add16_c<12>>>;   \
    Processor16 *sub##mode##add14_native_avx2 = &logic16_native_t_avx2<mode##_t_avx2<sub16_avx2, add16_avx2<14>>, mode##_t<sub16_c, add16_c<14>>>;   \
    Processor16 *sub##mode##add16_native_avx2 = &logic16_native_t_avx2<mode##_t_avx2<sub16_avx2, add16_avx2<16>>, mode##_t<sub16_c, add16_c<16>>>;   \
    Processor16 *add10##mode##_native_avx2    = &logic16_native_t_avx2<mode##_t_avx2<add16_avx2<10>, nop16_avx2>, mode##_t<add16_c<10>, nop16_c>>;   \
    Processor16 *add12##mode##_native_avx2    = &logic16_native_t_avx2<mode##_t_avx2<add16_avx2<12>, nop16_avx2>, mode##_t<add16_c<12>, nop16_c>>;   \
    Processor16 *add14##mode##_native_avx2    = &logic16_native_t_avx2<mode##_t_avx2<add16_avx2<14>, nop16_avx2>, mode##_t<add16_c<14>, nop16_c>>;   \
    Processor16 *add16##mode##_native_avx2    = &logic16_native_t_avx2<mode##_t_avx2<add16_avx2<16>, nop16_avx2>, mode##_t<add16_c<16>, nop16_c>>;   \
    Processor16 *add10##mode##sub_native_avx2 = &logic16_native_t_avx2<mode##_t_avx2<add16_avx2<10>, sub16_avx2>, mode##_t<add16_c<10>, sub16_c>>;   \
    Processor16 *add12##mode##sub_native_avx2 = &logic16_native_t_avx2<mode##_t_avx2<add16_avx2<12>, sub16_avx2>, mode##_t<add16_c<12>, sub16_c>>;   \
    Processor16 *add14##mode##sub_native_avx2 = &logic16_native_t_avx2<mode##_t_avx2<add16_avx2<14>, sub16_avx2>, mode##_t<add16_c<14>, sub16_c>>;   \
    Processor16 *add16##mode##sub_native_avx2 = &logic16_native_t_avx2<mode##_t_avx2<add16_avx2<16>, sub16_avx2>, mode##_t<add16_c<16>, sub16_c>>;   \
    Processor16 *add10##mode##add10_native_avx2 = &logic16_native_t_avx2<mode##_t_avx2<add16_avx2<10>, add16_avx2<10>>, mode##_t<add16_c<10>, add16_c<10>>>; \
    Processor16 *add12##mode##add12_native_avx2 = &logic16_native_t_avx2<mode##_t_avx2<add16_avx2<12>, add16_avx2<12>>, mode##_t<add16_c<12>, add16_c<12>>>; \
    Processor16 *add14##mode##add14_native_avx2 = &logic16_native_t_avx2<mode##_t_avx2<add16_avx2<14>, add16_avx2<14>>, mode##_t<add16_c<14>, add16_c<14>>>; \
    Processor16 *add16##mode##add16_native_avx2 = &logic16_native_t_avx2<mode##_t_avx2<add16_avx2<16>, add16_avx2<16>>, mode##_t<add16_c<16>, add16_c<16>>>;

DEFINE_SILLY_AVX2_VERSIONS(min)
DEFINE_SILLY_AVX2_VERSIONS(max)

#undef DEFINE_SILLY_AVX2_VERSIONS


} } } }

#endif // INTEL_INTRINSICS