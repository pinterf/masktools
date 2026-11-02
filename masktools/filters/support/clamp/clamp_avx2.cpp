#include "clamp.h"

#ifdef INTEL_INTRINSICS

#include "../../../common/simd_avx2.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Support  { namespace Clamp {

namespace {

static void clamp_avx2_t(uint8_t *pDst, ptrdiff_t dst_pitch, const uint8_t *pSrc1, ptrdiff_t src1_pitch, const uint8_t *pSrc2, ptrdiff_t src2_pitch, int width, int height, int overshoot, int undershoot)
{
    int mod32_width = (width / 32) * 32;
    auto pDst_s = pDst;
    auto pSrc1_s = pSrc1;
    auto pSrc2_s = pSrc2;

    auto overshoot_v = _mm256_set1_epi8(uint8_t(overshoot));
    auto undershoot_v = _mm256_set1_epi8(uint8_t(undershoot));

    for ( int j = 0; j < height; ++j ) {
        for ( int i = 0; i < mod32_width; i+=32 ) {
            auto upper_limit = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pSrc1+i));
            auto lower_limit = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pSrc2+i));

            upper_limit = _mm256_adds_epu8(upper_limit, overshoot_v);
            lower_limit = _mm256_subs_epu8(lower_limit, undershoot_v);

            auto limited = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pDst+i));

            limited = _mm256_min_epu8(limited, upper_limit);
            limited = _mm256_max_epu8(limited, lower_limit);

            _mm256_storeu_si256(reinterpret_cast<__m256i*>(pDst+i), limited);
        }
        pDst += dst_pitch;
        pSrc1 += src1_pitch;
        pSrc2 += src2_pitch;
    }

    if (width > mod32_width) {
        clamp_c(pDst_s + mod32_width, dst_pitch, pSrc1_s + mod32_width, src1_pitch, pSrc2_s + mod32_width, src2_pitch, width - mod32_width, height, overshoot, undershoot);
    }
}

static void clamp32_avx2_t(uint8_t *pDst, ptrdiff_t dst_pitch, const uint8_t *pSrc1, ptrdiff_t src1_pitch, const uint8_t *pSrc2, ptrdiff_t src2_pitch, int width, int height, float overshoot, float undershoot)
{
  width *= sizeof(float);
  int mod32_width = (width / 32) * 32;
  auto pDst_s = pDst;
  auto pSrc1_s = pSrc1;
  auto pSrc2_s = pSrc2;

  auto overshoot_v = _mm256_set1_ps(overshoot);
  auto undershoot_v = _mm256_set1_ps(undershoot);

  for (int j = 0; j < height; ++j) {
    for (int i = 0; i < mod32_width; i += 32) {
      auto upper_limit = _mm256_loadu_ps(reinterpret_cast<const float*>(pSrc1 + i));
      auto lower_limit = _mm256_loadu_ps(reinterpret_cast<const float*>(pSrc2 + i));

      upper_limit = _mm256_add_ps(upper_limit, overshoot_v);  // no clamp or saturation
      lower_limit = _mm256_sub_ps(lower_limit, undershoot_v);

      auto limited = _mm256_loadu_ps(reinterpret_cast<const float*>(pDst + i));

      // same operand order as SSE2 (NaN handling)
      limited = _mm256_min_ps(limited, upper_limit);
      limited = _mm256_max_ps(limited, lower_limit);

      _mm256_storeu_ps(reinterpret_cast<float*>(pDst + i), limited);
    }
    pDst += dst_pitch;
    pSrc1 += src1_pitch;
    pSrc2 += src2_pitch;
  }

  if (width > mod32_width) {
    clamp32_c(pDst_s + mod32_width, dst_pitch, pSrc1_s + mod32_width, src1_pitch, pSrc2_s + mod32_width, src2_pitch, (width - mod32_width) / sizeof(float), height, overshoot, undershoot);
  }
}

} // namespace

Processor *clamp_avx2 = &clamp_avx2_t;

Processor32 *clamp32_avx2 = &clamp32_avx2_t;

} } } } }

#endif // INTEL_INTRINSICS
