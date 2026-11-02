#include "binarize.h"

#ifdef INTEL_INTRINSICS

#include "../../common/simd_avx2.h"
#include "binarize32_c.h"

namespace {

// masks as in SSE2 (and C, NaN included): upper = !(x > t), lower = x > t
static MT_FORCEINLINE __m256 binarize32_upper_mask_avx2(__m256 x, __m256 t) { return _mm256_cmp_ps(x, t, _CMP_NGT_UQ); }
static MT_FORCEINLINE __m256 binarize32_lower_mask_avx2(__m256 x, __m256 t) { return _mm256_cmp_ps(x, t, _CMP_GT_OS); }

static MT_FORCEINLINE __m256 binarize32_upper_avx2_op(__m256 x, __m256 t, __m256 one) { return _mm256_and_ps(binarize32_upper_mask_avx2(x, t), one); }
static MT_FORCEINLINE __m256 binarize32_lower_avx2_op(__m256 x, __m256 t, __m256 one) { return _mm256_and_ps(binarize32_lower_mask_avx2(x, t), one); }
static MT_FORCEINLINE __m256 binarize32_0_x_avx2_op(__m256 x, __m256 t, __m256) { return _mm256_and_ps(binarize32_upper_mask_avx2(x, t), x); }
static MT_FORCEINLINE __m256 binarize32_t_x_avx2_op(__m256 x, __m256 t, __m256) { return _mm256_min_ps(t, x); }
static MT_FORCEINLINE __m256 binarize32_x_0_avx2_op(__m256 x, __m256 t, __m256) { return _mm256_and_ps(binarize32_lower_mask_avx2(x, t), x); }
static MT_FORCEINLINE __m256 binarize32_x_t_avx2_op(__m256 x, __m256 t, __m256) { return _mm256_max_ps(t, x); }
static MT_FORCEINLINE __m256 binarize32_t_0_avx2_op(__m256 x, __m256 t, __m256) { return _mm256_and_ps(binarize32_lower_mask_avx2(x, t), t); }
static MT_FORCEINLINE __m256 binarize32_0_t_avx2_op(__m256 x, __m256 t, __m256) { return _mm256_and_ps(binarize32_upper_mask_avx2(x, t), t); }
static MT_FORCEINLINE __m256 binarize32_x_255_avx2_op(__m256 x, __m256 t, __m256 one) { return _mm256_blendv_ps(one, x, binarize32_lower_mask_avx2(x, t)); }
static MT_FORCEINLINE __m256 binarize32_t_255_avx2_op(__m256 x, __m256 t, __m256 one) { return _mm256_blendv_ps(one, t, binarize32_lower_mask_avx2(x, t)); }
static MT_FORCEINLINE __m256 binarize32_255_x_avx2_op(__m256 x, __m256 t, __m256 one) { return _mm256_blendv_ps(x, one, binarize32_lower_mask_avx2(x, t)); }
static MT_FORCEINLINE __m256 binarize32_255_t_avx2_op(__m256 x, __m256 t, __m256 one) { return _mm256_blendv_ps(t, one, binarize32_lower_mask_avx2(x, t)); }

template<decltype(binarize32_upper_avx2_op) op, Operator32 op_c>
void binarize32_avx2_t(uint8_t *dstp, ptrdiff_t dst_pitch, float threshold, int width, int height)
{
    const int mod8_width = (width / 8) * 8; // pixels
    auto t = _mm256_set1_ps(threshold);
    auto one = _mm256_set1_ps(1.0f);
    auto dstp2 = dstp;

    for (int j = 0; j < height; ++j) {
        auto p = reinterpret_cast<float *>(dstp);
        for (int i = 0; i < mod8_width; i += 8)
            _mm256_storeu_ps(p + i, op(_mm256_loadu_ps(p + i), t, one));
        dstp += dst_pitch;
    }

    if (width > mod8_width)
        binarize32_t<op_c>(dstp2 + mod8_width * sizeof(float), dst_pitch, threshold, width - mod8_width, height);
}

} // namespace

namespace Filtering { namespace MaskTools { namespace Filters { namespace Binarize {

#define DEFINE_PROCESSORS(name) \
Processor32 *binarize32_##name##_avx2 = &binarize32_avx2_t<binarize32_##name##_avx2_op, binarize32_##name>;

DEFINE_PROCESSORS(upper)
DEFINE_PROCESSORS(lower)
DEFINE_PROCESSORS(0_x)
DEFINE_PROCESSORS(t_x)
DEFINE_PROCESSORS(x_0)
DEFINE_PROCESSORS(x_t)
DEFINE_PROCESSORS(t_0)
DEFINE_PROCESSORS(0_t)
DEFINE_PROCESSORS(x_255)
DEFINE_PROCESSORS(t_255)
DEFINE_PROCESSORS(255_x)
DEFINE_PROCESSORS(255_t)
#undef DEFINE_PROCESSORS

} } } }

#endif // INTEL_INTRINSICS
