#include "logic.h"

#ifdef INTEL_INTRINSICS

#include "../../common/simd.h"
#include "logic_c.h"

namespace {

/* sse2 */

static MT_FORCEINLINE __m128i add_sse2(__m128i a, __m128i b) { return _mm_adds_epu8(a, b); }
static MT_FORCEINLINE __m128i sub_sse2(__m128i a, __m128i b) { return _mm_subs_epu8(a, b); }
static MT_FORCEINLINE __m128i nop_sse2(__m128i a, __m128i) { return a; }

static MT_FORCEINLINE __m128i and_sse2_op(const __m128i &a, const __m128i &b, const __m128i&, const __m128i&) { 
    return _mm_and_si128(a, b); 
}

static MT_FORCEINLINE __m128i or_sse2_op(const __m128i &a, const __m128i &b, const __m128i&, const __m128i&) { 
    return _mm_or_si128(a, b); 
}

static MT_FORCEINLINE __m128i andn_sse2_op(const __m128i &a, const __m128i &b, const __m128i&, const __m128i&) { 
    return _mm_andnot_si128(a, b); 
}

static MT_FORCEINLINE __m128i xor_sse2_op(const __m128i &a, const __m128i &b, const __m128i&, const __m128i&) { 
    return _mm_xor_si128(a, b); 
}

template <decltype(add_sse2) opa, decltype(add_sse2) opb>
static MT_FORCEINLINE __m128i min_t_sse2(const __m128i &a, const __m128i &b, const __m128i& th1, const __m128i& th2) { 
    return _mm_min_epu8(opa(a, th1), opb(b, th2));
}

template <decltype(add_sse2) opa, decltype(add_sse2) opb>
static MT_FORCEINLINE __m128i max_t_sse2(const __m128i &a, const __m128i &b, const __m128i& th1, const __m128i& th2) { 
    return _mm_max_epu8(opa(a, th1), opb(b, th2));
}

template<MemoryMode mem_mode, decltype(and_sse2_op) op, decltype(_and) op_c>
    void logic_t_sse2(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int nWidth, int nHeight, uint8_t nThresholdDestination, uint8_t nThresholdSource)
{
    int wMod32 = (nWidth / 32) * 32;
    auto pDst2 = pDst;
    auto pSrc2 = pSrc;
    auto tDest = _mm_set1_epi8(uint8_t(nThresholdDestination));
    auto tSource = _mm_set1_epi8(uint8_t(nThresholdSource));

    for ( int j = 0; j < nHeight; ++j ) {
        for ( int i = 0; i < wMod32; i+=32 ) {
            _mm_prefetch(reinterpret_cast<const char*>(pDst)+i+384, _MM_HINT_T0);
            _mm_prefetch(reinterpret_cast<const char*>(pSrc)+i+384, _MM_HINT_T0);

            auto dst = simd_load_si128<mem_mode>(pDst+i);
            auto dst2 = simd_load_si128<mem_mode>(pDst+i+16);
            auto src = simd_load_si128<mem_mode>(pSrc+i);
            auto src2 = simd_load_si128<mem_mode>(pSrc+i+16);

            auto result = op(dst, src, tDest, tSource);
            auto result2 = op(dst2, src2, tDest, tSource);

            simd_store_si128<mem_mode>(pDst+i, result);
            simd_store_si128<mem_mode>(pDst+i+16, result2);
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

#define DEFINE_SSE2_VERSIONS(name, mem_mode) \
Processor *and_##name  = &logic_t_sse2<mem_mode, and_sse2_op, _and>; \
Processor *or_##name   = &logic_t_sse2<mem_mode, or_sse2_op, _or>; \
Processor *andn_##name = &logic_t_sse2<mem_mode, andn_sse2_op, andn>; \
Processor *xor_##name  = &logic_t_sse2<mem_mode, xor_sse2_op, _xor>;

DEFINE_SSE2_VERSIONS(sse2, MemoryMode::SSE2_UNALIGNED)

#define DEFINE_SILLY_SSE2_VERSIONS(mode, name, mem_mode) \
Processor *mode##_##name         = &logic_t_sse2<mem_mode, mode##_t_sse2<nop_sse2, nop_sse2>, mode##_t<nop, nop>>; \
Processor *mode##sub_##name      = &logic_t_sse2<mem_mode, mode##_t_sse2<nop_sse2, sub_sse2>, mode##_t<nop, sub>>; \
Processor *mode##add_##name      = &logic_t_sse2<mem_mode, mode##_t_sse2<nop_sse2, add_sse2>, mode##_t<nop, add>>; \
Processor *sub##mode##_##name    = &logic_t_sse2<mem_mode, mode##_t_sse2<sub_sse2, nop_sse2>, mode##_t<sub, nop>>; \
Processor *sub##mode##sub_##name = &logic_t_sse2<mem_mode, mode##_t_sse2<sub_sse2, sub_sse2>, mode##_t<sub, sub>>; \
Processor *sub##mode##add_##name = &logic_t_sse2<mem_mode, mode##_t_sse2<sub_sse2, add_sse2>, mode##_t<sub, add>>; \
Processor *add##mode##_##name    = &logic_t_sse2<mem_mode, mode##_t_sse2<add_sse2, nop_sse2>, mode##_t<add, nop>>; \
Processor *add##mode##sub_##name = &logic_t_sse2<mem_mode, mode##_t_sse2<add_sse2, sub_sse2>, mode##_t<add, sub>>; \
Processor *add##mode##add_##name = &logic_t_sse2<mem_mode, mode##_t_sse2<add_sse2, add_sse2>, mode##_t<add, add>>;

DEFINE_SILLY_SSE2_VERSIONS(min, sse2, MemoryMode::SSE2_UNALIGNED)
DEFINE_SILLY_SSE2_VERSIONS(max, sse2, MemoryMode::SSE2_UNALIGNED)
#undef DEFINE_SILLY_SSE2_VERSIONS
#undef DEFINE_SSE2_VERSIONS

} } } }

#endif // INTEL_INTRINSICS
