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

/* basic types definition : Byte, Word, Float, Double */
typedef char Char;
typedef short Short;
typedef unsigned char Byte;
typedef unsigned short Word;
typedef int64_t Int64; // the same type as int64_t: the Int64 specializations apply to int64_t as well (Linux: long)
typedef uint64_t Uint64;
typedef float Float;
typedef double Double;
typedef std::string String;

/* case insensive == operator for String */

static inline bool operator==(const String &s1, const String &s2)
{
  return s1.size() == s2.size() && std::equal(s1.begin(), s1.end(), s2.begin(), [](auto a, auto b) {return std::tolower(a) == std::tolower(b); });
//     !_strnicmp( s1.c_str(), s2.c_str(), s1.size() );
}


/* colorspaces, planar */
typedef enum {

   COLORSPACE_NONE = 0,

   COLORSPACE_Y8,

   COLORSPACE_YV12,
   COLORSPACE_I420,
   COLORSPACE_YV16,
   COLORSPACE_I422,
   COLORSPACE_YV24,
   COLORSPACE_I444,

   COLORSPACE_YV411,

   COLORSPACE_Y10,
   COLORSPACE_YUV420P10,
   COLORSPACE_YUV422P10,
   COLORSPACE_YUV444P10,

   COLORSPACE_Y12,
   COLORSPACE_YUV420P12,
   COLORSPACE_YUV422P12,
   COLORSPACE_YUV444P12,

   COLORSPACE_Y14,
   COLORSPACE_YUV420P14,
   COLORSPACE_YUV422P14,
   COLORSPACE_YUV444P14,

   COLORSPACE_Y16,
   COLORSPACE_YUV420P16,
   COLORSPACE_YUV422P16,
   COLORSPACE_YUV444P16,

   COLORSPACE_Y32,
   COLORSPACE_YUV420PS,
   COLORSPACE_YUV422PS,
   COLORSPACE_YUV444PS,

   COLORSPACE_RGBP8,
   COLORSPACE_RGBP10,
   COLORSPACE_RGBP12,
   COLORSPACE_RGBP14,
   COLORSPACE_RGBP16,
   COLORSPACE_RGBPS,

   // with alpha channels
   COLORSPACE_YUVA420P8,
   COLORSPACE_YUVA422P8,
   COLORSPACE_YUVA444P8,

   COLORSPACE_YUVA420P10,
   COLORSPACE_YUVA422P10,
   COLORSPACE_YUVA444P10,

   COLORSPACE_YUVA420P12,
   COLORSPACE_YUVA422P12,
   COLORSPACE_YUVA444P12,

   COLORSPACE_YUVA420P14,
   COLORSPACE_YUVA422P14,
   COLORSPACE_YUVA444P14,

   COLORSPACE_YUVA420P16,
   COLORSPACE_YUVA422P16,
   COLORSPACE_YUVA444P16,

   COLORSPACE_YUVA420PS,
   COLORSPACE_YUVA422PS,
   COLORSPACE_YUVA444PS,

   COLORSPACE_RGBAP8,
   COLORSPACE_RGBAP10,
   COLORSPACE_RGBAP12,
   COLORSPACE_RGBAP14,
   COLORSPACE_RGBAP16,
   COLORSPACE_RGBAPS,

   COLORSPACE_COUNT,

} Colorspace;

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

/* min & max */
template<typename T> T min(T a, T b) { return a < b ? a : b; }
template<typename T> T max(T a, T b) { return a > b ? a : b; }

static inline double fix(double a) { return std::isnan(a) || !std::isfinite(a) ? 0 : a; }

/* abs */
template<typename T> static T abs(T x) { return x < 0 ? -x : x; }
template<> inline Byte abs<Byte>(Byte x) { return x; } // unsigned abs = nop
template<> inline Word abs<Word>(Word x) { return x; } // unsigned abs = nop

/* max_value, for integer type */
// for threshold and clip (clamp)
template<typename T> static T max_value() { return T(-1); } // unsigned only, all bits to 1!
template<> inline int max_value<int>() { return 0x7fffffff; }
template<> inline Short max_value<Short>() { return (1 << 15) - 1; }
template<> inline Char max_value<Char>() { return (1 << 7) - 1; }
template<> inline Int64 max_value<Int64>() { return std::numeric_limits<int64_t>::max(); /*0x7FFFFFFFFFFFFFFFLL;*/ }
template<> inline Float max_value<Float>() { return 1.0f; } // for mask

/* min_value, for integer type */
template<typename T> static T min_value() { return 0; } // unsigned only
template<> inline Short min_value<Short>() { return -(1 << 15); }
template<> inline Char min_value<Char>() { return -(1 << 7); }
template<> inline Int64 min_value<Int64>() { return std::numeric_limits<int64_t>::min(); /*-(1LL << 63);*/ }

/* ceiled & floored round */
template<typename T> static T ceiled(T x, T mod) { return ((x + mod - 1) / mod) * mod; }
template<typename T> static T floored(T x, T mod) { return (x / mod) * mod; }

/* zero_threshold */
template<typename T> static T zero_threshold() { return 1; } // integer types
template<> inline Float zero_threshold<Float>() { return (Float)0.00001; } 
template<> inline Double zero_threshold<Double>() { return 0.00001; } 

/* conversion from and to */
template<typename To, typename From> static To convert(From x) { return To(x); }
template<> inline Char convert<Char, Double>(Double x) { return x >= 0 ? Char(x + 0.5) : Char(x - 0.5); }
template<> inline Byte convert<Byte, Double>(Double x) { return Byte(x + 0.5); }
template<> inline Int64 convert<Int64, Double>(Double x) { return x >= 0 ? Int64(x + 0.5) : Int64(x - 0.5); }
template<> inline Uint64 convert<Uint64, Double>(Double x) { return Uint64(x + 0.5); }
template<> inline Short convert<Short, Double>(Double x) { return x >= 0 ? Short(x + 0.5) : Short(x - 0.5); }
template<> inline Word convert<Word, Double>(Double x) { return Word(x + 0.5); }
template<> inline int convert<int, Double>(Double x) { return x >= 0 ? int(x + 0.5) : int(x - 0.5); }

template<> inline Char convert<Char, Float>(Float x) { return x >= 0 ? Char(x + 0.5f) : Char(x - 0.5f); }
template<> inline Byte convert<Byte, Float>(Float x) { return Byte(x + 0.5f); }
template<> inline Int64 convert<Int64, Float>(Float x) { return x >= 0 ? Int64(x + 0.5f) : Int64(x - 0.5f); }
template<> inline Uint64 convert<Uint64, Float>(Float x) { return Uint64(x + 0.5f); }
template<> inline Short convert<Short, Float>(Float x) { return x >= 0 ? Short(x + 0.5f) : Short(x - 0.5f); }
template<> inline Word convert<Word, Float>(Float x) { return Word(x + 0.5f); }
template<> inline int convert<int, Float>(Float x) { return x >= 0 ? int(x + 0.5f) : int(x - 0.5f); }

/* rounded division */
template<typename T> static T rounded_division(T x, T y) { return x / y; }
template<> inline int rounded_division<int>(int x, int y) { return x > 0 ? (x + (y >> 1)) / y : (x - (y >> 1)) / y; }
template<> inline Int64 rounded_division<Int64>(Int64 x, Int64 y) { return x > 0 ? (x + (y >> 1)) / y : (x - (y >> 1)) / y; }

/* clip */
template<typename T, typename U> T clip(U x, U mini = U(min_value<T>()), U maxi = U(max_value<T>())) { return convert<T, U>( min<U>( maxi, max<U>( mini, x ) ) ); }

/* threshold for creating mask */
template<typename T, typename U> static T threshold(U x, U mini, U maxi, U bottom = U(min_value<T>()), U top = U(max_value<T>()))
{
   return convert<T, U>( x <= mini ? bottom : x > maxi ? top : x );
}
// specialize template
template<> inline Float threshold<Float, Float>(Float x, Float mini, Float maxi, Float bottom, Float top)
{ // todo: proper non-clamping for float?
  bottom = Float(0.0f);
  top = Float(1.0f);
  return convert<Float, Float>(x <= mini ? bottom : x > maxi ? top : x);
}

template<int bits_per_pixel> static inline Word threshold16(int x, int mini, int maxi)
{ 
  int bottom = 0;
  int top = (1 << bits_per_pixel) - 1;
  return convert<Word, int>(x <= mini ? bottom : x > maxi ? top : x);
}

/* width & height ratios, according to the colorspace */
static const int plane_counts[COLORSPACE_COUNT] = { 0, 
1, 3, 3, 3, 3, 3, 3, 
3, // 411
1, 3, 3, 3, // 10 bits
1, 3, 3, 3, // 12 bits
1, 3, 3, 3, // 14 bits
1, 3, 3, 3, // 16 bits
1, 3, 3, 3, // 32 bits/float
3, 3, 3, 3, 3, 3, // RGB 8,10,12,14,16,32/f

4, 4, 4, // 8 bits YUVA 420,422,444
4, 4, 4, // 10 bits YUVA 420,422,444
4, 4, 4, // 12 bits YUVA 420,422,444
4, 4, 4, // 14 bits YUVA 420,422,444
4, 4, 4, // 16 bits YUVA 420,422,444
4, 4, 4, // 32 bits YUVA 420,422,444

4, 4, 4, 4, 4, 4 // RGBAP 8, 10, 12, 14, 16, 32

};

template<Colorspace C> int plane_count() { return plane_counts[C]; }

static const bool planes_isRGB[COLORSPACE_COUNT] = { false, 
false, false, false, false, false, false, false,
false, // 411
false, false, false, false, // 10 bits
false, false, false, false, // 12 bits
false, false, false, false, // 14 bits
false, false, false, false, // 16 bits
false, false, false, false, // 32 bits/float

true, true, true, true, true, true, // RGB 8,10,12,14,16,32/f

false, false, false, // 8 bits YUVA 420,422,444
false, false, false, // 10 bits YUVA 420,422,444
false, false, false, // 12 bits YUVA 420,422,444
false, false, false, // 14 bits YUVA 420,422,444
false, false, false, // 16 bits YUVA 420,422,444
false, false, false, // 32 bits YUVA 420,422,444

true, true, true, true, true, true // RGBAP 8,10,12,14,16,32/f

};

template<Colorspace C> bool plane_isRGB() { return planes_isRGB[C]; }

static const int width_ratios[4][COLORSPACE_COUNT] =
{
//                                                                                          RGBP              YUVA                                                 RGBAP
//                           YV                                                             0  1  1  1  1     0        1        1        1        1                 0  1  1  1  1  1
//      8 bits              411 10          12          14          16          32          8, 0, 2, 4, 6, S  8        0        2        4        6        S        8  0  2  4  6  S
   { 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
   { 0, 0, 2, 2, 2, 2, 1, 1, 1, 0, 2, 2, 1, 0, 2, 2, 1, 0, 2, 2, 1, 0, 2, 2, 1, 0, 2, 2, 1, 1, 1, 1, 1, 1, 1, 2, 2, 1, 2, 2, 1, 2, 2, 1, 2, 2, 1, 2, 2, 1, 2, 2, 1, 1, 1, 1, 1, 1, 1 },
   { 0, 0, 2, 2, 2, 2, 1, 1, 1, 0, 2, 2, 1, 0, 2, 2, 1, 0, 2, 2, 1, 0, 2, 2, 1, 0, 2, 2, 1, 1, 1, 1, 1, 1, 1, 2, 2, 1, 2, 2, 1, 2, 2, 1, 2, 2, 1, 2, 2, 1, 2, 2, 1, 1, 1, 1, 1, 1, 1 },
   { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
};

template<Colorspace C> int width_ratio(int nPlane) { return width_ratios[nPlane][C]; }

static const int height_ratios[4][COLORSPACE_COUNT] =
{
//                                                                                          RGBP              YUVA                                                 RGBAP
//                           YV                                                             0  1  1  1  1     0        1        1        1        1                 0  1  1  1  1  1
//      8 bits              411 10          12          14          16          32          8, 0, 2, 4, 6, S  8        0        2        4        6        S        8  0  2  4  6  S
   { 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
   { 0, 0, 2, 2, 1, 1, 1, 1, 4, 0, 2, 1, 1, 0, 2, 1, 1, 0, 2, 1, 1, 0, 2, 1, 1, 0, 2, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 2, 1, 1, 2, 1, 1, 2, 1, 1, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1 },
   { 0, 0, 2, 2, 1, 1, 1, 1, 4, 0, 2, 1, 1, 0, 2, 1, 1, 0, 2, 1, 1, 0, 2, 1, 1, 0, 2, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 2, 1, 1, 2, 1, 1, 2, 1, 1, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1 },
   { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
};

template<Colorspace C> int height_ratio(int nPlane) { return height_ratios[nPlane][C]; }

/* bit depths, according to the colorspace */
static const int bit_depths[COLORSPACE_COUNT] = { 0, 
8, 8, 8, 8, 8, 8, 8,
8, // 411
10, 10, 10, 10, // 10 bits
12, 12, 12, 12, // 12 bits
14, 14, 14, 14, // 14 bits
16, 16, 16, 16, // 16 bits
32, 32, 32, 32, // 32 bits/float
8, 10, 12, 14, 16, 32, // RGB 8,10,12,14,16,32/f
// YUVA
8, 8, 8,
10, 10, 10, // 10 bits
12, 12, 12, // 12 bits
14, 14, 14, // 14 bits
16, 16, 16, // 16 bits
32, 32, 32, // 32 bits/float
8, 10, 12, 14, 16, 32, // RGBA 8,10,12,14,16,32/f
};

template<Colorspace C> int bit_depth() { return bit_depths[C]; }

/* not used
static const int pixel_sizes[3][COLORSPACE_COUNT] = 
{
   { 0, 1, 1, 1, 1, 1, 1, 1, },
   { 0, 0, 4, 4, 2, 2, 1, 1, },
   { 0, 0, 4, 4, 2, 2, 1, 1, },
};

template<Colorspace C> int pixel_size(int nPlane) { return pixel_sizes[C][nPlane]; }
*/
} // namespace Filtering

#endif
