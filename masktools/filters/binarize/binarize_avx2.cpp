#include "binarize.h"

#ifdef INTEL_INTRINSICS

#include "../../common/simd_avx2.h"
#include "binarize_c.h"

namespace {

/* avx2 functions */
static MT_FORCEINLINE __m256i binarize_upper_avx2_op(__m256i x, __m256i t, __m256i) {
    auto r = _mm256_subs_epu8(x, t);
    return _mm256_cmpeq_epi8(r, _mm256_setzero_si256());
}

static MT_FORCEINLINE __m256i binarize_lower_avx2_op(__m256i x, __m256i, __m256i t128) {
    auto r = _mm256_add_epi8(x, _mm256_set1_epi32(0x80808080));
    return _mm256_cmpgt_epi8(r, t128);
}

static MT_FORCEINLINE __m256i binarize_0_x_avx2_op(__m256i x, __m256i t, __m256i t128) {
    auto upper = binarize_upper_avx2_op(x, t, t128);
    return _mm256_and_si256(upper, x);
}

static MT_FORCEINLINE __m256i binarize_t_x_avx2_op(__m256i x, __m256i t, __m256i) {
    return _mm256_min_epu8(t, x);
}

static MT_FORCEINLINE __m256i binarize_x_0_avx2_op(__m256i x, __m256i t, __m256i t128) {
    auto lower = binarize_lower_avx2_op(x, t, t128);
    return _mm256_and_si256(lower, x);
}

static MT_FORCEINLINE __m256i binarize_x_t_avx2_op(__m256i x, __m256i t, __m256i) {
    return _mm256_max_epu8(t, x);
}

static MT_FORCEINLINE __m256i binarize_t_0_avx2_op(__m256i x, __m256i t, __m256i t128) {
    auto lower = binarize_lower_avx2_op(x, t, t128);
    return _mm256_and_si256(lower, t);
}

static MT_FORCEINLINE __m256i binarize_0_t_avx2_op(__m256i x, __m256i t, __m256i t128) {
    auto upper = binarize_upper_avx2_op(x, t, t128);
    return _mm256_and_si256(upper, t);
}

static MT_FORCEINLINE __m256i binarize_x_255_avx2_op(__m256i x, __m256i t, __m256i t128) {
    auto upper = binarize_upper_avx2_op(x, t, t128);
    return _mm256_or_si256(upper, x);
}

static MT_FORCEINLINE __m256i binarize_255_x_avx2_op(__m256i x, __m256i t, __m256i t128) {
    auto lower = binarize_lower_avx2_op(x, t, t128);
    return _mm256_or_si256(lower, x);
}

static MT_FORCEINLINE __m256i binarize_t_255_avx2_op(__m256i x, __m256i t, __m256i t128) {
    auto upper = binarize_upper_avx2_op(x, t, t128);
    return _mm256_or_si256(upper, t);
}

static MT_FORCEINLINE __m256i binarize_255_t_avx2_op(__m256i x, __m256i t, __m256i t128) {
    auto lower = binarize_lower_avx2_op(x, t, t128);
    return _mm256_or_si256(lower, t);
}

template<decltype(binarize_upper_avx2_op) op, Operator op_c>
void binarize_avx2_t(uint8_t *dstp, ptrdiff_t dst_pitch, uint8_t threshold, int width, int height)
{
    auto t = _mm256_set1_epi8(uint8_t(threshold));
    auto t128 = _mm256_add_epi8(t, _mm256_set1_epi32(0x80808080));
    int mod32_width = (width / 32) * 32;
    int mod64_width = (width / 64) * 64;
    auto dstp2 = dstp;

    for ( int j = 0; j < height; ++j ) {
        for (int i = 0; i < mod64_width; i+=64 ) {
            auto src = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(dstp+i));
            auto src2 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(dstp + i + 32));
            auto result = op(src, t, t128);
            auto result2 = op(src2, t, t128);
            _mm256_storeu_si256(reinterpret_cast<__m256i*>(dstp + i), result);
            _mm256_storeu_si256(reinterpret_cast<__m256i*>(dstp+i+32), result2);
        }
        for (int i = mod64_width; i < mod32_width; i += 32) {
          auto src = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(dstp + i));
          auto result = op(src, t, t128);
          _mm256_storeu_si256(reinterpret_cast<__m256i*>(dstp + i), result);
        }
        dstp += dst_pitch;
    }

    if (width > mod32_width) {
        binarize_t<op_c>(dstp2 + mod32_width, dst_pitch, threshold, width - mod32_width, height);
    }
}

} // namespace

namespace Filtering { namespace MaskTools { namespace Filters { namespace Binarize {

#define DEFINE_AVX2_VERSIONS(name) \
    Processor *binarize_upper_##name = &binarize_avx2_t<binarize_upper_avx2_op, binarize_upper>; \
    Processor *binarize_lower_##name = &binarize_avx2_t<binarize_lower_avx2_op, binarize_lower>; \
    Processor *binarize_0_x_##name   = &binarize_avx2_t<binarize_0_x_avx2_op, binarize_0_x>; \
    Processor *binarize_t_x_##name   = &binarize_avx2_t<binarize_t_x_avx2_op, binarize_t_x>; \
    Processor *binarize_x_0_##name   = &binarize_avx2_t<binarize_x_0_avx2_op, binarize_x_0>; \
    Processor *binarize_x_t_##name   = &binarize_avx2_t<binarize_x_t_avx2_op, binarize_x_t>; \
    Processor *binarize_t_0_##name   = &binarize_avx2_t<binarize_t_0_avx2_op, binarize_t_0>; \
    Processor *binarize_0_t_##name   = &binarize_avx2_t<binarize_0_t_avx2_op, binarize_0_t>; \
    Processor *binarize_x_255_##name = &binarize_avx2_t<binarize_x_255_avx2_op, binarize_x_255>; \
    Processor *binarize_t_255_##name = &binarize_avx2_t<binarize_t_255_avx2_op, binarize_t_255>; \
    Processor *binarize_255_x_##name = &binarize_avx2_t<binarize_255_x_avx2_op, binarize_255_x>; \
    Processor *binarize_255_t_##name = &binarize_avx2_t<binarize_255_t_avx2_op, binarize_255_t>;


DEFINE_AVX2_VERSIONS(avx2)

} } } }

#endif // INTEL_INTRINSICS