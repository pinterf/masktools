#ifndef __Mt_SIMD_AVX2_H__
#define __Mt_SIMD_AVX2_H__

#include "simd.h"
#include "simd_avx.h"

#pragma warning(default: 4512 4244 4100)
#include <avs/config.h>

// experimental simd includes for avx2 compiled files
#if defined (__GNUC__) && ! defined (__INTEL_COMPILER)
#include <x86intrin.h>
// x86intrin.h includes header files for whatever instruction
// sets are specified on the compiler command line, such as: xopintrin.h, fma4intrin.h
#else
#include <immintrin.h> // MS version of immintrin.h covers AVX, AVX2 and FMA3
#endif // __GNUC__

#if !defined(__FMA__)
// Assume that all processors that have AVX2 also have FMA3
#if defined (__GNUC__) && ! defined (__INTEL_COMPILER) && ! defined (__clang__)
// Prevent error message in g++ when using FMA intrinsics with avx2:
#pragma message "It is recommended to specify also option -mfma when using -mavx2 or higher"
#else
#define __FMA__  1
#endif
#endif
// FMA3 instruction set
#if defined (__FMA__) && (defined(__GNUC__) || defined(__clang__))  && ! defined (__INTEL_COMPILER)
#include <fmaintrin.h>
#endif // __FMA__ 

// define some simd macros which work on msvc but not in gcc

#ifndef _mm256_cvtsi256_si32
// int _mm256_cvtsi256_si32 (__m256i a)
#define _mm256_cvtsi256_si32(a) (_mm_cvtsi128_si32(_mm256_castsi256_si128(a)))
#endif

#ifndef _mm256_loadu2_m128i
#define _mm256_loadu2_m128i(/* __m128i const* */ hiaddr, \
                            /* __m128i const* */ loaddr) \
    _mm256_set_m128i(_mm_loadu_si128(hiaddr), _mm_loadu_si128(loaddr))
#endif

#ifndef _mm256_loadu2_m128
#define _mm256_loadu2_m128(/* float const* */ hiaddr, \
                           /* float const* */ loaddr) \
    _mm256_set_m128(_mm_loadu_ps(hiaddr), _mm_loadu_ps(loaddr))
#endif


#include "common.h"

namespace Filtering {
// Anonymous namespace: this header is included from files built with different per file compiler flags.
namespace {

#define USE_MOVPS

template<MemoryMode mem_mode, typename T>
#if defined(GCC) || defined(CLANG)
__attribute__((__target__("avx2")))
#endif
static MT_FORCEINLINE __m256i simd256_load_si256(const T* ptr) {
#ifdef USE_MOVPS
  if constexpr (mem_mode == MemoryMode::SSE2_ALIGNED) {
    return _mm256_castps_si256(_mm256_load_ps(reinterpret_cast<const float*>(ptr)));
}
  else {
    return _mm256_castps_si256(_mm256_loadu_ps(reinterpret_cast<const float*>(ptr)));
  }
#else
  if constexpr (mem_mode == MemoryMode::SSE2_ALIGNED) {
    return _mm256_load_si256(reinterpret_cast<const __m256i*>(ptr));
  }
  else {
    return _mm256_loadu_si256(reinterpret_cast<const __m256i*>(ptr));
  }
#endif
}


template<Border border_mode, MemoryMode mem_mode>
#if defined(GCC) || defined(CLANG)
__attribute__((__target__("avx2")))
#endif
static MT_FORCEINLINE __m256i load_one_to_left_si256(const uint8_t *ptr) {
  if constexpr (border_mode == Border::Left) {
    auto lo128 = load_one_to_left<border_mode, mem_mode>(ptr); // really left!
    auto hi128 = simd_load_si128<MemoryMode::SSE2_UNALIGNED>(ptr + 16 - 1);
    return _mm256_set_m128i(hi128, lo128);
  }
  else {
    return simd256_load_si256<MemoryMode::SSE2_UNALIGNED>(ptr - 1);
  }
}

template<Border border_mode, MemoryMode mem_mode>
#if defined(GCC) || defined(CLANG)
__attribute__((__target__("avx2")))
#endif
static MT_FORCEINLINE __m256i load_one_to_right_si256(const uint8_t *ptr) {
  if constexpr (border_mode == Border::Right) {
    auto lo128 = simd_load_si128<MemoryMode::SSE2_UNALIGNED>(ptr + 1);
    auto hi128 = load_one_to_right<border_mode, mem_mode>(ptr + 16); // really right!
    return _mm256_set_m128i(hi128, lo128);
  }
  else {
    return simd256_load_si256<MemoryMode::SSE2_UNALIGNED>(ptr + 1);
  }
}

template<Border border_mode, MemoryMode mem_mode>
#if defined(GCC) || defined(CLANG)
__attribute__((__target__("avx2")))
#endif
static MT_FORCEINLINE __m256i load16_one_to_left_si256(const uint8_t* ptr) {
  if constexpr (border_mode == Border::Left) {
    auto lo128 = load16_one_to_left<border_mode, mem_mode>(ptr); // really left!
    auto hi128 = simd_load_si128<MemoryMode::SSE2_UNALIGNED>(ptr + 16 - 2);
    return _mm256_set_m128i(hi128, lo128);
  }
  else {
    return simd256_load_si256<MemoryMode::SSE2_UNALIGNED>(ptr - 2);
  }
}

template<Border border_mode, MemoryMode mem_mode>
#if defined(GCC) || defined(CLANG)
__attribute__((__target__("avx2")))
#endif
static MT_FORCEINLINE __m256i load16_one_to_right_si256(const uint8_t* ptr) {
  if constexpr (border_mode == Border::Right) {
    auto lo128 = simd_load_si128<MemoryMode::SSE2_UNALIGNED>(ptr + 2);
    auto hi128 = load16_one_to_right<border_mode, mem_mode>(ptr + 16); // really right!
    return _mm256_set_m128i(hi128, lo128);
  }
  else {
    return simd256_load_si256<MemoryMode::SSE2_UNALIGNED>(ptr + 2);
  }
}


#if defined(GCC) || defined(CLANG)
__attribute__((__target__("avx2")))
#endif
static MT_FORCEINLINE __m256i simd256_blend_epi8(const __m256i &selector, const __m256i &a, const __m256i &b) {
  return _mm256_blendv_epi8(b, a, selector);
}


#if defined(GCC) || defined(CLANG)
__attribute__((__target__("avx2")))
#endif
static MT_FORCEINLINE __m256i simd256_blendv_epi8(__m256i x, __m256i y, __m256i mask)
{
  return _mm256_blendv_epi8(x, y, mask);
}

#if defined(GCC) || defined(CLANG)
__attribute__((__target__("avx2")))
#endif
static MT_FORCEINLINE __m256i threshold_avx2(const __m256i &value, const __m256i &lowThresh, const __m256i &highThresh, const __m256i &v128) {
  auto sat = _mm256_sub_epi8(value, v128);
  auto low = _mm256_cmpgt_epi8(sat, lowThresh);
  auto high = _mm256_cmpgt_epi8(sat, highThresh);
  auto result = _mm256_and_si256(value, low);
  return _mm256_or_si256(result, high);
}

//  thresholds are decreased by half range in order to do signed comparison
template<int bits_per_pixel>
#if defined(GCC) || defined(CLANG)
__attribute__((__target__("avx2")))
#endif
static MT_FORCEINLINE __m256i threshold16_avx2(const __m256i &value, const __m256i &lowThresh, const __m256i &highThresh, const __m256i &vHalf, const __m256i &maxMask) {
  auto sat = _mm256_sub_epi16(value, vHalf);
  auto low = _mm256_cmpgt_epi16(sat, lowThresh);
  auto high = _mm256_cmpgt_epi16(sat, highThresh);
  if (bits_per_pixel < 16)
    high = _mm256_and_si256(high, maxMask); // clamp FFFF to 03FF 0FFF or 3FFF at 10, 12 and 14 bits
  auto result = _mm256_and_si256(value, low);
  return _mm256_or_si256(result, high);
}

#if defined(GCC) || defined(CLANG)
__attribute__((__target__("avx2")))
#endif
static MT_FORCEINLINE __m256i simd256_mullo_epi32(__m256i &a, __m256i &b) {
  return _mm256_mullo_epi32(a, b);
}


// non-existant in simd
#if defined(GCC) || defined(CLANG)
__attribute__((__target__("avx2")))
#endif
static MT_FORCEINLINE __m256i _MM256_CMPLE_EPU16(__m256i x, __m256i y)
{
  // Returns 0xFFFF where x <= y:
  return _mm256_cmpeq_epi16(_mm256_subs_epu16(x, y), _mm256_setzero_si256());
}

#pragma warning(disable: 4556)
// simulate real 256 bit byte-shift (not 2x128 lanes)
template<BYTE shiftcount>
#if defined(GCC) || defined(CLANG)
__attribute__((__target__("avx2")))
#endif
MT_FORCEINLINE __m256i _MM256_SLLI_SI256(__m256i a)
{
  if constexpr(shiftcount == 0)
    return a;
  else if constexpr (shiftcount < 16)
    return _mm256_alignr_epi8(a, _mm256_permute2x128_si256(a, a, _MM_SHUFFLE(0, 0, 2, 0)), 16 - shiftcount);
  else if constexpr (shiftcount == 16)
    return _mm256_permute2x128_si256(a, a, _MM_SHUFFLE(0, 0, 2, 0));
  else if constexpr (shiftcount < 32)
    return _mm256_slli_si256(_mm256_permute2x128_si256(a, a, _MM_SHUFFLE(0, 0, 2, 0)), shiftcount - 16);
  return _mm256_setzero_si256();
}

// simulate real 256 bit byte-shift (not 2x128 lanes)
template<BYTE shiftcount>
#if defined(GCC) || defined(CLANG)
__attribute__((__target__("avx2")))
#endif
MT_FORCEINLINE __m256i _MM256_SRLI_SI256(__m256i a)
{
  if constexpr (shiftcount == 0)
    return a;
  else if constexpr (shiftcount < 16)
    return _mm256_alignr_epi8(_mm256_permute2x128_si256(a, a, _MM_SHUFFLE(2, 0, 0, 1)), a, shiftcount);
  else if constexpr (shiftcount == 16)
    return _mm256_permute2x128_si256(a, a, _MM_SHUFFLE(2, 0, 0, 1));
  else if constexpr (shiftcount < 32)
    return _mm256_srli_si256(_mm256_permute2x128_si256(a, a, _MM_SHUFFLE(2, 0, 0, 1)), shiftcount - 16);
  return _mm256_setzero_si256();
}
#pragma warning(default: 4556)

} // namespace

}

#endif // __Mt_SIMD_AVX2_H__
