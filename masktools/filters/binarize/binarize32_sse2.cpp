#include "binarize.h"

#ifdef INTEL_INTRINSICS

#include "../../common/simd.h"
#include "binarize32_c.h"

namespace {

/* SSE2 functions */
// core: returns FFFFFFFF or 000000000 masks
static MT_FORCEINLINE __m128 binarize32_upper_sse2_op(__m128 x, __m128 t) {
    // x > t ? 0 : 1.0f;
    // x <= t > 1.0f : 0 // lte = ngt : not greater than
    return _mm_cmpngt_ps(x, t); // (a0 > b0) ? 00000000 : FFFFFFFF
}

// core: returns FFFFFFFF or 000000000 masks
static MT_FORCEINLINE __m128 binarize32_lower_sse2_op(__m128 x, __m128 t) {
    // x > t ? 1.0f : 0;
    return _mm_cmpgt_ps(x, t); // (a0 > b0) ? FFFFFFFF : 00000000
}

// returns 1.0 or 0.0 masks
static MT_FORCEINLINE __m128 binarize32_upper_0_1_sse2_op(__m128 x, __m128 t, __m128 tOne) {
  // x > t ? 0 : 1.0f;
  auto upper = binarize32_upper_sse2_op(x, t);
  return _mm_and_ps(upper, tOne);
}

// returns 1.0 or 0.0 masks
static MT_FORCEINLINE __m128 binarize32_lower_0_1_sse2_op(__m128 x, __m128 t, __m128 tOne) {
  // x > t ? 1.0f : 0;
  auto lower = binarize32_lower_sse2_op(x, t);
  return _mm_and_ps(lower, tOne);
}

static MT_FORCEINLINE __m128 binarize32_0_x_sse2_op(__m128 x, __m128 t, __m128) {
    // x > t ? 0 : x
    auto upper = binarize32_upper_sse2_op(x, t);
    return _mm_and_ps(upper, x);
}

static MT_FORCEINLINE __m128 binarize32_t_x_sse2_op(__m128 x, __m128 t, __m128) {
    // return x > t ? t : x;
    return _mm_min_ps(t, x);
}

static MT_FORCEINLINE __m128 binarize32_x_0_sse2_op(__m128 x, __m128 t, __m128) {
    // x > t ? x : 0;
    auto lower = binarize32_lower_sse2_op(x, t);
    return _mm_and_ps(lower, x);
}

static MT_FORCEINLINE __m128 binarize32_x_t_sse2_op(__m128 x, __m128 t, __m128) {
    // x > t ? x : t;
    return _mm_max_ps(t, x);
}

static MT_FORCEINLINE __m128 binarize32_t_0_sse2_op(__m128 x, __m128 t, __m128) {
    // x > t ? t : 0;
    auto lower = binarize32_lower_sse2_op(x, t);
    return _mm_and_ps(lower, t);
}

static MT_FORCEINLINE __m128 binarize32_0_t_sse2_op(__m128 x, __m128 t, __m128) {
    // return x > t ? 0 : t; 
    auto upper = binarize32_upper_sse2_op(x, t);
    return _mm_and_ps(upper, t);
}

static MT_FORCEINLINE __m128 binarize32_x_255_sse2_op(__m128 x, __m128 t, __m128 tOne) {
    // return x > t ? x : 1.0f;
    auto upper = binarize32_upper_sse2_op(x, t);
    // where zero -> x
    // where FFFF -> 1.0
    auto resOnes = _mm_and_ps(upper, tOne);
    auto invMask = _mm_cmpeq_ps(upper, _mm_setzero_ps()); // 00000000 -> FFFFFFFF, !=0 -> 00000000
    auto resX = _mm_and_ps(invMask, x);
    return _mm_or_ps(resOnes, resX);
}

static MT_FORCEINLINE __m128 binarize32_255_x_sse2_op(__m128 x, __m128 t, __m128 tOne) {
    // return x > t ? 1.0f : x; 
    auto lower = binarize32_lower_sse2_op(x, t);
    // where zero -> 1.0
    // where FFFF -> x
    auto resX = _mm_and_ps(lower, tOne);
    auto invMask = _mm_cmpeq_ps(lower, _mm_setzero_ps()); // 00000000 -> FFFFFFFF, !=0 -> 00000000
    auto resOnes = _mm_and_ps(invMask, x);
    return _mm_or_ps(resOnes, resX);
}

static MT_FORCEINLINE __m128 binarize32_t_255_sse2_op(__m128 x, __m128 t, __m128 tOne) {
    //  return x > t ? t : 1.0f; 
  auto upper = binarize32_upper_sse2_op(x, t);
  // where zero -> t
  // where FFFF -> 1.0
  auto resOnes = _mm_and_ps(upper, tOne);
  auto invMask = _mm_cmpeq_ps(upper, _mm_setzero_ps()); // 00000000 -> FFFFFFFF, !=0 -> 00000000
  auto resT = _mm_and_ps(invMask, t);
  return _mm_or_ps(resOnes, resT);
}

static MT_FORCEINLINE __m128 binarize32_255_t_sse2_op(__m128 x, __m128 t, __m128 tOne) {
    // return x > t ? 1.0f : t;
  auto lower = binarize32_lower_sse2_op(x, t);
  // where zero -> 1.0
  // where FFFF -> x
  auto resX = _mm_and_ps(lower, tOne);
  auto invMask = _mm_cmpeq_ps(lower, _mm_setzero_ps()); // 00000000 -> FFFFFFFF, !=0 -> 00000000
  auto resOnes = _mm_and_ps(invMask, t);
  return _mm_or_ps(resOnes, resX);
}

template<decltype(binarize32_upper_0_1_sse2_op) op, decltype(binarize32_upper) op_c>
void binarize32_sse2_t(uint8_t *dstp, ptrdiff_t dst_pitch, float threshold, int width, int height)
{
    width *= sizeof(float);
    
    auto t = _mm_set1_ps(threshold);
    auto tOne = _mm_set1_ps(1.0f);
    int mod32_width = (width / 32) * 32;
    auto dstp2 = dstp;

    for ( int j = 0; j < height; ++j ) {
        for ( int i = 0; i < mod32_width; i+=32 ) {
            _mm_prefetch(reinterpret_cast<const char*>(dstp)+i+320, _MM_HINT_T0);
            auto src = _mm_loadu_ps(reinterpret_cast<const float*>(dstp+i));
            auto src2 = _mm_loadu_ps(reinterpret_cast<const float*>(dstp+i+16));
            auto result = op(src, t, tOne);
            auto result2 = op(src2, t, tOne);
            _mm_storeu_ps(reinterpret_cast<float*>(dstp+i), result);
            _mm_storeu_ps(reinterpret_cast<float*>(dstp+i+16), result2);
        }
        dstp += dst_pitch;
    }

    if (width > mod32_width) {
        binarize32_t<op_c>(dstp2 + mod32_width, dst_pitch, threshold, (width - mod32_width) / sizeof(float), height);
    }
}

} // namespace

namespace Filtering { namespace MaskTools { namespace Filters { namespace Binarize {

#define DEFINE_SSE2_VERSIONS(name) \
    Processor32 *binarize32_upper_##name = &binarize32_sse2_t<binarize32_upper_0_1_sse2_op, binarize32_upper>; \
    Processor32 *binarize32_lower_##name = &binarize32_sse2_t<binarize32_lower_0_1_sse2_op, binarize32_lower>; \
    Processor32 *binarize32_0_x_##name   = &binarize32_sse2_t<binarize32_0_x_sse2_op, binarize32_0_x>; \
    Processor32 *binarize32_t_x_##name   = &binarize32_sse2_t<binarize32_t_x_sse2_op, binarize32_t_x>; \
    Processor32 *binarize32_x_0_##name   = &binarize32_sse2_t<binarize32_x_0_sse2_op, binarize32_x_0>; \
    Processor32 *binarize32_x_t_##name   = &binarize32_sse2_t<binarize32_x_t_sse2_op, binarize32_x_t>; \
    Processor32 *binarize32_t_0_##name   = &binarize32_sse2_t<binarize32_t_0_sse2_op, binarize32_t_0>; \
    Processor32 *binarize32_0_t_##name   = &binarize32_sse2_t<binarize32_0_t_sse2_op, binarize32_0_t>; \
    Processor32 *binarize32_x_255_##name = &binarize32_sse2_t<binarize32_x_255_sse2_op, binarize32_x_255>; \
    Processor32 *binarize32_t_255_##name = &binarize32_sse2_t<binarize32_t_255_sse2_op, binarize32_t_255>; \
    Processor32 *binarize32_255_x_##name = &binarize32_sse2_t<binarize32_255_x_sse2_op, binarize32_255_x>; \
    Processor32 *binarize32_255_t_##name = &binarize32_sse2_t<binarize32_255_t_sse2_op, binarize32_255_t>;

DEFINE_SSE2_VERSIONS(sse2)

} } } }

#endif // INTEL_INTRINSICS
