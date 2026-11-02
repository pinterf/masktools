#include "average.h"

#ifdef INTEL_INTRINSICS

#include "../../../common/simd_avx2.h"

// mt_average AVX2: (dst + src + 1) >> 1 (avg_epu8/epu16), float (dst + src) * 0.5; equal to C.

namespace Filtering { namespace MaskTools { namespace Filters { namespace Support { namespace Average {

namespace {

static void average_avx2_t(uint8_t *pDst, ptrdiff_t dst_pitch, const uint8_t *pSrc, ptrdiff_t src_pitch, int width, int height)
{
  const int mod32_width = (width / 32) * 32;
  auto pDst2 = pDst;
  auto pSrc2 = pSrc;

  for (int j = 0; j < height; ++j) {
    for (int i = 0; i < mod32_width; i += 32) {
      auto dst = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pDst + i));
      auto src = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pSrc + i));
      _mm256_storeu_si256(reinterpret_cast<__m256i*>(pDst + i), _mm256_avg_epu8(dst, src));
    }
    pDst += dst_pitch;
    pSrc += src_pitch;
  }

  if (width > mod32_width)
    average_c(pDst2 + mod32_width, dst_pitch, pSrc2 + mod32_width, src_pitch, width - mod32_width, height);
}

static void average16_avx2_t(uint8_t *pDst, ptrdiff_t dst_pitch, const uint8_t *pSrc, ptrdiff_t src_pitch, int width, int height, int nOrigHeight)
{
  const int rowsize = width * sizeof(uint16_t);
  const int mod32_width = (rowsize / 32) * 32;
  auto pDst2 = pDst;
  auto pSrc2 = pSrc;

  for (int j = 0; j < height; ++j) {
    for (int i = 0; i < mod32_width; i += 32) {
      auto dst = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pDst + i));
      auto src = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pSrc + i));
      _mm256_storeu_si256(reinterpret_cast<__m256i*>(pDst + i), _mm256_avg_epu16(dst, src));
    }
    pDst += dst_pitch;
    pSrc += src_pitch;
  }

  if (rowsize > mod32_width)
    average16_native_c(pDst2 + mod32_width, dst_pitch, pSrc2 + mod32_width, src_pitch, (rowsize - mod32_width) / sizeof(uint16_t), height, nOrigHeight);
}

static void average32_avx2_t(uint8_t *pDst, ptrdiff_t dst_pitch, const uint8_t *pSrc, ptrdiff_t src_pitch, int width, int height)
{
  const int rowsize = width * sizeof(float);
  const int mod32_width = (rowsize / 32) * 32;
  auto pDst2 = pDst;
  auto pSrc2 = pSrc;
  const auto vHalf = _mm256_set1_ps(0.5f);

  for (int j = 0; j < height; ++j) {
    for (int i = 0; i < mod32_width; i += 32) {
      auto dst = _mm256_loadu_ps(reinterpret_cast<const float*>(pDst + i));
      auto src = _mm256_loadu_ps(reinterpret_cast<const float*>(pSrc + i));
      _mm256_storeu_ps(reinterpret_cast<float*>(pDst + i), _mm256_mul_ps(_mm256_add_ps(dst, src), vHalf));
    }
    pDst += dst_pitch;
    pSrc += src_pitch;
  }

  if (rowsize > mod32_width)
    average32_c(pDst2 + mod32_width, dst_pitch, pSrc2 + mod32_width, src_pitch, (rowsize - mod32_width) / sizeof(float), height);
}

} // namespace

Processor *average_avx2 = &average_avx2_t;
Processor16 *average16_native_avx2 = &average16_avx2_t;
Processor32 *average32_avx2 = &average32_avx2_t;

} } } } }

#endif // INTEL_INTRINSICS
