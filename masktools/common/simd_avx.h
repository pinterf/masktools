#ifndef __Mt_SIMD_AVX_H__
#define __Mt_SIMD_AVX_H__

#include "simd.h"

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
__attribute__((__target__("avx")))
#endif
static MT_FORCEINLINE __m256 simd256_load_ps(const T* ptr) {
#ifdef USE_MOVPS
  if constexpr (mem_mode == MemoryMode::SSE2_ALIGNED) {
    return _mm256_load_ps(reinterpret_cast<const float*>(ptr));
  }
  else {
    return _mm256_loadu_ps(reinterpret_cast<const float*>(ptr));
  }
#else
  if constexpr (mem_mode == MemoryMode::SSE2_ALIGNED) {
    return _mm256_castsi256_ps(_mm256_load_si256(reinterpret_cast<const __m256i*>(ptr)));
  }
  else {
    return _mm256_castsi256_ps(_mm256_loadu_si256(reinterpret_cast<const __m256i*>(ptr)));
  }
#endif
}

template<MemoryMode mem_mode, typename T>
#if defined(GCC) || defined(CLANG)
__attribute__((__target__("avx")))
#endif
static MT_FORCEINLINE __m256 simd256_128_load_ps(const T* ptr) {
  if constexpr (mem_mode == MemoryMode::SSE2_ALIGNED) {
    return _mm256_loadu2_m128(reinterpret_cast<const float*>(ptr) + 4, reinterpret_cast<const float*>(ptr));
  }
  else {
    return _mm256_loadu2_m128(reinterpret_cast<const float*>(ptr) + 4, reinterpret_cast<const float*>(ptr));
  }
}

template<MemoryMode mem_mode, typename T>
#if defined(GCC) || defined(CLANG)
__attribute__((__target__("avx")))
#endif
static MT_FORCEINLINE void simd256_store_si256(T *ptr, __m256i value) {
#ifdef USE_MOVPS
  if constexpr (mem_mode == MemoryMode::SSE2_ALIGNED) {
    _mm256_store_ps(reinterpret_cast<float*>(ptr), _mm256_castsi256_ps(value));
  }
  else {
    _mm256_storeu_ps(reinterpret_cast<float*>(ptr), _mm256_castsi256_ps(value));
  }
#else
  if constexpr (mem_mode == MemoryMode::SSE2_ALIGNED) {
    _mm256_store_si256(reinterpret_cast<__m256i*>(ptr), value);
  }
  else {
    _mm256_storeu_si256(reinterpret_cast<__m256i*>(ptr), value);
  }
#endif
}

template<MemoryMode mem_mode, typename T>
#if defined(GCC) || defined(CLANG)
__attribute__((__target__("avx")))
#endif
static MT_FORCEINLINE void simd256_store_ps(T *ptr, __m256 value) {
#ifdef USE_MOVPS
  if constexpr (mem_mode == MemoryMode::SSE2_ALIGNED) {
    _mm256_store_ps(reinterpret_cast<float*>(ptr), value);
  }
  else {
    _mm256_storeu_ps(reinterpret_cast<float*>(ptr), value);
  }
#else
  if (mem_mode == MemoryMode::SSE2_ALIGNED) {
    _mm256_store_si256(reinterpret_cast<__m256i*>(ptr), _mm256_castps_si256(value));
  }
  else {
    _mm256_storeu_si256(reinterpret_cast<__m256i*>(ptr), _mm256_castps_si256(value));
  }
#endif
}

template<Border border_mode, MemoryMode mem_mode>
#if defined(GCC) || defined(CLANG)
__attribute__((__target__("avx")))
#endif
static MT_FORCEINLINE __m256 load32_one_to_left_si256(const uint8_t *ptr) {
  if constexpr (border_mode == Border::Left) {
    auto lo128 = load32_one_to_left<border_mode, mem_mode>(ptr); // really left!
    auto hi128 = simd_load_ps<MemoryMode::SSE2_UNALIGNED>(ptr + 16 - 4);
    return _mm256_set_m128(hi128, lo128);
  }
  else {
    return simd256_load_ps<MemoryMode::SSE2_UNALIGNED>(ptr - 4);
  }
}

template<Border border_mode, MemoryMode mem_mode>
#if defined(GCC) || defined(CLANG)
__attribute__((__target__("avx")))
#endif
static MT_FORCEINLINE __m256 load32_one_to_right_si256(const uint8_t *ptr) {
  if constexpr(border_mode == Border::Right) {
    auto lo128 = simd_load_ps<MemoryMode::SSE2_UNALIGNED>(ptr+4);
    auto hi128 = load32_one_to_right<border_mode, mem_mode>(ptr + 16); // really right!
    return _mm256_set_m128(hi128, lo128);
  }
  else {
    return simd256_load_ps<MemoryMode::SSE2_UNALIGNED>(ptr + 4);
  }
}

#if defined(GCC) || defined(CLANG)
__attribute__((__target__("avx")))
#endif
static MT_FORCEINLINE __m256 simd256_blendv_ps(__m256 x, __m256 y, __m256 mask)
{
  return _mm256_blendv_ps(x, y, mask);
}

#if defined(GCC) || defined(CLANG)
__attribute__((__target__("avx")))
#endif
static MT_FORCEINLINE __m256 _mm256_cmpgt_ps(__m256 a, __m256 b) {
  return _mm256_cmp_ps(a, b, _CMP_NLE_US); // NLE = GT
}

#if defined(GCC) || defined(CLANG)
__attribute__((__target__("avx")))
#endif
static MT_FORCEINLINE __m256 threshold32_avx(const __m256 &value, const __m256 &lowThresh, const __m256 &highThresh) {
  // create final mask 0.0 or 1.0 or x if between
  // value <= low ? 0.0f : value > high ? 1.0 : x
  auto tOne = _mm256_set1_ps(1.0f);
  auto lowMask = _mm256_cmpgt_ps(value, lowThresh);   // (value > lowTh) ? FFFFFFFF : 00000000
  auto tmpValue = _mm256_and_ps(lowMask, value); // 0 where value <= lowTh, value otherwise

  auto highMask = _mm256_cmpgt_ps(tmpValue, highThresh); // value > highTh) ? FFFFFFFF : 00000000
  auto result = simd256_blendv_ps(tmpValue, tOne, highMask);
  return result;
}

#if defined(GCC) || defined(CLANG)
__attribute__((__target__("avx")))
#endif
static MT_FORCEINLINE __m256 simd256_abs_ps(__m256 a) {
  // maybe not optimal, mask may be generated 
  const __m256 absmask = _mm256_castsi256_ps(_mm256_set1_epi32(~(1 << 31))); // 0x7FFFFFFF
  return _mm256_and_ps(a, absmask);
}

#if defined(GCC) || defined(CLANG)
__attribute__((__target__("avx")))
#endif
static MT_FORCEINLINE __m256 simd256_abs_diff_ps(__m256 a, __m256 b) {
  // maybe not optimal
  const __m256 absmask = _mm256_castsi256_ps(_mm256_set1_epi32(~(1 << 31))); // 0x7FFFFFFF
  return _mm256_and_ps(_mm256_sub_ps(a, b), absmask);
}

} // namespace

}

#endif // __Mt_SIMD_AVX_H__
