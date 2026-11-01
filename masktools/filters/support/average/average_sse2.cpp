#include "average.h"

#ifdef INTEL_INTRINSICS

#include "../../../common/simd.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Support  { namespace Average {

namespace {

static void average_sse2_t(uint8_t *pDst, ptrdiff_t dst_pitch, const uint8_t *pSrc, ptrdiff_t src_pitch, int width, int height)
{
    int mod16_width = (width / 16) * 16;
    auto pDst2 = pDst;
    auto pSrc2 = pSrc;

    for ( int j = 0; j < height; ++j ) {
        for ( int i = 0; i < mod16_width; i+=16 ) {
            _mm_prefetch(reinterpret_cast<const char*>(pDst)+i+256, _MM_HINT_T0);
            _mm_prefetch(reinterpret_cast<const char*>(pSrc)+i+256, _MM_HINT_T0);

            auto dst = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pDst+i));
            auto src = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pSrc+i));

            auto result  = _mm_avg_epu8(dst, src);

            _mm_storeu_si128(reinterpret_cast<__m128i*>(pDst+i), result);
        }
        pDst += dst_pitch;
        pSrc += src_pitch;
    }

    if (width > mod16_width) {
        average_c(pDst2 + mod16_width, dst_pitch, pSrc2 + mod16_width, src_pitch, width - mod16_width, height);
    }
}

static void average32_sse2_t(uint8_t *pDst, ptrdiff_t dst_pitch, const uint8_t *pSrc, ptrdiff_t src_pitch, int width, int height)
{
  width *= sizeof(float);

  int mod16_width = (width / 16) * 16;
  auto pDst2 = pDst;
  auto pSrc2 = pSrc;
  auto vDiv2 = _mm_set1_ps(0.5f);

  for (int j = 0; j < height; ++j) {
    for (int i = 0; i < mod16_width; i += 16) {
      _mm_prefetch(reinterpret_cast<const char*>(pDst) + i + 256, _MM_HINT_T0);
      _mm_prefetch(reinterpret_cast<const char*>(pSrc) + i + 256, _MM_HINT_T0);

      auto dst = _mm_loadu_ps(reinterpret_cast<const float*>(pDst + i));
      auto src = _mm_loadu_ps(reinterpret_cast<const float*>(pSrc + i));

      auto result = _mm_mul_ps(_mm_add_ps(dst, src), vDiv2);

      _mm_storeu_ps(reinterpret_cast<float*>(pDst + i), result);
    }
    pDst += dst_pitch;
    pSrc += src_pitch;
  }

  if (width > mod16_width) {
    average32_c(pDst2 + mod16_width, dst_pitch, pSrc2 + mod16_width, src_pitch, (width - mod16_width) / sizeof(float), height);
  }
}

} // namespace

Processor *average_sse2 = &average_sse2_t;

Processor32 *average32_sse2 = &average32_sse2_t;

} } } } }

#endif // INTEL_INTRINSICS
