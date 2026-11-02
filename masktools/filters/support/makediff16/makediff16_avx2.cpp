#include "../makediff/makediff.h"

#ifdef INTEL_INTRINSICS

#include "../../../common/simd_avx2.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Support  { namespace MakeDiff {

namespace {

template<bool lessThan16bits>
static void makediff16_native_avx2_t(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int nWidth, int nHeight, int bits_per_pixel)
{
    nWidth *= 2; // really rowsize: width * sizeof(uint16), see also division at C trailer

    int wMod32 = (nWidth / 32) * 32;

    auto pDst2 = pDst;
    auto pSrc2 = pSrc;
    auto halfrange = _mm256_set1_epi16(int16_t(1 << (bits_per_pixel - 1)));
    auto max_pixel_value = _mm256_set1_epi16(int16_t((1 << bits_per_pixel) - 1));

    for (int j = 0; j < nHeight; ++j) {
        for (int i = 0; i < wMod32; i+=32) {
            auto dst = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pDst+i));
            auto src = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pSrc+i));
            __m256i result;
            if (lessThan16bits) {
                result = _mm256_min_epu16(_mm256_subs_epu16(_mm256_add_epi16(dst, halfrange), src), max_pixel_value);
            }
            else {
                auto dstval = _mm256_sub_epi16(dst, halfrange);
                auto srcval = _mm256_sub_epi16(src, halfrange);
                result = _mm256_add_epi16(_mm256_subs_epi16(dstval, srcval), halfrange);
            }
            _mm256_storeu_si256(reinterpret_cast<__m256i*>(pDst + i), result);
        }
        pDst += nDstPitch;
        pSrc += nSrcPitch;
    }
    if (nWidth > wMod32) {
        makediff16_native_c(pDst2 + wMod32, nDstPitch, pSrc2 + wMod32, nSrcPitch, (nWidth - wMod32) / sizeof(uint16_t), nHeight, bits_per_pixel);
    }
}

// one template switch per call: 16 bit uses the signed pivot, 10-14 bit the unsigned add + min
void makediff16_native_avx2_dispatch(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int nWidth, int nHeight, int bits_per_pixel)
{
    if (bits_per_pixel == 16)
        makediff16_native_avx2_t<false>(pDst, nDstPitch, pSrc, nSrcPitch, nWidth, nHeight, bits_per_pixel);
    else
        makediff16_native_avx2_t<true>(pDst, nDstPitch, pSrc, nSrcPitch, nWidth, nHeight, bits_per_pixel);
}

} // namespace

Processor16 *makediff16_native_avx2 = &makediff16_native_avx2_dispatch;

} } } } }

#endif // INTEL_INTRINSICS
