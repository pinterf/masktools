#include "motionmask.h"

#ifdef INTEL_INTRINSICS

#include "../../../common/simd_avx2.h"
#include "motionmask_c.h"
#include "motionmask16_c.h"
#include "motionmask32_c.h"

// mt_motion AVX2: the sum of absolute differences (scene change detection) and the thresholded absolute
// difference, 8, 10-16 bit and float. Integer: equal to C; float: the mask equals C, the SAD sums in 8 lanes.

namespace Filtering { namespace MaskTools { namespace Filters { namespace Mask { namespace Motion {

namespace {

static MT_FORCEINLINE uint32_t hsum_epi32_avx2(__m256i v)
{
  auto s = _mm_add_epi32(_mm256_castsi256_si128(v), _mm256_extracti128_si256(v, 1));
  s = _mm_add_epi32(s, _mm_shuffle_epi32(s, _MM_SHUFFLE(1, 0, 3, 2)));
  s = _mm_add_epi32(s, _mm_shuffle_epi32(s, _MM_SHUFFLE(2, 3, 0, 1)));
  return uint32_t(_mm_cvtsi128_si32(s));
}

static uint64_t sad_avx2_op(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int nWidth, int nHeight)
{
  uint64_t sad = 0;
  const int wMod32 = (nWidth / 32) * 32;
  for (int y = 0; y < nHeight; ++y) {
    auto acc = _mm256_setzero_si256();
    for (int x = 0; x < wMod32; x += 32) {
      auto dst = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pDst + x));
      auto src = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pSrc + x));
      acc = _mm256_add_epi32(acc, _mm256_sad_epu8(dst, src)); // 4 x 64 bit, each below 2^32 within a row
    }
    unsigned int rowsum = hsum_epi32_avx2(acc);
    for (int x = wMod32; x < nWidth; x++)
      rowsum += abs(pDst[x] - pSrc[x]);
    sad += rowsum;
    pDst += nDstPitch;
    pSrc += nSrcPitch;
  }
  return sad;
}

static void mask_avx2_op(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int nLowThreshold, int nHighThreshold, int nWidth, int nHeight)
{
  const int wMod32 = (nWidth / 32) * 32;
  auto pDst2 = pDst;
  auto pSrc2 = pSrc;

  auto v128 = _mm256_set1_epi8(int8_t(0x80));
  auto lowThresh = _mm256_sub_epi8(_mm256_set1_epi8(int8_t(nLowThreshold)), v128);
  auto highThresh = _mm256_sub_epi8(_mm256_set1_epi8(int8_t(nHighThreshold)), v128);

  for (int y = 0; y < nHeight; ++y) {
    for (int x = 0; x < wMod32; x += 32) {
      auto dst = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pDst + x));
      auto src = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pSrc + x));
      auto diff = _mm256_or_si256(_mm256_subs_epu8(dst, src), _mm256_subs_epu8(src, dst));
      _mm256_storeu_si256(reinterpret_cast<__m256i*>(pDst + x), threshold_avx2(diff, lowThresh, highThresh, v128));
    }
    pDst += nDstPitch;
    pSrc += nSrcPitch;
  }

  if (nWidth > wMod32)
    mask_c_op(pDst2 + wMod32, nDstPitch, pSrc2 + wMod32, nSrcPitch, nLowThreshold, nHighThreshold, nWidth - wMod32, nHeight);
}

static uint64_t sad16_avx2_op(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int nWidth, int nHeight)
{
  uint64_t sad = 0;
  const int wMod16 = (nWidth / 16) * 16;
  const auto zero = _mm256_setzero_si256();
  for (int y = 0; y < nHeight; ++y) {
    const uint16_t *d = reinterpret_cast<const uint16_t *>(pDst);
    const uint16_t *s = reinterpret_cast<const uint16_t *>(pSrc);
    auto acc = zero;
    for (int x = 0; x < wMod16; x += 16) {
      auto dst = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(d + x));
      auto src = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(s + x));
      auto absdiff = _mm256_or_si256(_mm256_subs_epu16(dst, src), _mm256_subs_epu16(src, dst));
      acc = _mm256_add_epi32(acc, _mm256_unpacklo_epi16(absdiff, zero));
      acc = _mm256_add_epi32(acc, _mm256_unpackhi_epi16(absdiff, zero));
    }
    uint32_t rowsum = hsum_epi32_avx2(acc);
    for (int x = wMod16; x < nWidth; x++)
      rowsum += abs(d[x] - s[x]);
    sad += rowsum;
    pDst += nDstPitch;
    pSrc += nSrcPitch;
  }
  return sad;
}

// x <= lo ? 0 : x > hi ? max : x; the thresholds are 0..max (ScaleParam)
static void mask16_avx2_op(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int nLowThreshold, int nHighThreshold, int nWidth, int nHeight, int bits_per_pixel)
{
  const int wMod16 = (nWidth / 16) * 16;
  auto lowThresh = _mm256_set1_epi16(int16_t(nLowThreshold ^ 0x8000));
  auto highThresh = _mm256_set1_epi16(int16_t(nHighThreshold ^ 0x8000));
  auto vMax = _mm256_set1_epi16(int16_t((1 << bits_per_pixel) - 1));
  auto pDst2 = pDst;
  auto pSrc2 = pSrc;

  for (int y = 0; y < nHeight; ++y) {
    uint16_t *d = reinterpret_cast<uint16_t *>(pDst);
    const uint16_t *s = reinterpret_cast<const uint16_t *>(pSrc);
    for (int x = 0; x < wMod16; x += 16) {
      auto dst = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(d + x));
      auto src = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(s + x));
      auto diff = _mm256_or_si256(_mm256_subs_epu16(dst, src), _mm256_subs_epu16(src, dst));
      _mm256_storeu_si256(reinterpret_cast<__m256i*>(d + x), threshold16_avx2(diff, lowThresh, highThresh, vMax));
    }
    pDst += nDstPitch;
    pSrc += nSrcPitch;
  }

  if (nWidth > wMod16)
    mask16_c_op(pDst2 + wMod16 * sizeof(uint16_t), nDstPitch, pSrc2 + wMod16 * sizeof(uint16_t), nSrcPitch, nLowThreshold, nHighThreshold, nWidth - wMod16, nHeight, bits_per_pixel);
}

static double sad32_avx2_op(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int nWidth, int nHeight)
{
  double sad = 0;
  const int wMod8 = (nWidth / 8) * 8;
  const auto absmask = _mm256_castsi256_ps(_mm256_set1_epi32(0x7FFFFFFF));
  for (int y = 0; y < nHeight; ++y) {
    const float *d = reinterpret_cast<const float *>(pDst);
    const float *s = reinterpret_cast<const float *>(pSrc);
    auto acc = _mm256_setzero_ps();
    for (int x = 0; x < wMod8; x += 8)
      acc = _mm256_add_ps(acc, _mm256_and_ps(_mm256_sub_ps(_mm256_loadu_ps(d + x), _mm256_loadu_ps(s + x)), absmask));
    auto s4 = _mm_add_ps(_mm256_castps256_ps128(acc), _mm256_extractf128_ps(acc, 1));
    s4 = _mm_add_ps(s4, _mm_movehl_ps(s4, s4));
    s4 = _mm_add_ss(s4, _mm_shuffle_ps(s4, s4, _MM_SHUFFLE(1, 1, 1, 1)));
    float rowsum = _mm_cvtss_f32(s4);
    for (int x = wMod8; x < nWidth; x++)
      rowsum += abs(d[x] - s[x]);
    sad += rowsum;
    pDst += nDstPitch;
    pSrc += nSrcPitch;
  }
  return sad;
}

// threshold<float>(abs(d - s)): x <= lo ? 0 : x > hi ? 1 : x, abs as the C x < 0 ? -x : x
static void mask32_avx2_op(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, float nLowThreshold, float nHighThreshold, int nWidth, int nHeight)
{
  const int wMod8 = (nWidth / 8) * 8;
  const auto lo = _mm256_set1_ps(nLowThreshold);
  const auto hi = _mm256_set1_ps(nHighThreshold);
  const auto one = _mm256_set1_ps(1.0f);
  const auto zero = _mm256_setzero_ps();
  const auto sign = _mm256_castsi256_ps(_mm256_set1_epi32(int(0x80000000)));
  auto pDst2 = pDst;
  auto pSrc2 = pSrc;

  for (int y = 0; y < nHeight; ++y) {
    float *d = reinterpret_cast<float *>(pDst);
    const float *s = reinterpret_cast<const float *>(pSrc);
    for (int x = 0; x < wMod8; x += 8) {
      auto a = _mm256_sub_ps(_mm256_loadu_ps(d + x), _mm256_loadu_ps(s + x));
      a = _mm256_xor_ps(a, _mm256_and_ps(_mm256_cmp_ps(a, zero, _CMP_LT_OQ), sign));
      auto r = _mm256_blendv_ps(a, one, _mm256_cmp_ps(a, hi, _CMP_GT_OQ));
      _mm256_storeu_ps(d + x, _mm256_andnot_ps(_mm256_cmp_ps(a, lo, _CMP_LE_OQ), r));
    }
    pDst += nDstPitch;
    pSrc += nSrcPitch;
  }

  if (nWidth > wMod8)
    mask32_c_op(pDst2 + wMod8 * sizeof(float), nDstPitch, pSrc2 + wMod8 * sizeof(float), nSrcPitch, nLowThreshold, nHighThreshold, nWidth - wMod8, nHeight);
}

} // namespace

Processor *mask_avx2 = &mask_t<sad_avx2_op, mask_avx2_op>;
Processor16 *mask16_avx2 = &mask16_t<sad16_avx2_op, mask16_avx2_op>;
Processor32 *mask32_avx2 = &mask32_t<sad32_avx2_op, mask32_avx2_op>;

} } } } }

#endif // INTEL_INTRINSICS
