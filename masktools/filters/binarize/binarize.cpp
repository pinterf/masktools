#include "binarize.h"
#include "../../common/simd.h"

using namespace Filtering;

typedef uint8_t (Operator)(uint8_t, uint8_t);

inline uint8_t binarize_upper(uint8_t x, uint8_t t) { return x > t ? 0 : 255; }
inline uint8_t binarize_lower(uint8_t x, uint8_t t) { return x > t ? 255 : 0; }

inline uint8_t binarize_0_x(uint8_t x, uint8_t t) { return x > t ? 0 : x; }
inline uint8_t binarize_t_x(uint8_t x, uint8_t t) { return x > t ? t : x; }
inline uint8_t binarize_x_0(uint8_t x, uint8_t t) { return x > t ? x : 0; }
inline uint8_t binarize_x_t(uint8_t x, uint8_t t) { return x > t ? x : t; }

inline uint8_t binarize_t_0(uint8_t x, uint8_t t) { return x > t ? t : 0; }
inline uint8_t binarize_0_t(uint8_t x, uint8_t t) { return x > t ? 0 : t; }

inline uint8_t binarize_x_255(uint8_t x, uint8_t t) { return x > t ? x : 255; }
inline uint8_t binarize_t_255(uint8_t x, uint8_t t) { return x > t ? t : 255; }
inline uint8_t binarize_255_x(uint8_t x, uint8_t t) { return x > t ? 255 : x; }
inline uint8_t binarize_255_t(uint8_t x, uint8_t t) { return x > t ? 255 : t; }


template <Operator op>
void binarize_t(uint8_t *pDst, ptrdiff_t nDstPitch, uint8_t nThreshold, int nWidth, int nHeight)
{
   for ( int j = 0; j < nHeight; j++, pDst += nDstPitch )
      for ( int i = 0; i < nWidth; i++ )
         pDst[i] = op(pDst[i], nThreshold);
}

/* SSE2 functions */
static MT_FORCEINLINE __m128i binarize_upper_sse2_op(__m128i x, __m128i t, __m128i) {
    auto r = _mm_subs_epu8(x, t);
    return _mm_cmpeq_epi8(r, _mm_setzero_si128());
}

static MT_FORCEINLINE __m128i binarize_lower_sse2_op(__m128i x, __m128i, __m128i t128) {
    auto r = _mm_add_epi8(x, _mm_set1_epi32(0x80808080));
    return _mm_cmpgt_epi8(r, t128);
}

static MT_FORCEINLINE __m128i binarize_0_x_sse2_op(__m128i x, __m128i t, __m128i t128) {
    auto upper = binarize_upper_sse2_op(x, t, t128);
    return _mm_and_si128(upper, x);
}

static MT_FORCEINLINE __m128i binarize_t_x_sse2_op(__m128i x, __m128i t, __m128i) {
    return _mm_min_epu8(t, x);
}

static MT_FORCEINLINE __m128i binarize_x_0_sse2_op(__m128i x, __m128i t, __m128i t128) {
    auto lower = binarize_lower_sse2_op(x, t, t128);
    return _mm_and_si128(lower, x);
}

static MT_FORCEINLINE __m128i binarize_x_t_sse2_op(__m128i x, __m128i t, __m128i) {
    return _mm_max_epu8(t, x);
}

static MT_FORCEINLINE __m128i binarize_t_0_sse2_op(__m128i x, __m128i t, __m128i t128) {
    auto lower = binarize_lower_sse2_op(x, t, t128);
    return _mm_and_si128(lower, t);
}

static MT_FORCEINLINE __m128i binarize_0_t_sse2_op(__m128i x, __m128i t, __m128i t128) {
    auto upper = binarize_upper_sse2_op(x, t, t128);
    return _mm_and_si128(upper, t);
}

static MT_FORCEINLINE __m128i binarize_x_255_sse2_op(__m128i x, __m128i t, __m128i t128) {
    auto upper = binarize_upper_sse2_op(x, t, t128);
    return _mm_or_si128(upper, x);
}

static MT_FORCEINLINE __m128i binarize_255_x_sse2_op(__m128i x, __m128i t, __m128i t128) {
    auto lower = binarize_lower_sse2_op(x, t, t128);
    return _mm_or_si128(lower, x);
}

static MT_FORCEINLINE __m128i binarize_t_255_sse2_op(__m128i x, __m128i t, __m128i t128) {
    auto upper = binarize_upper_sse2_op(x, t, t128);
    return _mm_or_si128(upper, t);
}

static MT_FORCEINLINE __m128i binarize_255_t_sse2_op(__m128i x, __m128i t, __m128i t128) {
    auto lower = binarize_lower_sse2_op(x, t, t128);
    return _mm_or_si128(lower, t);
}

template<MemoryMode mem_mode, decltype(binarize_upper_sse2_op) op, decltype(binarize_upper) op_c>
void binarize_sse2_t(uint8_t *dstp, ptrdiff_t dst_pitch, uint8_t threshold, int width, int height)
{
    auto t = _mm_set1_epi8(uint8_t(threshold));
    auto t128 = _mm_add_epi8(t, _mm_set1_epi32(0x80808080));
    int mod32_width = (width / 32) * 32;
    auto dstp2 = dstp;

    for ( int j = 0; j < height; ++j ) {
        for ( int i = 0; i < mod32_width; i+=32 ) {
            _mm_prefetch(reinterpret_cast<const char*>(dstp)+i+320, _MM_HINT_T0);
            auto src = simd_load_si128<mem_mode>(dstp+i);
            auto src2 = simd_load_si128<mem_mode>(dstp+i+16);
            auto result = op(src, t, t128);
            auto result2 = op(src2, t, t128);
            simd_store_si128<mem_mode>(dstp+i, result);
            simd_store_si128<mem_mode>(dstp+i+16, result2);
        }
        dstp += dst_pitch;
    }

    if (width > mod32_width) {
        binarize_t<op_c>(dstp2 + mod32_width, dst_pitch, threshold, width - mod32_width, height);
    }
}

namespace Filtering { namespace MaskTools { namespace Filters { namespace Binarize {

Processor *binarize_upper_c = &binarize_t<binarize_upper>;
Processor *binarize_lower_c = &binarize_t<binarize_lower>;
Processor *binarize_0_x_c   = &binarize_t<binarize_0_x>;
Processor *binarize_t_x_c   = &binarize_t<binarize_t_x>;
Processor *binarize_x_0_c   = &binarize_t<binarize_x_0>;
Processor *binarize_x_t_c   = &binarize_t<binarize_x_t>;
Processor *binarize_t_0_c   = &binarize_t<binarize_t_0>;
Processor *binarize_0_t_c   = &binarize_t<binarize_0_t>;
Processor *binarize_x_255_c = &binarize_t<binarize_x_255>;
Processor *binarize_t_255_c = &binarize_t<binarize_t_255>;
Processor *binarize_255_x_c = &binarize_t<binarize_255_x>;
Processor *binarize_255_t_c = &binarize_t<binarize_255_t>;

#define DEFINE_SSE2_VERSIONS(name, mem_mode) \
    Processor *binarize_upper_##name = &binarize_sse2_t<mem_mode, binarize_upper_sse2_op, binarize_upper>; \
    Processor *binarize_lower_##name = &binarize_sse2_t<mem_mode, binarize_lower_sse2_op, binarize_lower>; \
    Processor *binarize_0_x_##name   = &binarize_sse2_t<mem_mode, binarize_0_x_sse2_op, binarize_0_x>; \
    Processor *binarize_t_x_##name   = &binarize_sse2_t<mem_mode, binarize_t_x_sse2_op, binarize_t_x>; \
    Processor *binarize_x_0_##name   = &binarize_sse2_t<mem_mode, binarize_x_0_sse2_op, binarize_x_0>; \
    Processor *binarize_x_t_##name   = &binarize_sse2_t<mem_mode, binarize_x_t_sse2_op, binarize_x_t>; \
    Processor *binarize_t_0_##name   = &binarize_sse2_t<mem_mode, binarize_t_0_sse2_op, binarize_t_0>; \
    Processor *binarize_0_t_##name   = &binarize_sse2_t<mem_mode, binarize_0_t_sse2_op, binarize_0_t>; \
    Processor *binarize_x_255_##name = &binarize_sse2_t<mem_mode, binarize_x_255_sse2_op, binarize_x_255>; \
    Processor *binarize_t_255_##name = &binarize_sse2_t<mem_mode, binarize_t_255_sse2_op, binarize_t_255>; \
    Processor *binarize_255_x_##name = &binarize_sse2_t<mem_mode, binarize_255_x_sse2_op, binarize_255_x>; \
    Processor *binarize_255_t_##name = &binarize_sse2_t<mem_mode, binarize_255_t_sse2_op, binarize_255_t>;


DEFINE_SSE2_VERSIONS(sse2, MemoryMode::SSE2_UNALIGNED)
DEFINE_SSE2_VERSIONS(asse2, MemoryMode::SSE2_ALIGNED)




} } } }