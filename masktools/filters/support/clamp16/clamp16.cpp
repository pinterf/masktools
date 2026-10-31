#include "../clamp/clamp.h"
#include "../../../common/simd.h"
#include "../../../common/16bit.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Support  { namespace Clamp {

template<int bits_per_pixel>
static uint16_t MT_FORCEINLINE clamp16_core_c(uint16_t dst, uint16_t upperLimit, uint16_t lowerLimit, int overshoot, int undershoot) {
    uint16_t result = static_cast<uint16_t>(dst > upperLimit + overshoot ? upperLimit + overshoot : dst);
    if (bits_per_pixel < 16)
      return min(static_cast<uint16_t>(result < lowerLimit - undershoot ? lowerLimit - undershoot : result), uint16_t((1 << bits_per_pixel) - 1));
    else
      return static_cast<uint16_t>(result < lowerLimit - undershoot ? lowerLimit - undershoot : result);
}

template<CpuFlags flags, bool lessThan16bits>
static __m128i MT_FORCEINLINE clamp16_core_simd(const __m128i &dst, const __m128i &upperLimit, const __m128i &lowerLimit, const __m128i &overshoot, const __m128i &undershoot, const __m128i &max_pixel_value) {
    auto upper_limit_real = _mm_adds_epu16(upperLimit, overshoot);
    if (lessThan16bits)
      upper_limit_real = simd_min_epu16<flags>(upper_limit_real, max_pixel_value);
    auto lower_limit_real = _mm_subs_epu16(lowerLimit, undershoot);

    auto limited = simd_min_epu16<flags>(dst, upper_limit_real); // _mm_min_epu16: SSE4.1 function. Simulated on SSE2
    return simd_max_epu16<flags>(limited, lower_limit_real); // _mm_max_epu16: SSE4.1 function. Simulated on SSE2
}

void clamp16_stacked_c(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pUpLimit, ptrdiff_t nUpLimitPitch, const uint8_t *pLowLimit, ptrdiff_t nLowLimitPitch, int nWidth, int nHeight, int nOrigHeight, int nOvershoot, int nUndershoot)
{
   auto pDstLsb = pDst + nDstPitch * nOrigHeight / 2;
   auto pUpLimitLsb = pUpLimit + nUpLimitPitch * nOrigHeight / 2;
   auto pLowLimitLsb = pLowLimit + nLowLimitPitch * nOrigHeight / 2;

   for (int y = 0; y < nHeight / 2; y++) {
       for (int x = 0; x < nWidth; x++) {
           uint16_t dst = read_word_stacked(pDst, pDstLsb, x);
           uint16_t upperLimit = read_word_stacked(pUpLimit, pUpLimitLsb, x);
           uint16_t lowerLimit = read_word_stacked(pLowLimit, pLowLimitLsb, x);

           uint16_t result = clamp16_core_c<16>(dst, upperLimit, lowerLimit, nOvershoot, nUndershoot);

           write_word_stacked(pDst, pDstLsb, x, result);
       }
       pDst += nDstPitch;
       pDstLsb += nDstPitch;
       pUpLimit += nUpLimitPitch;
       pUpLimitLsb += nUpLimitPitch;
       pLowLimit += nLowLimitPitch;
       pLowLimitLsb += nLowLimitPitch;
   }
}

template<int bits_per_pixel>
void clamp16_native_c(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pUpLimit, ptrdiff_t nUpLimitPitch, const uint8_t *pLowLimit, ptrdiff_t nLowLimitPitch, int nWidth, int nHeight, int nOrigHeight, int nOvershoot, int nUndershoot)
{
    UNUSED(nOrigHeight);
    for (int y = 0; y < nHeight; y++) {
        auto pDstWord = reinterpret_cast<uint16_t*>(pDst);
        auto pUpLimitWord = reinterpret_cast<const uint16_t*>(pUpLimit);
        auto pLowLimitWord = reinterpret_cast<const uint16_t*>(pLowLimit);

        for (int x = 0; x < nWidth; x++) {
            pDstWord[x] = clamp16_core_c<bits_per_pixel>(pDstWord[x], pUpLimitWord[x], pLowLimitWord[x], nOvershoot, nUndershoot);
        }
        pDst += nDstPitch;
        pUpLimit += nUpLimitPitch;
        pLowLimit += nLowLimitPitch;
    }
}

template<CpuFlags flags>
void clamp16_stacked_simd(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pUpLimit, ptrdiff_t nUpLimitPitch, const uint8_t *pLowLimit, ptrdiff_t nLowLimitPitch, int nWidth, int nHeight, int nOrigHeight, int nOvershoot, int nUndershoot)
{
    int wMod8 = (nWidth / 8) * 8;
    auto pDst2 = pDst;
    auto pUpLimit2 = pUpLimit;
    auto pLowLimit2 = pLowLimit;

    auto pDstLsb = pDst + nDstPitch * nOrigHeight / 2;
    auto pUpLimitLsb = pUpLimit + nUpLimitPitch * nOrigHeight / 2;
    auto pLowLimitLsb = pLowLimit + nLowLimitPitch * nOrigHeight / 2;

    auto overshoot_v = _mm_set1_epi16(uint16_t(nOvershoot));
    auto undershoot_v = _mm_set1_epi16(uint16_t(nUndershoot));
    auto ff = _mm_set1_epi16(0x00FF);
    auto zero = _mm_setzero_si128();
#pragma warning(disable: 4310)
    auto max_pixel_value = _mm_set1_epi16((short)0xFFFF); // n/a for 16 bits
#pragma warning(default: 4310)

    for ( int j = 0; j < nHeight / 2; ++j ) {
        for ( int i = 0; i < wMod8; i+=8 ) {
            auto upper_limit = read_word_stacked_simd(pUpLimit, pUpLimitLsb, i);
            auto lower_limit = read_word_stacked_simd(pLowLimit, pLowLimitLsb, i);
            auto limited = read_word_stacked_simd(pDst, pDstLsb, i);

            limited = clamp16_core_simd<flags, false>(limited, upper_limit, lower_limit, overshoot_v, undershoot_v, max_pixel_value);

            write_word_stacked_simd(pDst, pDstLsb, i, limited, ff, zero);
        }
        pDst += nDstPitch;
        pDstLsb += nDstPitch;
        pUpLimit += nUpLimitPitch;
        pUpLimitLsb += nUpLimitPitch;
        pLowLimit += nLowLimitPitch;
        pLowLimitLsb += nLowLimitPitch;
    }

    if (nWidth > wMod8) {
        clamp16_stacked_c(pDst2 + wMod8, nDstPitch, pUpLimit2 + wMod8, nUpLimitPitch, pLowLimit2+wMod8, nLowLimitPitch, nWidth - wMod8, nHeight, nOrigHeight, nOvershoot, nUndershoot);
    }
}

template<CpuFlags flags, int bits_per_pixel>
void clamp16_native_simd(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pUpLimit, ptrdiff_t nUpLimitPitch, const uint8_t *pLowLimit, ptrdiff_t nLowLimitPitch, int nWidth, int nHeight, int nOrigHeight, int nOvershoot, int nUndershoot)
{
    nWidth *= 2; // really rowsize: width * sizeof(uint16), see also division at C trailer
  
    int wMod16 = (nWidth / 16) * 16;
    auto pDst2 = pDst;
    auto pUpLimit2 = pUpLimit;
    auto pLowLimit2 = pLowLimit;

    auto overshoot_v = _mm_set1_epi16(uint16_t(nOvershoot));
    auto undershoot_v = _mm_set1_epi16(uint16_t(nUndershoot));
    int _max_pixel_value = (1 << bits_per_pixel) - 1;
    auto max_pixel_value = _mm_set1_epi16(uint16_t(_max_pixel_value));

    for ( int j = 0; j < nHeight; ++j ) {
        for ( int i = 0; i < wMod16; i+=16 ) {
            auto upper_limit = simd_load_si128<MemoryMode::SSE2_UNALIGNED>(reinterpret_cast<const __m128i*>(pUpLimit+i));
            auto lower_limit = simd_load_si128<MemoryMode::SSE2_UNALIGNED>(reinterpret_cast<const __m128i*>(pLowLimit+i));
            auto limited = simd_load_si128<MemoryMode::SSE2_UNALIGNED>(reinterpret_cast<const __m128i*>(pDst+i));
            if(bits_per_pixel < 16)
              limited = clamp16_core_simd<flags, true>(limited, upper_limit, lower_limit, overshoot_v, undershoot_v, max_pixel_value);
            else
              limited = clamp16_core_simd<flags, false>(limited, upper_limit, lower_limit, overshoot_v, undershoot_v, max_pixel_value);

            simd_store_si128<MemoryMode::SSE2_UNALIGNED>(reinterpret_cast<__m128i*>(pDst+i), limited);
        }
        pDst += nDstPitch;
        pUpLimit += nUpLimitPitch;
        pLowLimit += nLowLimitPitch;
    }

    if (nWidth > wMod16) {
        clamp16_native_c<bits_per_pixel>(pDst2 + wMod16, nDstPitch, pUpLimit2 + wMod16, nUpLimitPitch, pLowLimit2+wMod16, nLowLimitPitch, (nWidth - wMod16) / sizeof(uint16_t), nHeight, nOrigHeight, nOvershoot, nUndershoot);
    }
}

template void clamp16_stacked_simd<CPU_SSE2>(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pUpLimit, ptrdiff_t nUpLimitPitch, const uint8_t *pLowLimit, ptrdiff_t nLowLimitPitch, int nWidth, int nHeight, int nOrigHeight, int nOvershoot, int nUndershoot);
template void clamp16_stacked_simd<CPU_SSE4_1>(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pUpLimit, ptrdiff_t nUpLimitPitch, const uint8_t *pLowLimit, ptrdiff_t nLowLimitPitch, int nWidth, int nHeight, int nOrigHeight, int nOvershoot, int nUndershoot);

template void clamp16_native_c<10>(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pUpLimit, ptrdiff_t nUpLimitPitch, const uint8_t *pLowLimit, ptrdiff_t nLowLimitPitch, int nWidth, int nHeight, int nOrigHeight, int nOvershoot, int nUndershoot);
template void clamp16_native_c<12>(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pUpLimit, ptrdiff_t nUpLimitPitch, const uint8_t *pLowLimit, ptrdiff_t nLowLimitPitch, int nWidth, int nHeight, int nOrigHeight, int nOvershoot, int nUndershoot);
template void clamp16_native_c<14>(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pUpLimit, ptrdiff_t nUpLimitPitch, const uint8_t *pLowLimit, ptrdiff_t nLowLimitPitch, int nWidth, int nHeight, int nOrigHeight, int nOvershoot, int nUndershoot);
template void clamp16_native_c<16>(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pUpLimit, ptrdiff_t nUpLimitPitch, const uint8_t *pLowLimit, ptrdiff_t nLowLimitPitch, int nWidth, int nHeight, int nOrigHeight, int nOvershoot, int nUndershoot);

template void clamp16_native_simd<CPU_SSE2,10>(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pUpLimit, ptrdiff_t nUpLimitPitch, const uint8_t *pLowLimit, ptrdiff_t nLowLimitPitch, int nWidth, int nHeight, int nOrigHeight, int nOvershoot, int nUndershoot);
template void clamp16_native_simd<CPU_SSE2,12>(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pUpLimit, ptrdiff_t nUpLimitPitch, const uint8_t *pLowLimit, ptrdiff_t nLowLimitPitch, int nWidth, int nHeight, int nOrigHeight, int nOvershoot, int nUndershoot);
template void clamp16_native_simd<CPU_SSE2,14>(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pUpLimit, ptrdiff_t nUpLimitPitch, const uint8_t *pLowLimit, ptrdiff_t nLowLimitPitch, int nWidth, int nHeight, int nOrigHeight, int nOvershoot, int nUndershoot);
template void clamp16_native_simd<CPU_SSE2,16>(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pUpLimit, ptrdiff_t nUpLimitPitch, const uint8_t *pLowLimit, ptrdiff_t nLowLimitPitch, int nWidth, int nHeight, int nOrigHeight, int nOvershoot, int nUndershoot);

template void clamp16_native_simd<CPU_SSE4_1,10>(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pUpLimit, ptrdiff_t nUpLimitPitch, const uint8_t *pLowLimit, ptrdiff_t nLowLimitPitch, int nWidth, int nHeight, int nOrigHeight, int nOvershoot, int nUndershoot);
template void clamp16_native_simd<CPU_SSE4_1,12>(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pUpLimit, ptrdiff_t nUpLimitPitch, const uint8_t *pLowLimit, ptrdiff_t nLowLimitPitch, int nWidth, int nHeight, int nOrigHeight, int nOvershoot, int nUndershoot);
template void clamp16_native_simd<CPU_SSE4_1,14>(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pUpLimit, ptrdiff_t nUpLimitPitch, const uint8_t *pLowLimit, ptrdiff_t nLowLimitPitch, int nWidth, int nHeight, int nOrigHeight, int nOvershoot, int nUndershoot);
template void clamp16_native_simd<CPU_SSE4_1,16>(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pUpLimit, ptrdiff_t nUpLimitPitch, const uint8_t *pLowLimit, ptrdiff_t nLowLimitPitch, int nWidth, int nHeight, int nOrigHeight, int nOvershoot, int nUndershoot);

Processor16 *clamp16_stacked_sse2 = &clamp16_stacked_simd<CPU_SSE2>;
Processor16 *clamp16_stacked_sse4_1 = &clamp16_stacked_simd<CPU_SSE4_1>;

Processor16 *clamp16_native_10_c = &clamp16_native_c<10>;
Processor16 *clamp16_native_12_c = &clamp16_native_c<12>;
Processor16 *clamp16_native_14_c = &clamp16_native_c<14>;
Processor16 *clamp16_native_16_c = &clamp16_native_c<16>;

Processor16 *clamp16_native_10_sse2 = &clamp16_native_simd<CPU_SSE2,10>;
Processor16 *clamp16_native_10_sse4_1 = &clamp16_native_simd<CPU_SSE4_1,10>;
Processor16 *clamp16_native_12_sse2 = &clamp16_native_simd<CPU_SSE2, 12>;
Processor16 *clamp16_native_12_sse4_1 = &clamp16_native_simd<CPU_SSE4_1, 12>;
Processor16 *clamp16_native_14_sse2 = &clamp16_native_simd<CPU_SSE2, 14>;
Processor16 *clamp16_native_14_sse4_1 = &clamp16_native_simd<CPU_SSE4_1, 14>;
Processor16 *clamp16_native_16_sse2 = &clamp16_native_simd<CPU_SSE2, 16>;
Processor16 *clamp16_native_16_sse4_1 = &clamp16_native_simd<CPU_SSE4_1, 16>;



} } } } }