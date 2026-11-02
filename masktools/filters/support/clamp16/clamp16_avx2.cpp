#include "../clamp/clamp.h"

#ifdef INTEL_INTRINSICS

#include "../../../common/simd_avx2.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Support  { namespace Clamp {

namespace {

template<bool lessThan16bits>
static void clamp16_native_avx2_t(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pUpLimit, ptrdiff_t nUpLimitPitch, const uint8_t *pLowLimit, ptrdiff_t nLowLimitPitch, int nWidth, int nHeight, int bits_per_pixel, int nOvershoot, int nUndershoot)
{
    nWidth *= 2; // really rowsize: width * sizeof(uint16), see also division at C trailer

    int wMod32 = (nWidth / 32) * 32;
    auto pDst2 = pDst;
    auto pUpLimit2 = pUpLimit;
    auto pLowLimit2 = pLowLimit;

    auto overshoot_v = _mm256_set1_epi16(uint16_t(nOvershoot));
    auto undershoot_v = _mm256_set1_epi16(uint16_t(nUndershoot));
    auto max_pixel_value = _mm256_set1_epi16(uint16_t((1 << bits_per_pixel) - 1));

    for ( int j = 0; j < nHeight; ++j ) {
        for ( int i = 0; i < wMod32; i+=32 ) {
            auto upper_limit = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pUpLimit+i));
            auto lower_limit = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pLowLimit+i));
            auto limited = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pDst+i));

            upper_limit = _mm256_adds_epu16(upper_limit, overshoot_v);
            if (lessThan16bits)
              upper_limit = _mm256_min_epu16(upper_limit, max_pixel_value);
            lower_limit = _mm256_subs_epu16(lower_limit, undershoot_v);

            limited = _mm256_min_epu16(limited, upper_limit);
            limited = _mm256_max_epu16(limited, lower_limit);

            _mm256_storeu_si256(reinterpret_cast<__m256i*>(pDst+i), limited);
        }
        pDst += nDstPitch;
        pUpLimit += nUpLimitPitch;
        pLowLimit += nLowLimitPitch;
    }

    if (nWidth > wMod32) {
        clamp16_native_c(pDst2 + wMod32, nDstPitch, pUpLimit2 + wMod32, nUpLimitPitch, pLowLimit2 + wMod32, nLowLimitPitch, (nWidth - wMod32) / sizeof(uint16_t), nHeight, bits_per_pixel, nOvershoot, nUndershoot);
    }
}

// one template switch per call: 16 bit needs no clamping to max_pixel_value
void clamp16_native_avx2_dispatch(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pUpLimit, ptrdiff_t nUpLimitPitch, const uint8_t *pLowLimit, ptrdiff_t nLowLimitPitch, int nWidth, int nHeight, int bits_per_pixel, int nOvershoot, int nUndershoot)
{
    if (bits_per_pixel == 16)
        clamp16_native_avx2_t<false>(pDst, nDstPitch, pUpLimit, nUpLimitPitch, pLowLimit, nLowLimitPitch, nWidth, nHeight, bits_per_pixel, nOvershoot, nUndershoot);
    else
        clamp16_native_avx2_t<true>(pDst, nDstPitch, pUpLimit, nUpLimitPitch, pLowLimit, nLowLimitPitch, nWidth, nHeight, bits_per_pixel, nOvershoot, nUndershoot);
}

} // namespace

Processor16 *clamp16_native_avx2 = &clamp16_native_avx2_dispatch;

} } } } }

#endif // INTEL_INTRINSICS
