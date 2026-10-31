#ifndef __Mt_SIMD_H__
#define __Mt_SIMD_H__

#include <emmintrin.h>
#include <smmintrin.h>

#if defined (__GNUC__) && ! defined (__INTEL_COMPILER)
#include <x86intrin.h>
// x86intrin.h includes header files for whatever instruction
// sets are specified on the compiler command line, such as: xopintrin.h, fma4intrin.h
#else
#endif // __GNUC__

#include "common.h"

namespace Filtering {
// Anonymous namespace: this header is included from files built with different per file compiler flags.
namespace {

#define USE_MOVPS

  template<MemoryMode mem_mode, typename T>
  static MT_FORCEINLINE __m128i simd_load_si128(const T* ptr) {
#ifdef USE_MOVPS
    if constexpr (mem_mode == MemoryMode::SSE2_ALIGNED) {
      return _mm_castps_si128(_mm_load_ps(reinterpret_cast<const float*>(ptr)));
    }
    else {
      return _mm_castps_si128(_mm_loadu_ps(reinterpret_cast<const float*>(ptr)));
    }
#else
    if constexpr (mem_mode == MemoryMode::SSE2_ALIGNED) {
      return _mm_load_si128(reinterpret_cast<const __m128i*>(ptr));
    }
    else {
      return _mm_loadu_si128(reinterpret_cast<const __m128i*>(ptr));
    }
#endif
  }

  template<MemoryMode mem_mode, typename T>
  static MT_FORCEINLINE __m128 simd_load_ps(const T* ptr) {
#ifdef USE_MOVPS
    if constexpr (mem_mode == MemoryMode::SSE2_ALIGNED) {
      return _mm_load_ps(reinterpret_cast<const float*>(ptr));
    }
    else {
      return _mm_loadu_ps(reinterpret_cast<const float*>(ptr));
    }
#else
    if constexpr (mem_mode == MemoryMode::SSE2_ALIGNED) {
      return _mm_castsi128_ps(_mm_load_si128(reinterpret_cast<const __m128i*>(ptr)));
    }
    else {
      return _mm_castsi128_ps(_mm_loadu_si128(reinterpret_cast<const __m128i*>(ptr)));
    }
#endif
  }


  template<MemoryMode mem_mode, typename T>
  static MT_FORCEINLINE void simd_store_si128(T* ptr, __m128i value) {
#ifdef USE_MOVPS
    if constexpr (mem_mode == MemoryMode::SSE2_ALIGNED) {
      _mm_store_ps(reinterpret_cast<float*>(ptr), _mm_castsi128_ps(value));
    }
    else {
      _mm_storeu_ps(reinterpret_cast<float*>(ptr), _mm_castsi128_ps(value));
    }
#else
    if constexpr (mem_mode == MemoryMode::SSE2_ALIGNED) {
      _mm_store_si128(reinterpret_cast<__m128i*>(ptr), value);
    }
    else {
      _mm_storeu_si128(reinterpret_cast<__m128i*>(ptr), value);
    }
#endif
  }

  template<MemoryMode mem_mode, typename T>
  static MT_FORCEINLINE void simd_store_ps(T* ptr, __m128 value) {
#ifdef USE_MOVPS
    if constexpr (mem_mode == MemoryMode::SSE2_ALIGNED) {
      _mm_store_ps(reinterpret_cast<float*>(ptr), value);
    }
    else {
      _mm_storeu_ps(reinterpret_cast<float*>(ptr), value);
    }
#else
    if constexpr (mem_mode == MemoryMode::SSE2_ALIGNED) {
      _mm_store_si128(reinterpret_cast<__m128i*>(ptr), _mm_castps_si128(value));
    }
    else {
      _mm_storeu_si128(reinterpret_cast<__m128i*>(ptr), _mm_castps_si128(value));
    }
#endif
  }

  static MT_FORCEINLINE int simd_bit_scan_forward(int value) {
#ifdef __INTEL_COMPILER
    return _bit_scan_forward(value);
#elif __GNUC__
    // assume: value contains at least 1 bits set to 1
    auto index = __builtin_ffsll(value);
    return index - 1; // return the real position, ignore 'not found' case when result is 0
    // Built-in Function : int __builtin_ffs(int x)
    // Returns one plus the index of the least significant 1 - bit of x, or if x is zero, returns zero.
#else
    unsigned long index;
    _BitScanForward(&index, value);
    /*
    If a set bit is found, the bit position of the first set bit found is returned in the first parameter.
    If no set bit is found, 0 is returned; otherwise, 1 is returned.
    */
    return index;

#endif
  }



#pragma warning(disable: 4309)

  template<Border border_mode, MemoryMode mem_mode>
  static MT_FORCEINLINE __m128i load_one_to_left(const uint8_t* ptr) {
    if constexpr (border_mode == Border::Left) {
      auto mask_left = _mm_setr_epi8(0xFF, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00);
      auto val = simd_load_si128<mem_mode>(ptr);
      return _mm_or_si128(_mm_slli_si128(val, 1), _mm_and_si128(val, mask_left)); // clone leftmost
    }
    else {
      return simd_load_si128<MemoryMode::SSE2_UNALIGNED>(ptr - 1);
    }
  }

  template<Border border_mode, MemoryMode mem_mode>
  static MT_FORCEINLINE __m128i load_one_to_right(const uint8_t* ptr) {
    if constexpr (border_mode == Border::Right) {
      auto mask_right = _mm_setr_epi8(00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 0xFF);
      auto val = simd_load_si128<mem_mode>(ptr);
      return _mm_or_si128(_mm_srli_si128(val, 1), _mm_and_si128(val, mask_right));
    }
    else {
      return simd_load_si128<MemoryMode::SSE2_UNALIGNED>(ptr + 1);
    }
  }


  template<Border border_mode, MemoryMode mem_mode>
  static MT_FORCEINLINE __m128i load16_one_to_left(const uint8_t* ptr) {
    if constexpr (border_mode == Border::Left) {
      auto mask_left = _mm_setr_epi8(0xFF, 0xFF, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00);
      auto val = simd_load_si128<mem_mode>(ptr);
      return _mm_or_si128(_mm_slli_si128(val, 2), _mm_and_si128(val, mask_left));
    }
    else {
      return simd_load_si128<MemoryMode::SSE2_UNALIGNED>(ptr - 2);
    }
  }

  template<Border border_mode, MemoryMode mem_mode>
  static MT_FORCEINLINE __m128i load16_one_to_right(const uint8_t* ptr) {
    if constexpr (border_mode == Border::Right) {
      auto mask_right = _mm_setr_epi8(00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 0xFF, 0xFF);
      auto val = simd_load_si128<mem_mode>(ptr);
      return _mm_or_si128(_mm_srli_si128(val, 2), _mm_and_si128(val, mask_right));
    }
    else {
      return simd_load_si128<MemoryMode::SSE2_UNALIGNED>(ptr + 2);
    }
  }



  template<Border border_mode, MemoryMode mem_mode>
  static MT_FORCEINLINE __m128 load32_one_to_left(const uint8_t* ptr) {
    if constexpr (border_mode == Border::Left) {
      auto mask_left = _mm_setr_epi8(0xFF, 0xFF, 0xFF, 0xFF, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00);
      auto val = simd_load_si128<mem_mode>(ptr);
      return _mm_castsi128_ps(_mm_or_si128(_mm_slli_si128(val, 4), _mm_and_si128(val, mask_left)));
    }
    else {
      return simd_load_ps<MemoryMode::SSE2_UNALIGNED>(ptr - 4);
    }
  }

  template<Border border_mode, MemoryMode mem_mode>
  static MT_FORCEINLINE __m128 load32_one_to_right(const uint8_t* ptr) {
    if constexpr (border_mode == Border::Right) {
      auto mask_right = _mm_setr_epi8(00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 0xFF, 0xFF, 0xFF, 0xFF);
      auto val = simd_load_si128<mem_mode>(ptr);
      return _mm_castsi128_ps(_mm_or_si128(_mm_srli_si128(val, 4), _mm_and_si128(val, mask_right)));
    }
    else {
      return simd_load_ps<MemoryMode::SSE2_UNALIGNED>(ptr + 4);
    }
  }


#pragma warning(default: 4309)

  static MT_FORCEINLINE __m128i simd_movehl_si128(const __m128i& a, const __m128i& b) {
    return _mm_castps_si128(_mm_movehl_ps(_mm_castsi128_ps(a), _mm_castsi128_ps(b)));
  }

  template<CpuFlags flags>
  static MT_FORCEINLINE __m128i simd_blend_epi8(__m128i const& selector, __m128i const& a, __m128i const& b) {
    if constexpr (flags >= CPU_SSE4_1) {
      return _mm_blendv_epi8(b, a, selector);
    }
    else {
      return _mm_or_si128(_mm_and_si128(selector, a), _mm_andnot_si128(selector, b));
    }
  }


  // another blendv, good param order
  template<CpuFlags flags>
  static MT_FORCEINLINE __m128i simd_blendv_epi8(__m128i x, __m128i y, __m128i mask)
  {
    if constexpr (flags >= CPU_SSE4_1) {
      return _mm_blendv_epi8(x, y, mask);
    }
    else {
      // Replace bit in x with bit in y when matching bit in mask is set:
      return _mm_or_si128(_mm_andnot_si128(mask, x), _mm_and_si128(mask, y));
    }
  }

  template<CpuFlags flags>
  static MT_FORCEINLINE __m128 simd_blendv_ps(__m128 x, __m128 y, __m128 mask)
  {
    if constexpr (flags >= CPU_SSE4_1) {
      return _mm_blendv_ps(x, y, mask);
    }
    else {
      // Replace bit in x with bit in y when matching bit in mask is set:
      return _mm_or_ps(_mm_andnot_ps(mask, x), _mm_and_ps(mask, y));
    }
  }

  static MT_FORCEINLINE __m128i threshold_sse2(const __m128i& value, const __m128i& lowThresh, const __m128i& highThresh, const __m128i& v128) {
    auto sat = _mm_sub_epi8(value, v128);
    auto low = _mm_cmpgt_epi8(sat, lowThresh);
    auto high = _mm_cmpgt_epi8(sat, highThresh);
    auto result = _mm_and_si128(value, low);
    return _mm_or_si128(result, high);
  }


  //  thresholds are decreased by half range in order to do signed comparison
  template<int bits_per_pixel>
  static MT_FORCEINLINE __m128i threshold16_sse2(const __m128i& value, const __m128i& lowThresh, const __m128i& highThresh, const __m128i& vHalf, const __m128i& maxMask) {
    auto sat = _mm_sub_epi16(value, vHalf);
    auto low = _mm_cmpgt_epi16(sat, lowThresh);
    auto high = _mm_cmpgt_epi16(sat, highThresh);
    if constexpr (bits_per_pixel < 16)
      high = _mm_and_si128(high, maxMask); // clamp FFFF to 03FF 0FFF or 3FFF at 10, 12 and 14 bits
    auto result = _mm_and_si128(value, low);
    return _mm_or_si128(result, high);
  }


  template<CpuFlags flags>
  static MT_FORCEINLINE __m128 threshold32_sse2(const __m128& value, const __m128& lowThresh, const __m128& highThresh) {
    // create final mask 0.0 or 1.0 or x if between
    // value <= low ? 0.0f : value > high ? 1.0 : x
    auto tOne = _mm_set1_ps(1.0f);
    auto lowMask = _mm_cmpgt_ps(value, lowThresh);   // (value > lowTh) ? FFFFFFFF : 00000000
    auto tmpValue = _mm_and_ps(lowMask, value); // 0 where value <= lowTh, value otherwise

    auto highMask = _mm_cmpgt_ps(tmpValue, highThresh); // value > highTh) ? FFFFFFFF : 00000000
    auto result = simd_blendv_ps<flags>(tmpValue, tOne, highMask);
    return result;
  }


  template<CpuFlags flags>
  static MT_FORCEINLINE __m128i simd_mullo_epi32(__m128i& a, __m128i& b) {
    if constexpr (flags >= CPU_SSE4_1) {
      return _mm_mullo_epi32(a, b);
    }
    else {
      auto a13 = _mm_shuffle_epi32(a, 0xF5);          // (-,a3,-,a1)
      auto b13 = _mm_shuffle_epi32(b, 0xF5);          // (-,b3,-,b1)
      auto prod02 = _mm_mul_epu32(a, b);                 // (-,a2*b2,-,a0*b0)
      auto prod13 = _mm_mul_epu32(a13, b13);             // (-,a3*b3,-,a1*b1)
      auto prod01 = _mm_unpacklo_epi32(prod02, prod13);   // (-,-,a1*b1,a0*b0) 
      auto prod23 = _mm_unpackhi_epi32(prod02, prod13);   // (-,-,a3*b3,a2*b2) 
      return _mm_unpacklo_epi64(prod01, prod23);   // (ab3,ab2,ab1,ab0)
    }
  }


  // sse2 replacement of _mm_mullo_epi32 in SSE4.1
  // another way for do mullo for SSE2, actually not used, there is simd_mullo_epi32
  // use it after speed test, may have too much overhead and C is faster
  static MT_FORCEINLINE __m128i _MM_MULLO_EPI32(const __m128i& a, const __m128i& b)
  {
    // for SSE 4.1: return _mm_mullo_epi32(a, b);
    __m128i tmp1 = _mm_mul_epu32(a, b); // mul 2,0
    __m128i tmp2 = _mm_mul_epu32(_mm_srli_si128(a, 4), _mm_srli_si128(b, 4)); // mul 3,1
                                                                              // shuffle results to [63..0] and pack. a2->a1, a0->a0
    return _mm_unpacklo_epi32(_mm_shuffle_epi32(tmp1, _MM_SHUFFLE(0, 0, 2, 0)), _mm_shuffle_epi32(tmp2, _MM_SHUFFLE(0, 0, 2, 0)));
  }

#pragma warning(disable: 4309)
  // fake _mm_packus_epi32 (orig is SSE4.1 only)
  static MT_FORCEINLINE __m128i _MM_PACKUS_EPI32(__m128i a, __m128i b)
  {

    const __m128i val_32 = _mm_set1_epi32(0x8000);
    const __m128i val_16 = _mm_set1_epi16(0x8000);

    a = _mm_sub_epi32(a, val_32);
    b = _mm_sub_epi32(b, val_32);
    a = _mm_packs_epi32(a, b);
    a = _mm_add_epi16(a, val_16);
    return a;
  }
#pragma warning(default: 4309)


  // non-existant in simd
  static MT_FORCEINLINE __m128i _MM_CMPLE_EPU16(__m128i x, __m128i y)
  {
    // Returns 0xFFFF where x <= y:
    return _mm_cmpeq_epi16(_mm_subs_epu16(x, y), _mm_setzero_si128());
  }

  // SSE2 version of SSE4.1-only _mm_max_epu16
  template<CpuFlags flags>
  static MT_FORCEINLINE __m128i simd_max_epu16(__m128i x, __m128i y)
  {
    if constexpr (flags >= CPU_SSE4_1) {
      return _mm_max_epu16(x, y);
    }
    else {
      return simd_blendv_epi8<flags>(x, y, _MM_CMPLE_EPU16(x, y)); // blendv (not blend) watch param order !
    }
  }

  // SSE2 version of SSE4.1-only _mm_min_epu16
  template<CpuFlags flags>
  static MT_FORCEINLINE __m128i simd_min_epu16(__m128i x, __m128i y)
  {
    if constexpr (flags >= CPU_SSE4_1) {
      return _mm_min_epu16(x, y);
    }
    else {
      return simd_blendv_epi8<flags>(y, x, _MM_CMPLE_EPU16(x, y)); // blendv (not blend) watch param order !
    }
  }

  template<CpuFlags flags>
  static MT_FORCEINLINE __m128i simd_packus_epi32(__m128i& a, __m128i& b) {
    if constexpr (flags >= CPU_SSE4_1) {
      return _mm_packus_epi32(a, b);
    }
    else {
      return _MM_PACKUS_EPI32(a, b);
    }
  }

  // SSE2 version of the SSE3 _mm_hadd_ps: (a0+a1, a2+a3, b0+b1, b2+b3), the same sums
  static MT_FORCEINLINE __m128 simd_hadd_ps(__m128 a, __m128 b) {
    return _mm_add_ps(_mm_shuffle_ps(a, b, _MM_SHUFFLE(2, 0, 2, 0)), _mm_shuffle_ps(a, b, _MM_SHUFFLE(3, 1, 3, 1)));
  }

  static MT_FORCEINLINE __m128 simd_abs_ps(__m128 a) {
    // maybe not optimal, mask may be generated 
    const __m128 absmask = _mm_castsi128_ps(_mm_set1_epi32(~(1 << 31))); // 0x7FFFFFFF
    return _mm_and_ps(a, absmask);
  }

  static MT_FORCEINLINE __m128 simd_abs_diff_ps(__m128 a, __m128 b) {
    // maybe not optimal
    const __m128 absmask = _mm_castsi128_ps(_mm_set1_epi32(~(1 << 31))); // 0x7FFFFFFF
    return _mm_and_ps(_mm_sub_ps(a, b), absmask);
  }

  static MT_FORCEINLINE __m128i read_word_stacked_simd(const uint8_t* pMsb, const uint8_t* pLsb, int x) {
    auto msb = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(pMsb + x));
    auto lsb = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(pLsb + x));
    return _mm_unpacklo_epi8(lsb, msb);
  }

  static MT_FORCEINLINE void write_word_stacked_simd(uint8_t* pMsb, uint8_t* pLsb, int x, const __m128i& value, const __m128i& ff, const __m128i& zero) {
    auto result_lsb = _mm_and_si128(value, ff);
    auto result_msb = _mm_srli_epi16(value, 8);

    result_lsb = _mm_packus_epi16(result_lsb, zero);
    result_msb = _mm_packus_epi16(result_msb, zero);

    _mm_storel_epi64(reinterpret_cast<__m128i*>(pMsb + x), result_msb);
    _mm_storel_epi64(reinterpret_cast<__m128i*>(pLsb + x), result_lsb);
  }
#pragma warning(default: 4556)
} // namespace

}

#endif // __Mt_SIMD_H__
