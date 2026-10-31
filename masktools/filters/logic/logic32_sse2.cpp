#include "logic.h"

#ifdef INTEL_INTRINSICS

#include "../../common/simd.h"
#include "logic32_c.h"

namespace {

/* sse2 */

static MT_FORCEINLINE __m128 add_sse2(__m128 a, __m128 b) { return _mm_add_ps(a, b); } // no clamp or saturation on float
static MT_FORCEINLINE __m128 sub_sse2(__m128 a, __m128 b) { return _mm_sub_ps(a, b); }
static MT_FORCEINLINE __m128 nop_sse2(__m128 a, __m128) { return a; }

static MT_FORCEINLINE __m128 and_sse2_op(const __m128 &a, const __m128 &b, const __m128&, const __m128&) { 
    return _mm_and_ps(a, b); 
}

static MT_FORCEINLINE __m128 or_sse2_op(const __m128 &a, const __m128 &b, const __m128&, const __m128&) { 
    return _mm_or_ps(a, b); 
}

static MT_FORCEINLINE __m128 andn_sse2_op(const __m128 &a, const __m128 &b, const __m128&, const __m128&) { 
    return _mm_andnot_ps(a, b); 
}

static MT_FORCEINLINE __m128 xor_sse2_op(const __m128 &a, const __m128 &b, const __m128&, const __m128&) { 
    return _mm_xor_ps(a, b); 
}

template <decltype(add_sse2) opa, decltype(add_sse2) opb>
static MT_FORCEINLINE __m128 min_t_sse2(const __m128 &a, const __m128 &b, const __m128& th1, const __m128& th2) { 
    return _mm_min_ps(opa(a, th1), opb(b, th2));
}

template <decltype(add_sse2) opa, decltype(add_sse2) opb>
static MT_FORCEINLINE __m128 max_t_sse2(const __m128 &a, const __m128 &b, const __m128& th1, const __m128& th2) { 
    return _mm_max_ps(opa(a, th1), opb(b, th2));
}

template<MemoryMode mem_mode, decltype(and_sse2_op) op, decltype(_and) op_c>
    void logic_t_sse2(float *pDst8, ptrdiff_t nDstPitch, const float *pSrc8, ptrdiff_t nSrcPitch, int nWidth, int nHeight, float nThresholdDestination, float nThresholdSource)
{
    uint8_t *pDst = reinterpret_cast<uint8_t *>(pDst8);
    const uint8_t *pSrc = reinterpret_cast<const uint8_t *>(pSrc8);

    nWidth *= sizeof(float);

    int wMod32 = (nWidth / 32) * 32;
    auto pDst2 = pDst;
    auto pSrc2 = pSrc;
    auto tDest = _mm_set1_ps(nThresholdDestination);
    auto tSource = _mm_set1_ps(nThresholdSource);

    for ( int j = 0; j < nHeight; ++j ) {
        for ( int i = 0; i < wMod32; i+=32 ) {
            _mm_prefetch(reinterpret_cast<const char*>(pDst)+i+384, _MM_HINT_T0);
            _mm_prefetch(reinterpret_cast<const char*>(pSrc)+i+384, _MM_HINT_T0);

            auto dst = simd_load_ps<mem_mode>(pDst+i);
            auto dst2 = simd_load_ps<mem_mode>(pDst+i+16);
            auto src = simd_load_ps<mem_mode>(pSrc+i);
            auto src2 = simd_load_ps<mem_mode>(pSrc+i+16);

            auto result = op(dst, src, tDest, tSource);
            auto result2 = op(dst2, src2, tDest, tSource);

            simd_store_ps<mem_mode>(pDst+i, result);
            simd_store_ps<mem_mode>(pDst+i+16, result2);
        }
        pDst += nDstPitch;
        pSrc += nSrcPitch;
    }

    if (nWidth > wMod32) {
        logic_t<op_c>((float *)(pDst2 + wMod32), nDstPitch, (float *)(pSrc2 + wMod32), nSrcPitch, (nWidth - wMod32) / sizeof(float), nHeight, nThresholdDestination, nThresholdSource);
    }
}

} // namespace

namespace Filtering { namespace MaskTools { namespace Filters { namespace Logic {

#define DEFINE_SSE2_VERSIONS(name, mem_mode) \
Processor32 *and_##name  = &logic_t_sse2<mem_mode, and_sse2_op, _and>; \
Processor32 *or_##name   = &logic_t_sse2<mem_mode, or_sse2_op, _or>; \
Processor32 *andn_##name = &logic_t_sse2<mem_mode, andn_sse2_op, andn>; \
Processor32 *xor_##name  = &logic_t_sse2<mem_mode, xor_sse2_op, _xor>;

DEFINE_SSE2_VERSIONS(32_sse2, MemoryMode::SSE2_UNALIGNED)
DEFINE_SSE2_VERSIONS(32_asse2, MemoryMode::SSE2_ALIGNED)

#define DEFINE_SILLY_SSE2_VERSIONS(mode, name, mem_mode) \
Processor32 *mode##_##name         = &logic_t_sse2<mem_mode, mode##_t_sse2<nop_sse2, nop_sse2>, mode##_t<nop, nop>>; \
Processor32 *mode##sub_##name      = &logic_t_sse2<mem_mode, mode##_t_sse2<nop_sse2, sub_sse2>, mode##_t<nop, sub>>; \
Processor32 *mode##add_##name      = &logic_t_sse2<mem_mode, mode##_t_sse2<nop_sse2, add_sse2>, mode##_t<nop, add>>; \
Processor32 *sub##mode##_##name    = &logic_t_sse2<mem_mode, mode##_t_sse2<sub_sse2, nop_sse2>, mode##_t<sub, nop>>; \
Processor32 *sub##mode##sub_##name = &logic_t_sse2<mem_mode, mode##_t_sse2<sub_sse2, sub_sse2>, mode##_t<sub, sub>>; \
Processor32 *sub##mode##add_##name = &logic_t_sse2<mem_mode, mode##_t_sse2<sub_sse2, add_sse2>, mode##_t<sub, add>>; \
Processor32 *add##mode##_##name    = &logic_t_sse2<mem_mode, mode##_t_sse2<add_sse2, nop_sse2>, mode##_t<add, nop>>; \
Processor32 *add##mode##sub_##name = &logic_t_sse2<mem_mode, mode##_t_sse2<add_sse2, sub_sse2>, mode##_t<add, sub>>; \
Processor32 *add##mode##add_##name = &logic_t_sse2<mem_mode, mode##_t_sse2<add_sse2, add_sse2>, mode##_t<add, add>>;

DEFINE_SILLY_SSE2_VERSIONS(min, 32_sse2, MemoryMode::SSE2_UNALIGNED)
DEFINE_SILLY_SSE2_VERSIONS(max, 32_sse2, MemoryMode::SSE2_UNALIGNED)
DEFINE_SILLY_SSE2_VERSIONS(min, 32_asse2, MemoryMode::SSE2_ALIGNED)
DEFINE_SILLY_SSE2_VERSIONS(max, 32_asse2, MemoryMode::SSE2_ALIGNED)
#undef DEFINE_SILLY_SSE2_VERSIONS
#undef DEFINE_SSE2_VERSIONS

} } } }

#endif // INTEL_INTRINSICS
