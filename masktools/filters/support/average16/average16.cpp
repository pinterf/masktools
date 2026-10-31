#include "../average/average.h"
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


} } } } }