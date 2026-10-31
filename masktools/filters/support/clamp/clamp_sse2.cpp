#include "clamp.h"

#ifdef INTEL_INTRINSICS

#include "../../../common/simd.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Support  { namespace Clamp {

namespace {

template<MemoryMode mem_mode>
static void clamp_sse2_t(uint8_t *pDst, ptrdiff_t dst_pitch, const uint8_t *pSrc1, ptrdiff_t src1_pitch, const uint8_t *pSrc2, ptrdiff_t src2_pitch, int width, int height, int overshoot, int undershoot)
{
    int mod16_width = (width / 16) * 16;
    auto pDst_s = pDst;
    auto pSrc1_s = pSrc1;
    auto pSrc2_s = pSrc2;

    auto overshoot_v = _mm_set1_epi8(uint8_t(overshoot));
    auto undershoot_v = _mm_set1_epi8(uint8_t(undershoot));

    for ( int j = 0; j < height; ++j ) {
        for ( int i = 0; i < mod16_width; i+=16 ) {
            auto upper_limit = simd_load_si128<mem_mode>(pSrc1+i);
            auto lower_limit = simd_load_si128<mem_mode>(pSrc2+i);

            upper_limit = _mm_adds_epu8(upper_limit, overshoot_v);
            lower_limit = _mm_subs_epu8(lower_limit, undershoot_v);

            auto limited = simd_load_si128<mem_mode>(pDst+i);

            limited = _mm_min_epu8(limited, upper_limit);
            limited = _mm_max_epu8(limited, lower_limit);

            simd_store_si128<mem_mode>(pDst+i, limited);
        }
        pDst += dst_pitch;
        pSrc1 += src1_pitch;
        pSrc2 += src2_pitch;
    }

    if (width > mod16_width) {
        clamp_c(pDst_s + mod16_width, dst_pitch, pSrc1_s + mod16_width, src1_pitch, pSrc2_s+mod16_width, src2_pitch, width - mod16_width, height, overshoot, undershoot);
    }
}

template<MemoryMode mem_mode>
static void clamp32_sse2_t(uint8_t *pDst, ptrdiff_t dst_pitch, const uint8_t *pSrc1, ptrdiff_t src1_pitch, const uint8_t *pSrc2, ptrdiff_t src2_pitch, int width, int height, float overshoot, float undershoot)
{
  width *= sizeof(float);
  int mod16_width = (width / 16) * 16;
  auto pDst_s = pDst;
  auto pSrc1_s = pSrc1;
  auto pSrc2_s = pSrc2;

  auto overshoot_v = _mm_set1_ps(overshoot);
  auto undershoot_v = _mm_set1_ps(undershoot);

  for (int j = 0; j < height; ++j) {
    for (int i = 0; i < mod16_width; i += 16) {
      auto upper_limit = simd_load_ps<mem_mode>(pSrc1 + i);
      auto lower_limit = simd_load_ps<mem_mode>(pSrc2 + i);

      upper_limit = _mm_add_ps(upper_limit, overshoot_v);  // no clamp or saturation
      lower_limit = _mm_sub_ps(lower_limit, undershoot_v);

      auto limited = simd_load_ps<mem_mode>(pDst + i);

      limited = _mm_min_ps(limited, upper_limit);
      limited = _mm_max_ps(limited, lower_limit);

      simd_store_ps<mem_mode>(pDst + i, limited);
    }
    pDst += dst_pitch;
    pSrc1 += src1_pitch;
    pSrc2 += src2_pitch;
  }

  if (width > mod16_width) {
    clamp32_c(pDst_s + mod16_width, dst_pitch, pSrc1_s + mod16_width, src1_pitch, pSrc2_s + mod16_width, src2_pitch, (width - mod16_width) / sizeof(float), height, overshoot, undershoot);
  }
}

} // namespace

Processor *clamp_sse2 = &clamp_sse2_t<MemoryMode::SSE2_UNALIGNED>;
Processor *clamp_asse2 = &clamp_sse2_t<MemoryMode::SSE2_ALIGNED>;

Processor32 *clamp32_sse2 = &clamp32_sse2_t<MemoryMode::SSE2_UNALIGNED>;
Processor32 *clamp32_asse2 = &clamp32_sse2_t<MemoryMode::SSE2_ALIGNED>;

} } } } }

#endif // INTEL_INTRINSICS
