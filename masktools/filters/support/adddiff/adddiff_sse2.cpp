#include "adddiff.h"

#ifdef INTEL_INTRINSICS

#include "../../../common/simd.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Support  { namespace AddDiff {

namespace {

template<MemoryMode mem_mode>
static void adddiff_sse2_t(uint8_t *pDst, ptrdiff_t dst_pitch, const uint8_t *pSrc, ptrdiff_t src_pitch, int width, int height)
{
    int mod32_width = (width / 32) * 32;
    auto pDst2 = pDst;
    auto pSrc2 = pSrc;
    auto v128 = _mm_set1_epi32(0x80808080);

    for ( int j = 0; j < height; ++j ) {
        for ( int i = 0; i < mod32_width; i+=32 ) {
            _mm_prefetch(reinterpret_cast<const char*>(pDst)+i+128, _MM_HINT_T0);
            _mm_prefetch(reinterpret_cast<const char*>(pSrc)+i+128, _MM_HINT_T0);

            auto dst = simd_load_si128<mem_mode>(pDst+i);
            auto dst2 = simd_load_si128<mem_mode>(pDst+i+16);
            auto src = simd_load_si128<mem_mode>(pSrc+i);
            auto src2 = simd_load_si128<mem_mode>(pSrc+i+16);

            auto dstsub = _mm_sub_epi8(dst, v128);
            auto dstsub2 = _mm_sub_epi8(dst2, v128);

            auto srcsub = _mm_sub_epi8(src, v128);
            auto srcsub2 = _mm_sub_epi8(src2, v128);

            auto added = _mm_adds_epi8(dstsub, srcsub);
            auto added2 = _mm_adds_epi8(dstsub2, srcsub2);

            auto result = _mm_add_epi8(added, v128);
            auto result2 = _mm_add_epi8(added2, v128);

            simd_store_si128<mem_mode>(pDst+i, result);
            simd_store_si128<mem_mode>(pDst+i+16, result2);
        }
        pDst += dst_pitch;
        pSrc += src_pitch;
    }

    if (width > mod32_width) {
        adddiff_c(pDst2 + mod32_width, dst_pitch, pSrc2 + mod32_width, src_pitch, width - mod32_width, height);
    }
}

template<MemoryMode mem_mode>
static void adddiff32_sse2_t(uint8_t *pDst, ptrdiff_t dst_pitch, const uint8_t *pSrc, ptrdiff_t src_pitch, int width, int height, bool chroma)
{
  width *= sizeof(float);

  int mod16_width = (width / 16) * 16;
  auto pDst2 = pDst;
  auto pSrc2 = pSrc;
#ifdef FLOAT_CHROMA_IS_HALF_CENTERED
  const auto vHalf = _mm_set1_ps(0.5f);  // also for chroma
#else
  const auto vHalf = _mm_set1_ps(chroma ? 0.0f : 0.5f);
#endif

  for (int j = 0; j < height; ++j) {
    for (int i = 0; i < mod16_width; i += 16) {
      _mm_prefetch(reinterpret_cast<const char*>(pDst) + i + 128, _MM_HINT_T0);
      _mm_prefetch(reinterpret_cast<const char*>(pSrc) + i + 128, _MM_HINT_T0);

      auto dst = simd_load_ps<mem_mode>(pDst + i);
      auto src = simd_load_ps<mem_mode>(pSrc + i);

      auto result = _mm_sub_ps(_mm_add_ps(dst, src), vHalf); // to work like "x y + 128 -"

      simd_store_ps<mem_mode>(pDst + i, result);
    }
    pDst += dst_pitch;
    pSrc += src_pitch;
  }

  if (width > mod16_width) {
    adddiff32_c(pDst2 + mod16_width, dst_pitch, pSrc2 + mod16_width, src_pitch, (width - mod16_width) / sizeof(float), height, chroma);
  }
}

} // namespace

Processor *adddiff_sse2 = &adddiff_sse2_t<MemoryMode::SSE2_UNALIGNED>;
Processor *adddiff_asse2 = &adddiff_sse2_t<MemoryMode::SSE2_ALIGNED>;

Processor32 *adddiff32_sse2 = &adddiff32_sse2_t<MemoryMode::SSE2_UNALIGNED>;
Processor32 *adddiff32_asse2 = &adddiff32_sse2_t<MemoryMode::SSE2_ALIGNED>;

} } } } }

#endif // INTEL_INTRINSICS
