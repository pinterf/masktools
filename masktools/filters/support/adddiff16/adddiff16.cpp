#include "../adddiff/adddiff.h"
#include "../../../common/simd.h"
#include "../../../common/16bit.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Support  { namespace AddDiff {

template<int bits_per_pixel>
static MT_FORCEINLINE uint16_t adddiff16_core_c(uint16_t dst, uint16_t src) {
    const int max_pixel_value = (1 << bits_per_pixel) - 1;
    const int half = 1 << (bits_per_pixel - 1);
    return clip<uint16_t, int>(int(dst) + src - (half), 0, max_pixel_value);
}

template<CpuFlags flags, bool lessThan16bits>
static MT_FORCEINLINE __m128i adddiff16_core_simd(const __m128i &dst, const __m128i &src, const __m128i &halfrange, const __m128i &max_pixel_value) {
  if (lessThan16bits) {
    auto val = _mm_add_epi16(src, dst);
    auto result = _mm_subs_epu16(val, halfrange);
    return simd_min_epu16<flags>(result, max_pixel_value);
  }
  else {
    auto dstval = _mm_sub_epi16(dst, halfrange);
    auto srcval = _mm_sub_epi16(src, halfrange);
    auto result = _mm_adds_epi16(dstval, srcval);
    return _mm_add_epi16(result, halfrange);
  }
}


void adddiff16_stacked_c(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int nWidth, int nHeight, int nOrigHeight)
{
   auto pDstLsb = pDst + nDstPitch * nOrigHeight / 2;
   auto pSrcLsb = pSrc + nSrcPitch * nOrigHeight / 2;

   for (int y = 0; y < nHeight / 2; y++) {
       for (int x = 0; x < nWidth; x++) {
           uint16_t dst = read_word_stacked(pDst, pDstLsb, x);
           uint16_t src = read_word_stacked(pSrc, pSrcLsb, x);

           uint16_t result = adddiff16_core_c<16>(dst, src);

           write_word_stacked(pDst, pDstLsb, x, result);
       }
       pDst += nDstPitch;
       pDstLsb += nDstPitch;
       pSrc += nSrcPitch;
       pSrcLsb += nSrcPitch;
   }
}

template<int bits_per_pixel>
void adddiff16_native_c(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int nWidth, int nHeight, int)
{
    for (int y = 0; y < nHeight; y++) {
        auto pDstWord = reinterpret_cast<uint16_t*>(pDst);
        auto pSrcWord = reinterpret_cast<const uint16_t*>(pSrc);

        for (int x = 0; x < nWidth; x++) { // width is real width
          pDstWord[x] = adddiff16_core_c<bits_per_pixel>(pDstWord[x], pSrcWord[x]);
        }
        pDst += nDstPitch;
        pSrc += nSrcPitch;
    }
}

#pragma warning(disable: 4309)

void adddiff16_stacked_sse2(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int nWidth, int nHeight, int nOrigHeight)
{
    int wMod8 = (nWidth / 8) * 8;
    auto pDst2 = pDst;
    auto pSrc2 = pSrc;

    auto pDstLsb = pDst + nDstPitch * nOrigHeight / 2;
    auto pSrcLsb = pSrc + nSrcPitch * nOrigHeight / 2;

    auto ff = _mm_set1_epi16(0x00FF);
    auto zero = _mm_setzero_si128();
    auto halfrange = _mm_set1_epi16(0x8000);

    for ( int j = 0; j < nHeight / 2; ++j ) {
        for ( int i = 0; i < wMod8; i+=8 ) {
            auto dst = read_word_stacked_simd(pDst, pDstLsb, i);
            auto src = read_word_stacked_simd(pSrc, pSrcLsb, i);
            auto result = adddiff16_core_simd<CPU_SSE2,false>(dst, src, halfrange, halfrange /*dummy*/);

            write_word_stacked_simd(pDst, pDstLsb, i, result, ff, zero);
        }
        pDst += nDstPitch;
        pDstLsb += nDstPitch;
        pSrc += nSrcPitch;
        pSrcLsb += nSrcPitch;
    }

    if (nWidth > wMod8) {
        adddiff16_stacked_c(pDst2 + wMod8, nDstPitch, pSrc2 + wMod8, nSrcPitch, nWidth - wMod8, nHeight, nOrigHeight);
    }
}

// for 16 bits SSE2 is enough, 10-14 can benefit from SSE4_1
template<CpuFlags flags, int bits_per_pixel>
void adddiff16_native_simd(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int nWidth, int nHeight, int nOrigHeight)
{
    nWidth *= 2; // really rowsize: width * sizeof(uint16), see also division at C trailer

    int wMod16 = (nWidth / 16) * 16;

    auto pDst2 = pDst;
    auto pSrc2 = pSrc;
    auto halfrange = _mm_set1_epi16(1 << (bits_per_pixel - 1));
    int _max_pixel_value = (1 << bits_per_pixel) - 1;
#pragma warning(disable: 4244)
    auto max_pixel_value = _mm_set1_epi16(_max_pixel_value);
#pragma warning(default: 4244)

    for (int j = 0; j < nHeight; ++j) {
        for (int i = 0; i < wMod16; i+=16) {
            auto dst = simd_load_si128<MemoryMode::SSE2_UNALIGNED>(reinterpret_cast<const __m128i*>(pDst+i));
            auto src = simd_load_si128<MemoryMode::SSE2_UNALIGNED>(reinterpret_cast<const __m128i*>(pSrc+i));

            __m128i result;
            if(bits_per_pixel < 16)
              result = adddiff16_core_simd<flags, true>(dst, src, halfrange, max_pixel_value);
            else
              result = adddiff16_core_simd<flags, false>(dst, src, halfrange, max_pixel_value);

            simd_store_si128<MemoryMode::SSE2_UNALIGNED>(reinterpret_cast<__m128i*>(pDst+i), result);
        }
        pDst += nDstPitch;
        pSrc += nSrcPitch;
    }
    if (nWidth > wMod16) {
        adddiff16_native_c<bits_per_pixel>(pDst2 + wMod16, nDstPitch, pSrc2 + wMod16, nSrcPitch, (nWidth - wMod16)/sizeof(uint16_t), nHeight, nOrigHeight);
    }
}

#define MAKE_TEMPLATES(bits_per_pixel) \
template void adddiff16_native_c<bits_per_pixel>(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int nWidth, int nHeight, int nOrigHeight); \
template void adddiff16_native_simd<CPU_SSE2,bits_per_pixel>(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int nWidth, int nHeight, int nOrigHeight); \
template void adddiff16_native_simd<CPU_SSE4_1,bits_per_pixel>(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int nWidth, int nHeight, int nOrigHeight);
MAKE_TEMPLATES(10)
MAKE_TEMPLATES(12)
MAKE_TEMPLATES(14)
MAKE_TEMPLATES(16)
#undef MAKE_TEMPLATES

#define MAKE_EXPORTS(bits_per_pixel) \
Processor16 *adddiff16_native_##bits_per_pixel##_c = &adddiff16_native_c<bits_per_pixel>; \
Processor16 *adddiff16_native_##bits_per_pixel##_sse2 = &adddiff16_native_simd<CPU_SSE2,bits_per_pixel>; \
Processor16 *adddiff16_native_##bits_per_pixel##_sse4_1 = &adddiff16_native_simd<CPU_SSE4_1, bits_per_pixel>;

MAKE_EXPORTS(10)
MAKE_EXPORTS(12)
MAKE_EXPORTS(14)
MAKE_EXPORTS(16)
#undef MAKE_EXPORTS

} } } } }