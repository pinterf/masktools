#include "../average/average.h"
#include "../../../common/simd.h"
#include "../../../common/16bit.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Support  { namespace Average {

void average16_stacked_c(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int nWidth, int nHeight, int nOrigHeight)
{
    auto pDstLsb = pDst + nDstPitch * nOrigHeight / 2;
    auto pSrcLsb = pSrc + nSrcPitch * nOrigHeight / 2;

    for (int y = 0; y < nHeight / 2; y++) {
        for (int x = 0; x < nWidth; x++) {
            uint16_t dst = read_word_stacked(pDst, pDstLsb, x);
            uint16_t src = read_word_stacked(pSrc, pSrcLsb, x);

            uint16_t avg = (dst + src + 1) >> 1;

            write_word_stacked(pDst, pDstLsb, x, avg);
        }
        pDst += nDstPitch;
        pDstLsb += nDstPitch;
        pSrc += nSrcPitch;
        pSrcLsb += nSrcPitch;
    }
}

void average16_native_c(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int nWidth, int nHeight, int)
{
    for (int y = 0; y < nHeight; y++) {
        auto pDstWord = reinterpret_cast<uint16_t*>(pDst);
        auto pSrcWord = reinterpret_cast<const uint16_t*>(pSrc);

        for (int x = 0; x < nWidth; x++) {
            pDstWord[x] = (pDstWord[x] + pSrcWord[x] + 1) >> 1;
        }
        pDst += nDstPitch;
        pSrc += nSrcPitch;
    }
}


void average16_stacked_sse2(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int nWidth, int nHeight, int nOrigHeight)
{
    int wMod8 = (nWidth / 8) * 8;

    auto pDst2 = pDst;
    auto pSrc2 = pSrc;
    auto pDstLsb = pDst + nDstPitch * nOrigHeight / 2;
    auto pSrcLsb = pSrc + nSrcPitch * nOrigHeight / 2;

    auto ff = _mm_set1_epi16(0x00FF);
    auto zero = _mm_setzero_si128();

    for ( int j = 0; j < nHeight / 2; ++j ) {
        for ( int i = 0; i < wMod8; i+=8 ) {
            auto dst = read_word_stacked_simd(pDst, pDstLsb, i);
            auto src = read_word_stacked_simd(pSrc, pSrcLsb, i);

            auto result = _mm_avg_epu16(dst, src);

            write_word_stacked_simd(pDst, pDstLsb, i, result, ff, zero);
        }
        pDst += nDstPitch;
        pDstLsb += nDstPitch;
        pSrc += nSrcPitch;
        pSrcLsb += nSrcPitch;
    }
    if (nWidth > wMod8) {
        average16_stacked_c(pDst2 + wMod8, nDstPitch, pSrc2 + wMod8, nSrcPitch, nWidth - wMod8, nHeight, nOrigHeight);
    }
}

void average16_native_sse2(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, int nWidth, int nHeight, int nOrigHeight)
{
    nWidth *= 2; // really rowsize: width * sizeof(uint16), see also division at C trailer
  
    int wMod16 = (nWidth / 16) * 16;

    auto pDst2 = pDst;
    auto pSrc2 = pSrc;

    for (int j = 0; j < nHeight; ++j) {
        for (int i = 0; i < wMod16; i+=16) {
            auto dst = simd_load_si128<MemoryMode::SSE2_UNALIGNED>(reinterpret_cast<const __m128i*>(pDst+i));
            auto src = simd_load_si128<MemoryMode::SSE2_UNALIGNED>(reinterpret_cast<const __m128i*>(pSrc+i));

            auto result = _mm_avg_epu16(dst, src);

            simd_store_si128<MemoryMode::SSE2_UNALIGNED>(reinterpret_cast<__m128i*>(pDst+i), result);
        }
        pDst += nDstPitch;
        pSrc += nSrcPitch;
    }
    if (nWidth > wMod16) {
        average16_native_c(pDst2 + wMod16, nDstPitch, pSrc2 + wMod16, nSrcPitch, (nWidth - wMod16) / sizeof(uint16_t), nHeight, nOrigHeight);
    }
}



} } } } }