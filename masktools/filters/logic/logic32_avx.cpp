#include "logic.h"

#ifdef INTEL_INTRINSICS

#include "../../common/simd_avx.h"

using namespace Filtering;

namespace {

static MT_FORCEINLINE float add(float a, float b) { return a + b; }
static MT_FORCEINLINE float sub(float a, float b) { return a - b; }
static MT_FORCEINLINE float nop(float a, float b) { UNUSED(b); return a; }

#define CAST_U32(x) (*reinterpret_cast<uint32_t *>(&x))

static MT_FORCEINLINE float cast_to_float(uint32_t x)
{
  uint32_t tmp = x; return *reinterpret_cast<float *>(&tmp);
}
// not much sense on float, but for the sake of generality
static MT_FORCEINLINE float _and(float a, float b, float th1, float th2) { UNUSED(th1); UNUSED(th2); return cast_to_float(CAST_U32(a) & CAST_U32(b)); }
static MT_FORCEINLINE float _or (float a, float b, float th1, float th2) { UNUSED(th1); UNUSED(th2); return cast_to_float(CAST_U32(a) | CAST_U32(b)); }
static MT_FORCEINLINE float andn(float a, float b, float th1, float th2) { UNUSED(th1); UNUSED(th2); return cast_to_float(CAST_U32(a) & ~CAST_U32(b)); }
static MT_FORCEINLINE float _xor(float a, float b, float th1, float th2) { UNUSED(th1); UNUSED(th2); return cast_to_float(CAST_U32(a) ^ CAST_U32(b)); }

template <decltype(add) opa, decltype(add) opb>
static MT_FORCEINLINE float min_t(float a, float b, float th1, float th2) { 
    return min<float>(opa(a, th1), opb(b, th2)); 
}

template <decltype(add) opa, decltype(add) opb>
static MT_FORCEINLINE float max_t(float a, float b, float th1, float th2) { 
    return max<float>(opa(a, th1), opb(b, th2)); 
}

template <decltype(_and) op>
static void logic_t(float *pDst, ptrdiff_t nDstPitch, const float *pSrc, ptrdiff_t nSrcPitch, int nWidth, int nHeight, float nThresholdDestination, float nThresholdSource)
{
   nDstPitch /= sizeof(float);
   nSrcPitch /= sizeof(float);
   for ( int y = 0; y < nHeight; y++ )
   {
      for ( int x = 0; x < nWidth; x++ )
         pDst[x] = op(pDst[x], pSrc[x], nThresholdDestination, nThresholdSource);
      pDst += nDstPitch;
      pSrc += nSrcPitch;
   }
}

/* avx */

static MT_FORCEINLINE __m256 add_avx(__m256 a, __m256 b) { return _mm256_add_ps(a, b); } // no clamp or saturation on float
static MT_FORCEINLINE __m256 sub_avx(__m256 a, __m256 b) { return _mm256_sub_ps(a, b); }
static MT_FORCEINLINE __m256 nop_avx(__m256 a, __m256) { return a; }

static MT_FORCEINLINE __m256 and_avx_op(const __m256 &a, const __m256 &b, const __m256&, const __m256&) { 
    return _mm256_and_ps(a, b); 
}

static MT_FORCEINLINE __m256 or_avx_op(const __m256 &a, const __m256 &b, const __m256&, const __m256&) { 
    return _mm256_or_ps(a, b); 
}

static MT_FORCEINLINE __m256 andn_avx_op(const __m256 &a, const __m256 &b, const __m256&, const __m256&) { 
    return _mm256_andnot_ps(a, b); 
}

static MT_FORCEINLINE __m256 xor_avx_op(const __m256 &a, const __m256 &b, const __m256&, const __m256&) { 
    return _mm256_xor_ps(a, b); 
}

template <decltype(add_avx) opa, decltype(add_avx) opb>
static MT_FORCEINLINE __m256 min_t_avx(const __m256 &a, const __m256 &b, const __m256& th1, const __m256& th2) { 
    return _mm256_min_ps(opa(a, th1), opb(b, th2));
}

template <decltype(add_avx) opa, decltype(add_avx) opb>
static MT_FORCEINLINE __m256 max_t_avx(const __m256 &a, const __m256 &b, const __m256& th1, const __m256& th2) { 
    return _mm256_max_ps(opa(a, th1), opb(b, th2));
}


template<MemoryMode mem_mode, decltype(and_avx_op) op, decltype(_and) op_c>
    void logic_t_avx(float *pDst8, ptrdiff_t nDstPitch, const float *pSrc8, ptrdiff_t nSrcPitch, int nWidth, int nHeight, float nThresholdDestination, float nThresholdSource)
{
    uint8_t *pDst = reinterpret_cast<uint8_t *>(pDst8);
    const uint8_t *pSrc = reinterpret_cast<const uint8_t *>(pSrc8);

    nWidth *= sizeof(float);

    int wMod32 = (nWidth / 32) * 32;
    auto pDst2 = pDst;
    auto pSrc2 = pSrc;
    auto tDest = _mm256_set1_ps(nThresholdDestination);
    auto tSource = _mm256_set1_ps(nThresholdSource);

    for ( int j = 0; j < nHeight; ++j ) {
        for ( int i = 0; i < wMod32; i+=32 ) {
            _mm_prefetch(reinterpret_cast<const char*>(pDst) + i + 384, _MM_HINT_T0);
            _mm_prefetch(reinterpret_cast<const char*>(pSrc) + i + 384, _MM_HINT_T0);
          
            auto dst = simd256_load_ps<mem_mode>(pDst+i);
            auto src = simd256_load_ps<mem_mode>(pSrc+i);

            auto result = op(dst, src, tDest, tSource);

            simd256_store_ps<mem_mode>(pDst+i, result);
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

#define DEFINE_AVX_VERSIONS(name, mem_mode) \
Processor32 *and_##name  = &logic_t_avx<mem_mode, and_avx_op, _and>; \
Processor32 *or_##name   = &logic_t_avx<mem_mode, or_avx_op, _or>; \
Processor32 *andn_##name = &logic_t_avx<mem_mode, andn_avx_op, andn>; \
Processor32 *xor_##name  = &logic_t_avx<mem_mode, xor_avx_op, _xor>;

DEFINE_AVX_VERSIONS(32_avx, MemoryMode::SSE2_UNALIGNED)
DEFINE_AVX_VERSIONS(32_aavx, MemoryMode::SSE2_ALIGNED)

#define DEFINE_SILLY_AVX_VERSIONS(mode, name, mem_mode) \
Processor32 *mode##_##name         = &logic_t_avx<mem_mode, mode##_t_avx<nop_avx, nop_avx>, mode##_t<nop, nop>>;   \
Processor32 *mode##sub_##name      = &logic_t_avx<mem_mode, mode##_t_avx<nop_avx, sub_avx>, mode##_t<nop, sub>>;   \
Processor32 *mode##add_##name      = &logic_t_avx<mem_mode, mode##_t_avx<nop_avx, add_avx>, mode##_t<nop, add>>;   \
Processor32 *sub##mode##_##name    = &logic_t_avx<mem_mode, mode##_t_avx<sub_avx, nop_avx>, mode##_t<sub, nop>>;   \
Processor32 *sub##mode##sub_##name = &logic_t_avx<mem_mode, mode##_t_avx<sub_avx, sub_avx>, mode##_t<sub, sub>>;   \
Processor32 *sub##mode##add_##name = &logic_t_avx<mem_mode, mode##_t_avx<sub_avx, add_avx>, mode##_t<sub, add>>;   \
Processor32 *add##mode##_##name    = &logic_t_avx<mem_mode, mode##_t_avx<add_avx, nop_avx>, mode##_t<add, nop>>;   \
Processor32 *add##mode##sub_##name = &logic_t_avx<mem_mode, mode##_t_avx<add_avx, sub_avx>, mode##_t<add, sub>>;   \
Processor32 *add##mode##add_##name = &logic_t_avx<mem_mode, mode##_t_avx<add_avx, add_avx>, mode##_t<add, add>>;

DEFINE_SILLY_AVX_VERSIONS(min, 32_avx, MemoryMode::SSE2_UNALIGNED)
DEFINE_SILLY_AVX_VERSIONS(max, 32_avx, MemoryMode::SSE2_UNALIGNED)
DEFINE_SILLY_AVX_VERSIONS(min, 32_aavx, MemoryMode::SSE2_ALIGNED)
DEFINE_SILLY_AVX_VERSIONS(max, 32_aavx, MemoryMode::SSE2_ALIGNED)

#undef DEFINE_SILLY_AVX_VERSIONS
#undef DEFINE_AVX_VERSIONS
//#undef DEFINE_C_VERSIONS
} } } }

#endif // INTEL_INTRINSICS