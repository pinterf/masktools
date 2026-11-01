#include "invert.h"

#ifdef INTEL_INTRINSICS

#include "../../common/simd.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Invert {

void invert16_sse2(uint8_t *pDst, ptrdiff_t nDstPitch, int nWidth, int nHeight, int bits_per_pixel)
{
  nWidth *= 2; // really rowsize: width * sizeof(uint16)

  auto fff = _mm_set1_epi16(short((1 << bits_per_pixel) - 1));
  for (int j = 0; j < nHeight; j++) {
    for (int i = 0; i < nWidth; i += 16) {
      auto src = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pDst + i));
      auto result = _mm_xor_si128(src, fff);
      _mm_storeu_si128(reinterpret_cast<__m128i*>(pDst + i), result);
    }
    pDst += nDstPitch;
  }
}

void invert_sse2(uint8_t *pDst, ptrdiff_t nDstPitch, int nWidth, int nHeight, int)
{
    auto fff = _mm_set1_epi32(0xFFFFFFFF);
    for ( int j = 0; j < nHeight; j++ ) {
        for ( int i = 0; i < nWidth; i+=16 ) {
            auto src = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pDst+i));
            auto result = _mm_xor_si128(src, fff);
            _mm_storeu_si128(reinterpret_cast<__m128i*>(pDst+i), result);
        }
        pDst += nDstPitch;
    }
}

void invert32_sse2(uint8_t *pDst, ptrdiff_t nDstPitch, int nWidth, int nHeight, int)
{
  nWidth *= sizeof(float);

  auto one = _mm_set1_ps(1.0f);
  for (int j = 0; j < nHeight; j++) {
    for (int i = 0; i < nWidth; i += 16) {
      auto src = _mm_loadu_ps(reinterpret_cast<const float*>(pDst + i));
      auto result = _mm_sub_ps(one, src);
      _mm_storeu_ps(reinterpret_cast<float*>(pDst + i), result);
    }
    pDst += nDstPitch;
  }
}

} } } }

#endif // INTEL_INTRINSICS
