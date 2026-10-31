#ifndef __Mt_16bit_H__
#define __Mt_16bit_H__

#include "common.h"

namespace Filtering {

static MT_FORCEINLINE uint16_t read_word_stacked(const uint8_t *pMsb, const uint8_t *pLsb, int x) {
    return (uint16_t(pMsb[x]) << 8) + pLsb[x];
}

static MT_FORCEINLINE void write_word_stacked(uint8_t *pMsb, uint8_t *pLsb, int x, uint16_t value) {
    pMsb[x] = value >> 8;
    pLsb[x] = value & 0xFF;
}

}

#endif // __Mt_16bit_H__
