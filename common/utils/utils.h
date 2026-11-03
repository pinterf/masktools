#ifndef __Common_Utils_H__
#define __Common_Utils_H__

#include <assert.h>
#include <string>
#include <vector>
#include <list>
#include <stdio.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <float.h>
#include <limits>
#include <cstring>
#include <cctype>
#include <cmath>
#include <cstdint>


#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX // no min & max macros
#endif
#include <windows.h>
#undef NOMINMAX
#endif

#ifndef MT_FORCEINLINE
#if defined(__clang__)
// Check clang first. clang-cl also defines __MSC_VER
// We set MSVC because they are mostly compatible
#   define CLANG
#if defined(_MSC_VER)
#   define MSVC
#   define MT_FORCEINLINE __attribute__((always_inline)) inline
#else
#   define MT_FORCEINLINE __attribute__((always_inline)) inline
#endif
#elif   defined(_MSC_VER)
#   define MSVC
#   define MSVC_PURE
#   define MT_FORCEINLINE __forceinline
#elif defined(__GNUC__)
#   define GCC
#   define MT_FORCEINLINE __attribute__((always_inline)) inline
#else
#   define MT_FORCEINLINE inline
#   undef __forceinline
#   define __forceinline inline
#endif 

#endif


// INTEL_INTRINSICS / NEON_INTRINSICS: the x86 / NEON SIMD code is compiled. CMake defines them
// (ENABLE_INTEL_SIMD, ENABLE_NEON_SIMD), the MSVC solution does not: derive them from the target there.
#if defined(_MSC_VER) && !defined(INTEL_INTRINSICS) && !defined(NEON_INTRINSICS)
#if defined(_M_IX86) || defined(_M_X64)
#define INTEL_INTRINSICS
#elif defined(_M_ARM64)
#define NEON_INTRINSICS
#endif
#endif

// MT_X86(x): x only in the x86 builds, e.g. the registration of an SSE2 processor inside a macro
#ifdef INTEL_INTRINSICS
#define MT_X86(...) __VA_ARGS__
#else
#define MT_X86(...)
#endif
// MT_NEON(x): x only in the aarch64 NEON builds
#ifdef NEON_INTRINSICS
#define MT_NEON(...) __VA_ARGS__
#else
#define MT_NEON(...)
#endif

#ifndef _WIN32
#define OutputDebugString(x)
#endif

#if (defined(GCC) || defined(CLANG)) && !defined(_WIN32)
#include <stdlib.h>
#define _aligned_malloc(size, alignment) aligned_alloc(alignment, size)
#define _aligned_free(ptr) free(ptr)
#endif

#ifndef _WIN32
#include <stdio.h>
#ifdef AVS_POSIX
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 1
#endif
#include <limits.h>
#endif
#endif

#pragma warning(disable:4267) // disable possible loss of data conversion
#pragma warning(disable:4127) // disable conditional expression is constant
#pragma warning(disable:4499) // disable warning C4499: 'static': an explicit specialization cannot have a storage class (ignored)

#define UNUSED(x) (void)(x)

namespace Filtering {

typedef std::string String;

/* case insensive == operator for String */

static inline bool operator==(const String &s1, const String &s2)
{
  return s1.size() == s2.size() && std::equal(s1.begin(), s1.end(), s2.begin(), [](auto a, auto b) {return std::tolower(a) == std::tolower(b); });
//     !_strnicmp( s1.c_str(), s2.c_str(), s1.size() );
}


/* log level */
typedef enum {

   LOG_ERROR = 0,
   LOG_WARNING = 1,
   LOG_DEBUG = 2,

} LogLevel;

/* default log level */
#ifdef DEBUG
#define MAX_LOG_LEVEL LOG_DEBUG
#else
#define MAX_LOG_LEVEL LOG_ERROR
#endif

/* debug print function */
static inline void print(LogLevel level, const char *format, ...)
{
   va_list args;
   char buf[1024];

   if ( level > MAX_LOG_LEVEL )
      return;

#ifdef _WIN32
   va_start(args, format);
   vsprintf(buf, format, args);
   OutputDebugString(buf);
#else
   // non-Windows: debug output not implemented
#endif
}

// Anonymous namespace: this header is included from files built with different per file compiler flags.
namespace {

/* min & max */
template<typename T> T min(T a, T b) { return a < b ? a : b; }
template<typename T> T max(T a, T b) { return a > b ? a : b; }

static inline double fix(double a) { return std::isnan(a) || !std::isfinite(a) ? 0 : a; }

/* abs */
template<typename T> static T abs(T x) { return x < 0 ? -x : x; }
template<> inline uint8_t abs<uint8_t>(uint8_t x) { return x; } // unsigned abs = nop
template<> inline uint16_t abs<uint16_t>(uint16_t x) { return x; } // unsigned abs = nop

/* max_value, for integer type */
// for threshold and clip (clamp)
template<typename T> static T max_value() { return T(-1); } // unsigned only, all bits to 1!
template<> inline int max_value<int>() { return 0x7fffffff; }
template<> inline int16_t max_value<int16_t>() { return (1 << 15) - 1; }
template<> inline int8_t max_value<int8_t>() { return (1 << 7) - 1; }
template<> inline int64_t max_value<int64_t>() { return std::numeric_limits<int64_t>::max(); /*0x7FFFFFFFFFFFFFFFLL;*/ }
template<> inline float max_value<float>() { return 1.0f; } // for mask

/* min_value, for integer type */
template<typename T> static T min_value() { return 0; } // unsigned only
template<> inline int16_t min_value<int16_t>() { return -(1 << 15); }
template<> inline int8_t min_value<int8_t>() { return -(1 << 7); }
template<> inline int64_t min_value<int64_t>() { return std::numeric_limits<int64_t>::min(); /*-(1LL << 63);*/ }

/* ceiled & floored round */
template<typename T> static T ceiled(T x, T mod) { return ((x + mod - 1) / mod) * mod; }
template<typename T> static T floored(T x, T mod) { return (x / mod) * mod; }

/* zero_threshold */
template<typename T> static T zero_threshold() { return 1; } // integer types
template<> inline float zero_threshold<float>() { return (float)0.00001; } 
template<> inline double zero_threshold<double>() { return 0.00001; } 

/* conversion from and to */
template<typename To, typename From> static To convert(From x) { return To(x); }
template<> inline int8_t convert<int8_t, double>(double x) { return x >= 0 ? int8_t(x + 0.5) : int8_t(x - 0.5); }
template<> inline uint8_t convert<uint8_t, double>(double x) { return uint8_t(x + 0.5); }
template<> inline int64_t convert<int64_t, double>(double x) { return x >= 0 ? int64_t(x + 0.5) : int64_t(x - 0.5); }
template<> inline uint64_t convert<uint64_t, double>(double x) { return uint64_t(x + 0.5); }
template<> inline int16_t convert<int16_t, double>(double x) { return x >= 0 ? int16_t(x + 0.5) : int16_t(x - 0.5); }
template<> inline uint16_t convert<uint16_t, double>(double x) { return uint16_t(x + 0.5); }
template<> inline int convert<int, double>(double x) { return x >= 0 ? int(x + 0.5) : int(x - 0.5); }

template<> inline int8_t convert<int8_t, float>(float x) { return x >= 0 ? int8_t(x + 0.5f) : int8_t(x - 0.5f); }
template<> inline uint8_t convert<uint8_t, float>(float x) { return uint8_t(x + 0.5f); }
template<> inline int64_t convert<int64_t, float>(float x) { return x >= 0 ? int64_t(x + 0.5f) : int64_t(x - 0.5f); }
template<> inline uint64_t convert<uint64_t, float>(float x) { return uint64_t(x + 0.5f); }
template<> inline int16_t convert<int16_t, float>(float x) { return x >= 0 ? int16_t(x + 0.5f) : int16_t(x - 0.5f); }
template<> inline uint16_t convert<uint16_t, float>(float x) { return uint16_t(x + 0.5f); }
template<> inline int convert<int, float>(float x) { return x >= 0 ? int(x + 0.5f) : int(x - 0.5f); }

/* rounded division */
template<typename T> static T rounded_division(T x, T y) { return x / y; }
template<> inline int rounded_division<int>(int x, int y) { return x > 0 ? (x + (y >> 1)) / y : (x - (y >> 1)) / y; }
template<> inline int64_t rounded_division<int64_t>(int64_t x, int64_t y) { return x > 0 ? (x + (y >> 1)) / y : (x - (y >> 1)) / y; }

/* clip */
template<typename T, typename U> T clip(U x, U mini = U(min_value<T>()), U maxi = U(max_value<T>())) { return convert<T, U>( min<U>( maxi, max<U>( mini, x ) ) ); }

/* threshold for creating mask */
template<typename T, typename U> static T threshold(U x, U mini, U maxi, U bottom = U(min_value<T>()), U top = U(max_value<T>()))
{
   return convert<T, U>( x <= mini ? bottom : x > maxi ? top : x );
}
// specialize template
template<> inline float threshold<float, float>(float x, float mini, float maxi, float bottom, float top)
{ // todo: proper non-clamping for float?
  bottom = float(0.0f);
  top = float(1.0f);
  return convert<float, float>(x <= mini ? bottom : x > maxi ? top : x);
}

// top: the max pixel value of the bit depth
static inline uint16_t threshold16(int x, int mini, int maxi, int top)
{
  return convert<uint16_t, int>(x <= mini ? 0 : x > maxi ? top : x);
}

} // namespace

} // namespace Filtering

#endif
