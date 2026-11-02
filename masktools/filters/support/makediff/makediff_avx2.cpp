#include "makediff.h"

#ifdef INTEL_INTRINSICS

#include "../../../common/simd_avx2.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Support  { namespace MakeDiff {

namespace {

static void makediff_avx2_t(uint8_t *pDst, ptrdiff_t dst_pitch, const uint8_t *pSrc, ptrdiff_t src_pitch, int width, int height)
{
    int mod32_width = (width / 32) * 32;
    auto pDst2 = pDst;
    auto pSrc2 = pSrc;
    auto v128 = _mm256_set1_epi8(int8_t(0x80));

    for ( int j = 0; j < height; ++j ) {
        for ( int i = 0; i < mod32_width; i+=32 ) {
            auto dst = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pDst+i));
            auto src = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pSrc+i));

            auto dstsub = _mm256_sub_epi8(dst, v128);
            auto srcsub = _mm256_sub_epi8(src, v128);
            auto result = _mm256_add_epi8(_mm256_subs_epi8(dstsub, srcsub), v128);

            _mm256_storeu_si256(reinterpret_cast<__m256i*>(pDst+i), result);
        }
        pDst += dst_pitch;
        pSrc += src_pitch;
    }

    if (width > mod32_width) {
        makediff_c(pDst2 + mod32_width, dst_pitch, pSrc2 + mod32_width, src_pitch, width - mod32_width, height);
    }
}

static void makediff32_avx2_t(uint8_t *pDst, ptrdiff_t dst_pitch, const uint8_t *pSrc, ptrdiff_t src_pitch, int width, int height, bool chroma)
{
  width *= sizeof(float);

  int mod32_width = (width / 32) * 32;
  auto pDst2 = pDst;
  auto pSrc2 = pSrc;
#ifdef FLOAT_CHROMA_IS_HALF_CENTERED
  const auto vHalf = _mm256_set1_ps(0.5f);  // also for chroma
#else
  const auto vHalf = _mm256_set1_ps(chroma ? 0.0f : 0.5f);
#endif

  for (int j = 0; j < height; ++j) {
    for (int i = 0; i < mod32_width; i += 32) {
      auto dst = _mm256_loadu_ps(reinterpret_cast<const float*>(pDst + i));
      auto src = _mm256_loadu_ps(reinterpret_cast<const float*>(pSrc + i));

      auto result = _mm256_add_ps(_mm256_sub_ps(dst, src), vHalf); // to work like "x y - 128 +"

      _mm256_storeu_ps(reinterpret_cast<float*>(pDst + i), result);
    }
    pDst += dst_pitch;
    pSrc += src_pitch;
  }

  if (width > mod32_width) {
    makediff32_c(pDst2 + mod32_width, dst_pitch, pSrc2 + mod32_width, src_pitch, (width - mod32_width) / sizeof(float), height, chroma);
  }
}

} // namespace

Processor *makediff_avx2 = &makediff_avx2_t;

Processor32 *makediff32_avx2 = &makediff32_avx2_t;

} } } } }

#endif // INTEL_INTRINSICS
