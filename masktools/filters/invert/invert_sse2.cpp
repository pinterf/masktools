#include "invert.h"

#ifdef INTEL_INTRINSICS

#include "../../common/simd.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Invert {

namespace {

template<int bits_per_pixel>
void invert16_t_sse2(uint8_t *pDst, ptrdiff_t nDstPitch, int nWidth, int nHeight)
{
  nWidth *= 2; // really rowsize: width * sizeof(uint16)

  int max_pixel_value_mask = (1 << bits_per_pixel) - 1;
#pragma warning(disable: 4244)
  auto fff = _mm_set1_epi16(max_pixel_value_mask);
#pragma warning(default: 4244)
  for (int j = 0; j < nHeight; j++) {
    for (int i = 0; i < nWidth; i += 16) {
      auto src = simd_load_si128<MemoryMode::SSE2_UNALIGNED>(pDst + i);
      auto result = _mm_xor_si128(src, fff);
      simd_store_si128<MemoryMode::SSE2_UNALIGNED>(pDst + i, result);
    }
    pDst += nDstPitch;
  }
}

} // namespace

void invert_sse2(uint8_t *pDst, ptrdiff_t nDstPitch, int nWidth, int nHeight)
{
    auto fff = _mm_set1_epi32(0xFFFFFFFF);
    for ( int j = 0; j < nHeight; j++ ) {
        for ( int i = 0; i < nWidth; i+=16 ) {
            auto src = simd_load_si128<MemoryMode::SSE2_UNALIGNED>(pDst+i);
            auto result = _mm_xor_si128(src, fff);
            simd_store_si128<MemoryMode::SSE2_UNALIGNED>(pDst+i, result);
        }
        pDst += nDstPitch;
    }
}

void invert32_sse2(uint8_t *pDst, ptrdiff_t nDstPitch, int nWidth, int nHeight)
{
  nWidth *= sizeof(float);

  auto one = _mm_set1_ps(1.0f);
  for (int j = 0; j < nHeight; j++) {
    for (int i = 0; i < nWidth; i += 16) {
      auto src = simd_load_ps<MemoryMode::SSE2_UNALIGNED>(pDst + i);
      auto result = _mm_sub_ps(one, src);
      simd_store_ps<MemoryMode::SSE2_UNALIGNED>(pDst + i, result);
    }
    pDst += nDstPitch;
  }
}
Processor *invert10_sse2 = &invert16_t_sse2<10>;
Processor *invert12_sse2 = &invert16_t_sse2<12>;
Processor *invert14_sse2 = &invert16_t_sse2<14>;
Processor *invert16_sse2 = &invert16_t_sse2<16>;

} } } }

#endif // INTEL_INTRINSICS
