#include "logic.h"

#ifdef INTEL_INTRINSICS

#include "../../common/simd_avx2.h"

using namespace Filtering;

namespace {

static MT_FORCEINLINE uint8_t add(uint8_t a, uint8_t b) { return clip<uint8_t, int>(a + (int)b); }
static MT_FORCEINLINE uint8_t sub(uint8_t a, uint8_t b) { return clip<uint8_t, int>(a - (int)b); }
static MT_FORCEINLINE uint8_t nop(uint8_t a, uint8_t b) { UNUSED(b); return a; }

static MT_FORCEINLINE uint8_t _and(uint8_t a, uint8_t b, uint8_t th1, uint8_t th2) { UNUSED(th1); UNUSED(th2); return a & b; }
static MT_FORCEINLINE uint8_t _or(uint8_t a, uint8_t b, uint8_t th1, uint8_t th2) { UNUSED(th1); UNUSED(th2); return a | b; }
static MT_FORCEINLINE uint8_t andn(uint8_t a, uint8_t b, uint8_t th1, uint8_t th2) { UNUSED(th1); UNUSED(th2); return ~a & b; }
static MT_FORCEINLINE uint8_t _xor(uint8_t a, uint8_t b, uint8_t th1, uint8_t th2) { UNUSED(th1); UNUSED(th2); return a ^ b; }

template <decltype(add) opa, decltype(add) opb>
static MT_FORCEINLINE uint8_t min_t(uint8_t a, uint8_t b, uint8_t th1, uint8_t th2) { 
    return min<uint8_t>(opa(a, th1), opb(b, th2)); 
}

template <decltype(add) opa, decltype(add) opb>
static MT_FORCEINLINE uint8_t max_t(uint8_t a, uint8_t b, uint8_t th1, uint8_t th2) { 
    return max<uint8_t>(opa(a, th1), opb(b, th2)); 
}

template <decltype(_and) op>
static void logic_t(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int nWidth, int nHeight, uint8_t nThresholdDestination, uint8_t nThresholdSource)
{
   for ( int y = 0; y < nHeight; y++ )
   {
      for ( int x = 0; x < nWidth; x++ )
         pDst[x] = op(pDst[x], pSrc[x], nThresholdDestination, nThresholdSource);
      pDst += nDstPitch;
      pSrc += nSrcPitch;
   }
}

/* avx2 */

static MT_FORCEINLINE __m256i add_avx2(__m256i a, __m256i b) { return _mm256_adds_epu8(a, b); }
static MT_FORCEINLINE __m256i sub_avx2(__m256i a, __m256i b) { return _mm256_subs_epu8(a, b); }
static MT_FORCEINLINE __m256i nop_avx2(__m256i a, __m256i) { return a; }

static MT_FORCEINLINE __m256i and_avx2_op(const __m256i &a, const __m256i &b, const __m256i&, const __m256i&) { 
    return _mm256_and_si256(a, b); 
}

static MT_FORCEINLINE __m256i or_avx2_op(const __m256i &a, const __m256i &b, const __m256i&, const __m256i&) { 
    return _mm256_or_si256(a, b); 
}

static MT_FORCEINLINE __m256i andn_avx2_op(const __m256i &a, const __m256i &b, const __m256i&, const __m256i&) { 
    return _mm256_andnot_si256(a, b); 
}

static MT_FORCEINLINE __m256i xor_avx2_op(const __m256i &a, const __m256i &b, const __m256i&, const __m256i&) { 
    return _mm256_xor_si256(a, b); 
}

template <decltype(add_avx2) opa, decltype(add_avx2) opb>
static MT_FORCEINLINE __m256i min_t_avx2(const __m256i &a, const __m256i &b, const __m256i& th1, const __m256i& th2) { 
    return _mm256_min_epu8(opa(a, th1), opb(b, th2));
}

template <decltype(add_avx2) opa, decltype(add_avx2) opb>
static MT_FORCEINLINE __m256i max_t_avx2(const __m256i &a, const __m256i &b, const __m256i& th1, const __m256i& th2) { 
    return _mm256_max_epu8(opa(a, th1), opb(b, th2));
}


template<MemoryMode mem_mode, decltype(and_avx2_op) op, decltype(_and) op_c>
    static void logic_t_avx2(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int nWidth, int nHeight, uint8_t nThresholdDestination, uint8_t nThresholdSource)
{
    int wMod32 = (nWidth / 32) * 32;
    auto pDst2 = pDst;
    auto pSrc2 = pSrc;
    auto tDest = _mm256_set1_epi8(uint8_t(nThresholdDestination));
    auto tSource = _mm256_set1_epi8(uint8_t(nThresholdSource));

    for ( int j = 0; j < nHeight; ++j ) {
        for ( int i = 0; i < wMod32; i+=32 ) {
            auto dst = simd256_load_si256<mem_mode>(pDst+i);
            auto src = simd256_load_si256<mem_mode>(pSrc+i);

            auto result = op(dst, src, tDest, tSource);

            simd256_store_si256<mem_mode>(pDst+i, result);
        }
        pDst += nDstPitch;
        pSrc += nSrcPitch;
    }

    if (nWidth > wMod32) {
        logic_t<op_c>(pDst2 + wMod32, nDstPitch, pSrc2 + wMod32, nSrcPitch, nWidth - wMod32, nHeight, nThresholdDestination, nThresholdSource);
    }
}



} // namespace

namespace Filtering { namespace MaskTools { namespace Filters { namespace Logic {

#define DEFINE_AVX2_VERSIONS(name, mem_mode) \
Processor *and_##name  = &logic_t_avx2<mem_mode, and_avx2_op, _and>; \
Processor *or_##name   = &logic_t_avx2<mem_mode, or_avx2_op, _or>; \
Processor *andn_##name = &logic_t_avx2<mem_mode, andn_avx2_op, andn>; \
Processor *xor_##name  = &logic_t_avx2<mem_mode, xor_avx2_op, _xor>;

DEFINE_AVX2_VERSIONS(avx2, MemoryMode::SSE2_UNALIGNED)

#define DEFINE_SILLY_AVX2_VERSIONS(mode, name, mem_mode) \
Processor *mode##_##name         = &logic_t_avx2<mem_mode, mode##_t_avx2<nop_avx2, nop_avx2>, mode##_t<nop, nop>>;   \
Processor *mode##sub_##name      = &logic_t_avx2<mem_mode, mode##_t_avx2<nop_avx2, sub_avx2>, mode##_t<nop, sub>>;   \
Processor *mode##add_##name      = &logic_t_avx2<mem_mode, mode##_t_avx2<nop_avx2, add_avx2>, mode##_t<nop, add>>;   \
Processor *sub##mode##_##name    = &logic_t_avx2<mem_mode, mode##_t_avx2<sub_avx2, nop_avx2>, mode##_t<sub, nop>>;   \
Processor *sub##mode##sub_##name = &logic_t_avx2<mem_mode, mode##_t_avx2<sub_avx2, sub_avx2>, mode##_t<sub, sub>>;   \
Processor *sub##mode##add_##name = &logic_t_avx2<mem_mode, mode##_t_avx2<sub_avx2, add_avx2>, mode##_t<sub, add>>;   \
Processor *add##mode##_##name    = &logic_t_avx2<mem_mode, mode##_t_avx2<add_avx2, nop_avx2>, mode##_t<add, nop>>;   \
Processor *add##mode##sub_##name = &logic_t_avx2<mem_mode, mode##_t_avx2<add_avx2, sub_avx2>, mode##_t<add, sub>>;   \
Processor *add##mode##add_##name = &logic_t_avx2<mem_mode, mode##_t_avx2<add_avx2, add_avx2>, mode##_t<add, add>>;

DEFINE_SILLY_AVX2_VERSIONS(min, avx2, MemoryMode::SSE2_UNALIGNED)
DEFINE_SILLY_AVX2_VERSIONS(max, avx2, MemoryMode::SSE2_UNALIGNED)

#undef DEFINE_SILLY_SSE2_VERSIONS
#undef DEFINE_SSE2_VERSIONS
#undef DEFINE_C_VERSIONS

} } } }

#endif // INTEL_INTRINSICS