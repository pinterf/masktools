#ifndef __Mt_Lut_Functions_H__
#define __Mt_Lut_Functions_H__

#include <limits>

namespace Filtering { namespace MaskTools { namespace Filters { namespace Lut {

typedef enum {

   NONIZER = 0,
   AVERAGER = 1,
   MINIMIZER = 2,
   MAXIMIZER = 3,
   DEVIATER = 4,
   RANGIZER = 5,
   MEDIANIZER = 6,
   MEDIANIZER4 = 7,
   MEDIANIZER6 = 8,
   MEDIANIZER2 = 9,

   NUM_MODES,

} MPROCESSOR_MODE;

#define EXPRESSION_SINGLE( realtime, base, mode1, mode2 ) &base< realtime, mode2 >
#define EXPRESSION_DUAL( realtime, base, mode1, mode2 ) &base< mode1, mode2 >

#define MPROCESSOR(realtime, expr, param, mode) \
{ \
   expr( realtime, param, mode, Nonizer ), \
   expr( realtime, param, mode, Averager<int64_t> ), \
   expr( realtime, param, mode, Minimizer ), \
   expr( realtime, param, mode, Maximizer ), \
   expr( realtime, param, mode, Deviater<int64_t> ), \
   expr( realtime, param, mode, Rangizer ), \
   expr( realtime, param, mode, Medianizer ), \
   expr( realtime, param, mode, MedianizerBetter<4> ), \
   expr( realtime, param, mode, MedianizerBetter<6> ), \
   expr( realtime, param, mode, MedianizerBetter<2> ), \
}

#define MPROCESSOR_SINGLE( base, realtime )     MPROCESSOR( realtime, EXPRESSION_SINGLE, base, )
#define MPROCESSOR_DUAL( base, mode ) MPROCESSOR( false, EXPRESSION_DUAL, base, mode )
// for dual: parameter 'realtime' n/a

/* 16 bit */
// 10-16 bit: the accumulators get the bit depth at run time: T(mode, bits_per_pixel)
#define EXPRESSION16_SINGLE( realtime, base, mode1, mode2 ) &base< realtime, mode2 >
#define EXPRESSION16_DUAL( realtime, base, mode1, mode2 ) &base< mode1, mode2 >

#define MPROCESSOR16(realtime, expr, param, mode) \
{ \
   expr( realtime, param, mode, Nonizer16 ), \
   expr( realtime, param, mode, Averager16 ), \
   expr( realtime, param, mode, Minimizer16 ), \
   expr( realtime, param, mode, Maximizer16 ), \
   expr( realtime, param, mode, Deviater16 ), \
   expr( realtime, param, mode, Rangizer16 ), \
   expr( realtime, param, mode, Medianizer16 ), \
   expr( realtime, param, mode, MedianizerBetter16<4> ), \
   expr( realtime, param, mode, MedianizerBetter16<6> ), \
   expr( realtime, param, mode, MedianizerBetter16<2> ), \
}

#define MPROCESSOR16_SINGLE( base, realtime ) MPROCESSOR16( realtime, EXPRESSION16_SINGLE, base, )
#define MPROCESSOR16_DUAL( base, mode )       MPROCESSOR16( false, EXPRESSION16_DUAL, base, mode )

/* 32 bit float */
#define EXPRESSION32_SINGLE( base, mode1, mode2 ) &base< mode2 >
#define EXPRESSION32_DUAL( base, mode1, mode2 ) &base< mode1, mode2 >

#define MPROCESSOR32(expr, param, mode) \
{ \
   expr( param, mode, Nonizer32 ), \
   expr( param, mode, Averager32 ), \
   expr( param, mode, Minimizer32 ), \
   expr( param, mode, Maximizer32 ), \
   expr( param, mode, Deviater32 ), \
   expr( param, mode, Rangizer32 ), \
   expr( param, mode, Medianizer32 ), \
   expr( param, mode, MedianizerBetter32<4> ), \
   expr( param, mode, MedianizerBetter32<6> ), \
   expr( param, mode, MedianizerBetter32<2> ), \
}

#define MPROCESSOR32_SINGLE( base )     MPROCESSOR32( EXPRESSION32_SINGLE, base, )
#define MPROCESSOR32_DUAL( base, mode ) MPROCESSOR32( EXPRESSION32_DUAL, base, mode )


static inline int ModeToInt(const String &mode)
{
   if ( mode == "average" || mode == "avg" )
      return AVERAGER;
   else if ( mode == "standard deviation" || mode == "std" )
      return DEVIATER;
   else if ( mode == "minimum" || mode == "min" )
      return MINIMIZER;
   else if ( mode == "maximum" || mode == "max" )
      return MAXIMIZER;
   else if ( mode == "range" )
      return RANGIZER;
   else if ( mode == "median" || mode == "med" )
      return MEDIANIZER4;
   else
      return NONIZER;
}

class Nonizer {

   double *pdCoefficients;
   int nCoefficients;
   double dTotalCoefficients;
   double dSum;
   int nPosition;

public:
   Nonizer(const String &mode)
   {
      auto coefficients = Parser::getDefaultParser().parse(mode, Parser::SYMBOL_SEPARATORS_COORD).getExpression();
      nCoefficients = coefficients.size();
      pdCoefficients = new double[nCoefficients];
      int i = 0;

      dTotalCoefficients = 0;

      while ( !coefficients.empty() )
      {
         dTotalCoefficients += (pdCoefficients[i++] = coefficients.front().getValue(0, 0, 0));
         coefficients.pop_front();
      }
   }
   ~Nonizer()
   {
      delete[] pdCoefficients;
   }
   void reset(int nValue = 255)
   {
      UNUSED(nValue);
      dSum = 0;
      nPosition = 0;
   }
   void add(int nValue)
   {
      if (nPosition < nCoefficients)
         dSum += pdCoefficients[nPosition++] * nValue;
   }
   uint8_t finalize() const
   {
      return clip<uint8_t, int64_t>(convert<int64_t, double>(dSum / dTotalCoefficients));
   }

   void reset_w() { reset(); }
   void add_w(int nValue, float nWeight) { add(clip<uint8_t, float>(nValue*nWeight)); }
   uint8_t finalize_w() const { return finalize(); }

};

class Minimizer {

   int nMin;

public:

   Minimizer(const String &mode) { UNUSED(mode); }
   void reset() { nMin = 255; }
   void add(int nValue) { nMin = min<int>( nMin, nValue ); }
   uint8_t finalize() const { return static_cast<uint8_t>(nMin); }

   void reset_w() { reset(); }
   void add_w(int nValue, float nWeight) { add(clip<uint8_t, float>(nValue*nWeight)); }
   uint8_t finalize_w() const { return finalize(); }

};

class Medianizer {

   int nSize;
   int elements[256];

public:

   Medianizer(const String &mode) { UNUSED(mode); }
   void reset() { memset( elements, 0, sizeof( elements ) ); nSize = 0; }
   void reset(int nValue) { reset(); add( nValue ); }
   void add(int nValue) { elements[nValue]++; nSize++; }
   uint8_t finalize() const
   {
      int nCount = 0;
      int nIdx = -1;
      const int nLowHalf = (nSize + 1) >> 1;
      while ( nCount < nLowHalf )
         nCount += elements[++nIdx];

      if ( nSize & 1 ) /* nSize odd -> median belong to the middle class */
         return static_cast<uint8_t>(nIdx);
      else
      {
         if ( nCount >= nLowHalf + 1 ) /* nSize even, but middle class owns both median elements */
            return static_cast<uint8_t>(nIdx);
         else /* nSize even, middle class owns only one element, it's the lowest, so we search for the next one */
         {
            int nSndIdx = nIdx;
            while ( !elements[++nSndIdx] ) {}
            return static_cast<uint8_t>((nIdx + nSndIdx + 1) >> 1); /* and we return the average */
         }
      }
   }

   void reset_w() { reset(); }
   void add_w(int nValue, float nWeight) { add(clip<uint8_t, float>(nValue*nWeight)); }
   uint8_t finalize_w() const { return finalize(); }

};

template<int n>
class MedianizerBetter
{
   int nSize;
   int elements[256];
   int elements2[256 >> n];

public:
   MedianizerBetter(const String &mode) { UNUSED(mode); }
   void reset() { nSize = 0; memset( elements, 0, sizeof( elements ) ); memset( elements2, 0, sizeof( elements2 ) ); }
   void add(int nValue) { elements[nValue]++; elements2[nValue>>n]++; nSize++; }

   uint8_t finalize() const
   {
      int nCount = 0;
      int nIdx = -1;
      const int nLowHalf = (nSize + 1) >> 1;

      while ( nCount < nLowHalf )
         nCount += elements2[++nIdx]; /* low resolution search */

      nCount -= elements2[nIdx];
      nIdx <<= n;
      nIdx--;

      while ( nCount < nLowHalf )
         nCount += elements[++nIdx]; /* high resolution search */

      if ( nSize & 1 ) /* nSize odd -> median belong to the middle class */
         return static_cast<uint8_t>(nIdx);
      else
      {
         if ( nCount >= nLowHalf + 1 ) /* nSize even, but middle class owns both median elements */
            return static_cast<uint8_t>(nIdx);
         else /* nSize even, middle class owns only one element, it's the lowest, so we search for the next one */
         {
            int nSndIdx = nIdx;
            while ( !elements[++nSndIdx] ) {}
            return static_cast<uint8_t>((nIdx + nSndIdx + 1) >> 1); /* and we return the average */
         }
      }
   }

   void reset_w() { reset(); }
   void add_w(int nValue, float nWeight) { add(clip<uint8_t, float>(nValue*nWeight)); }
   uint8_t finalize_w() const { return finalize(); }

};

class Maximizer {

   int nMax;

public:

   Maximizer(const String &mode) { UNUSED(mode); }
   void reset() { nMax = 0; }
   void add(int nValue) { nMax = max<int>( nMax, nValue ); }
   uint8_t finalize() const { return static_cast<uint8_t>(nMax); }

   void reset_w() { reset(); }
   void add_w(int nValue, float nWeight) { add(clip<uint8_t, float>(nValue*nWeight)); }
   uint8_t finalize_w() const { return finalize(); }

};

class Rangizer {

   int nMin, nMax;

public:

   Rangizer(const String &mode) { UNUSED(mode); }
   void reset() { nMin = 255; nMax = 0; }
   void add(int nValue) { nMin = min<int>( nMin, nValue ); nMax = max<int>( nMax, nValue ); }
   uint8_t finalize() const { return static_cast<uint8_t>(nMax - nMin); }

   void reset_w() { reset(); }
   void add_w(int nValue, float nWeight) { add(clip<uint8_t,float>(nValue*nWeight)); }
   uint8_t finalize_w() const { return finalize(); }

};

// only <int> is used, for 4K+ videos <int64_t> needed
template<typename T>
class Averager
{

   T nSum, nCount;
  // for weight mode:
   double fSum;
   float fWeightSum;

public:

   Averager(const String &mode) { UNUSED(mode); }
   void reset() { nSum = 0; nCount = 0; }
   void add(int nValue) { nSum += nValue; nCount++; }
   // 4K era warning!! Integer can hold the sum of 8 421 504 pixels (worst case 255)
   // 3840 x 2180 = 8371200, just below limit
   // 4096 x 2180 = 8929280 pixels, over limit!
   // unsigned int32 is OK, but is not enough for 8K videos
   uint8_t finalize() const { return static_cast<uint8_t>(rounded_division<T>( nSum, nCount )); }

   void reset_w() { fSum = 0.0; fWeightSum = 0.0f; }
   void add_w(int nValue, float nWeight) { fSum += nValue*nWeight; fWeightSum += nWeight; }
   uint8_t finalize_w() const { return clip<uint8_t, float>((float)(fSum / fWeightSum)); }
};

template<typename T>
class Deviater
{
   T nSum, nCount;
   int64_t nSum2;
   // for weight mode:
   double fSum, fSum2;
   float fWeightSum;

public:

   Deviater(const String &mode) { UNUSED(mode); }
   void reset() { nSum = 0; nSum2 = 0; nCount = 0; }
   void reset(int nValue) { nSum = nValue; nSum2 = nValue * nValue; nCount = 1; }
   void add(int nValue) { nSum += nValue; nSum2 += nValue * nValue; nCount++; }
   uint8_t finalize() const { return clip<uint8_t, double>(sqrt((double(nSum2) * nCount - double(nSum) * double(nSum)) / (double(nCount) * nCount))); }

   void reset_w() { fSum = 0.0; fSum2 = 0.0; fWeightSum = 0.0f; }
   void reset_w(int nValue, float nWeight) { fSum = nValue*nWeight; fSum2 = fSum * fSum; fWeightSum = nWeight; }
   void add_w(int nValue, float nWeight) { fSum += nValue*nWeight; fSum += nValue*nValue*nWeight*nWeight; fWeightSum += nWeight; }
   uint8_t finalize_w() const { return clip<uint8_t, double>(sqrt((double(fSum2) * fWeightSum - double(fSum) * double(fSum)) / (double(fWeightSum) * fWeightSum))); }

};

/**********/
/* 16 bit */
/**********/
class Nonizer16 {

  double *pdCoefficients;
  int nCoefficients;
  double dTotalCoefficients;
  double dSum;
  int nPosition;

public:
  Nonizer16(const String &mode, int bits_per_pixel)
  {
    UNUSED(bits_per_pixel);
    auto coefficients = Parser::getDefaultParser().parse(mode, Parser::SYMBOL_SEPARATORS_COORD).getExpression();
    nCoefficients = coefficients.size();
    pdCoefficients = new double[nCoefficients];
    int i = 0;

    dTotalCoefficients = 0;

    while (!coefficients.empty())
    {
      dTotalCoefficients += (pdCoefficients[i++] = coefficients.front().getValue(0, 0, 0));
      coefficients.pop_front();
    }
  }
  ~Nonizer16()
  {
    delete[] pdCoefficients;
  }
  void reset()
  {
    dSum = 0;
    nPosition = 0;
  }
  void add(int nValue)
  {
    if (nPosition < nCoefficients)
      dSum += pdCoefficients[nPosition++] * nValue;
  }
  uint16_t finalize() const
  {
    return clip<uint16_t, int64_t>(convert<int64_t, double>(dSum / dTotalCoefficients));
  }

  void reset_w() { reset(); }
  void add_w(int nValue, float nWeight) { add(clip<uint16_t, float>(nValue*nWeight)); }
  uint16_t finalize_w() const { return finalize(); }

};

class Minimizer16 {

  int nMin;
  const int nMaxPixel;

public:

  Minimizer16(const String &mode, int bits_per_pixel) : nMaxPixel((1 << bits_per_pixel) - 1) { UNUSED(mode); }
  void reset() { nMin = nMaxPixel; }
  void add(int nValue) { nMin = min<int>(nMin, nValue); }
  uint16_t finalize() const { return static_cast<uint16_t>(nMin); }

  void reset_w() { reset(); }
  void add_w(int nValue, float nWeight) { add(clip<uint16_t, float>(nValue*nWeight)); }
  uint16_t finalize_w() const { return finalize(); }

};

class Medianizer16 {

  int nSize;
  int elements[65536]; // really 1 << bits_per_pixel, but avoid overflow

public:

  Medianizer16(const String &mode, int bits_per_pixel) { UNUSED(mode); UNUSED(bits_per_pixel); }
  void reset() { memset(elements, 0, sizeof(elements)); nSize = 0; }
  void reset(int nValue) { reset(); add(nValue); }
  void add(int nValue) { elements[nValue]++; nSize++; }
  uint16_t finalize() const
  {
    int nCount = 0;
    int nIdx = -1;
    const int nLowHalf = (nSize + 1) >> 1;
    while (nCount < nLowHalf)
      nCount += elements[++nIdx];

    if (nSize & 1) /* nSize odd -> median belong to the middle class */
      return static_cast<uint16_t>(nIdx);
    else
    {
      if (nCount >= nLowHalf + 1) /* nSize even, but middle class owns both median elements */
        return static_cast<uint16_t>(nIdx);
      else /* nSize even, middle class owns only one element, it's the lowest, so we search for the next one */
      {
        int nSndIdx = nIdx;
        while (!elements[++nSndIdx]) {}
        return static_cast<uint16_t>((nIdx + nSndIdx + 1) >> 1); /* and we return the average */
      }
    }
  }

  void reset_w() { reset(); }
  void add_w(int nValue, float nWeight) { add(clip<uint16_t, float>(nValue*nWeight)); }
  uint16_t finalize_w() const { return finalize(); }

};

template<int n>
class MedianizerBetter16
{
  int nSize;
  int elements[65536];
  int elements2[65536 >> n];

public:
  MedianizerBetter16(const String &mode, int bits_per_pixel) { UNUSED(mode); UNUSED(bits_per_pixel); }
  void reset() { nSize = 0; memset(elements, 0, sizeof(elements)); memset(elements2, 0, sizeof(elements2)); }
  void add(int nValue) { elements[nValue]++; elements2[nValue >> n]++; nSize++; }

  uint16_t finalize() const
  {
    int nCount = 0;
    int nIdx = -1;
    const int nLowHalf = (nSize + 1) >> 1;

    while (nCount < nLowHalf)
      nCount += elements2[++nIdx]; /* low resolution search */

    nCount -= elements2[nIdx];
    nIdx <<= n;
    nIdx--;

    while (nCount < nLowHalf)
      nCount += elements[++nIdx]; /* high resolution search */

    if (nSize & 1) /* nSize odd -> median belong to the middle class */
      return static_cast<uint16_t>(nIdx);
    else
    {
      if (nCount >= nLowHalf + 1) /* nSize even, but middle class owns both median elements */
        return static_cast<uint16_t>(nIdx);
      else /* nSize even, middle class owns only one element, it's the lowest, so we search for the next one */
      {
        int nSndIdx = nIdx;
        while (!elements[++nSndIdx]) {}
        return static_cast<uint16_t>((nIdx + nSndIdx + 1) >> 1); /* and we return the average */
      }
    }
  }

  void reset_w() { reset(); }
  void add_w(int nValue, float nWeight) { add(clip<uint16_t, float>(nValue*nWeight)); }
  uint16_t finalize_w() const { return finalize(); }

};

class Maximizer16 {

  int nMax;

public:

  Maximizer16(const String &mode, int bits_per_pixel) { UNUSED(mode); UNUSED(bits_per_pixel); }
  void reset() { nMax = 0; }
  void add(int nValue) { nMax = max<int>(nMax, nValue); }
  uint16_t finalize() const { return static_cast<uint16_t>(nMax); }

  void reset_w() { reset(); }
  void add_w(int nValue, float nWeight) { add(clip<uint16_t, float>(nValue*nWeight)); }
  uint16_t finalize_w() const { return finalize(); }

};

class Rangizer16 {

  int nMin, nMax;
  const int nMaxPixel;

public:

  Rangizer16(const String &mode, int bits_per_pixel) : nMaxPixel((1 << bits_per_pixel) - 1) { UNUSED(mode); }
  void reset() { nMin = nMaxPixel; nMax = 0; }
  void add(int nValue) { nMin = min<int>(nMin, nValue); nMax = max<int>(nMax, nValue); }
  uint16_t finalize() const { return static_cast<uint16_t>(nMax - nMin); }

  void reset_w() { reset(); }
  void add_w(int nValue, float nWeight) { add(clip<uint16_t, float>(nValue*nWeight)); }
  uint16_t finalize_w() const { return finalize(); }

};

// for 4K+ videos or high bit depth <int64_t> needed
class Averager16
{

  int64_t nSum;
  int nCount;
  // for weight mode:
  double fSum;
  float fWeightSum;

public:

  Averager16(const String &mode, int bits_per_pixel) { UNUSED(mode); UNUSED(bits_per_pixel); }
  void reset() { nSum = 0; nCount = 0; }
  void add(int nValue) { nSum += nValue; nCount++; }
  uint16_t finalize() const { return static_cast<uint16_t>(rounded_division<int64_t>(nSum, nCount)); }

  void reset_w() { fSum = 0.0; fWeightSum = 0.0f; }
  void add_w(int nValue, float nWeight) { fSum += nValue*nWeight; fWeightSum += nWeight; }
  uint16_t finalize_w() const { return clip<uint16_t, int64_t>(convert<int64_t, double>(fSum / fWeightSum)); }

};

class Deviater16
{
  int64_t nSum;
  int nCount;
  int64_t nSum2;
  // for weight mode:
  double fSum, fSum2;
  float fWeightSum;

public:

  Deviater16(const String &mode, int bits_per_pixel) { UNUSED(mode); UNUSED(bits_per_pixel); }
  void reset() { nSum = 0; nSum2 = 0; nCount = 0; }
  void reset(int nValue) { nSum = nValue; nSum2 = nValue * nValue; nCount = 1; }
  void add(int nValue) { nSum += nValue; nSum2 += nValue * nValue; nCount++; }
  uint16_t finalize() const { return convert<uint16_t, double>(sqrt((double(nSum2) * nCount - double(nSum) * double(nSum)) / (double(nCount) * nCount))); }
  
  void reset_w() { fSum = 0.0; fSum2 = 0.0; fWeightSum = 0.0f; }
  void reset_w(int nValue, float nWeight) { fSum = nValue*nWeight; fSum2 = fSum * fSum; fWeightSum = nWeight; }
  void add_w(int nValue, float nWeight) { fSum += nValue*nWeight; fSum += nValue*nValue*nWeight*nWeight; fWeightSum += nWeight; }
  uint16_t finalize_w() const { return clip<uint16_t, double>(sqrt((double(fSum2) * fWeightSum - double(fSum) * double(fSum)) / (double(fWeightSum) * fWeightSum))); }

};

/**********/
/* 32 bit */
/**********/
class Nonizer32 {

  double *pdCoefficients;
  int nCoefficients;
  double dTotalCoefficients;
  double dSum;
  int nPosition;

public:
  Nonizer32(const String &mode)
  {
    auto coefficients = Parser::getDefaultParser().parse(mode, Parser::SYMBOL_SEPARATORS_COORD).getExpression();
    nCoefficients = coefficients.size();
    pdCoefficients = new double[nCoefficients];
    int i = 0;

    dTotalCoefficients = 0;

    while (!coefficients.empty())
    {
      dTotalCoefficients += (pdCoefficients[i++] = coefficients.front().getValue(0, 0, 0));
      coefficients.pop_front();
    }
  }
  ~Nonizer32()
  {
    delete[] pdCoefficients;
  }
  void reset(float nValue = 1.0)
  {
    UNUSED(nValue);
    dSum = 0;
    nPosition = 0;
  }
  void add(float nValue)
  {
    if (nPosition < nCoefficients)
      dSum += pdCoefficients[nPosition++] * nValue;
  }
  float finalize() const
  {
    return (float)(dSum / dTotalCoefficients);
  }

  void reset_w() { reset(); }
  void add_w(float nValue, float nWeight) { add(nValue*nWeight); }
  float finalize_w() const { return finalize(); }

};

class Minimizer32 {

  float nMin;

public:

  Minimizer32(const String &mode) { UNUSED(mode); }
  void reset() { nMin = 1.0f; }
  void add(float nValue) { nMin = min<float>(nMin, nValue); }
  float finalize() const { return nMin; }

  void reset_w() { reset(); }
  void add_w(float nValue, float nWeight) { add(nValue*nWeight); }
  float finalize_w() const { return finalize(); }

};

class Medianizer32 {

  int nSize;
  // fake median, we simulate median as float quantized to 16 bit, then scale back the result
  int elements[65536]; 

public:

  Medianizer32(const String &mode) { UNUSED(mode); }
  void reset() { memset(elements, 0, sizeof(elements)); nSize = 0; }
  void reset(float nValue) { reset(); add(nValue); }
  void add(float nValue) { elements[nValue <= 0.0f ? 0 : (nValue>=1.0f ? 65535 : uint16_t(nValue * 65535))]++; nSize++; }
  float finalize() const
  {
    int nCount = 0;
    int nIdx = -1;
    const int nLowHalf = (nSize + 1) >> 1;
    while (nCount < nLowHalf)
      nCount += elements[++nIdx];

    if (nSize & 1) /* nSize odd -> median belong to the middle class */
      return nIdx / 65535.0f;
    else
    {
      if (nCount >= nLowHalf + 1) /* nSize even, but middle class owns both median elements */
        return nIdx / 65535.0f;
      else /* nSize even, middle class owns only one element, it's the lowest, so we search for the next one */
      {
        int nSndIdx = nIdx;
        while (!elements[++nSndIdx]) {}
        return (nIdx + nSndIdx)/2.0f / 65535.0f; /* and we return the average, and scale back to float */
      }
    }
  }

  void reset_w() { reset(); }
  void add_w(float nValue, float nWeight) { add(nValue*nWeight); }
  float finalize_w() const { return finalize(); }

};

template<int n>
class MedianizerBetter32
{
  int nSize;
  // fake median, we simulate median as float quantized to 16 bit, then scale back the result
  int elements[65536];
  int elements2[65536 >> n];

public:
  MedianizerBetter32(const String &mode) { UNUSED(mode); }
  void reset() { nSize = 0; memset(elements, 0, sizeof(elements)); memset(elements2, 0, sizeof(elements2)); }
  void add(float nValue) { int nValueNew = nValue < 0.0f ? 0 : (nValue >= 1.0f ? 65535 : uint16_t(nValue * 65535));  elements[nValueNew]++; elements2[nValueNew >> n]++; nSize++; }

  float finalize() const
  {
    int nCount = 0;
    int nIdx = -1;
    const int nLowHalf = (nSize + 1) >> 1;

    while (nCount < nLowHalf)
      nCount += elements2[++nIdx]; /* low resolution search */

    nCount -= elements2[nIdx];
    nIdx <<= n;
    nIdx--;

    while (nCount < nLowHalf)
      nCount += elements[++nIdx]; /* high resolution search */

    if (nSize & 1) /* nSize odd -> median belong to the middle class */
      return nIdx / 65535.0f; // scale back
    else
    {
      if (nCount >= nLowHalf + 1) /* nSize even, but middle class owns both median elements */
        return nIdx / 65535.0f;
      else /* nSize even, middle class owns only one element, it's the lowest, so we search for the next one */
      {
        int nSndIdx = nIdx;
        while (!elements[++nSndIdx]) {}
        return (nIdx + nSndIdx) / 2.0f / 65535.0f; /* and we return the average */
      }
    }
  }

  void reset_w() { reset(); }
  void add_w(float nValue, float nWeight) { add(nValue*nWeight); }
  float finalize_w() const { return finalize(); }
};

class Maximizer32 {

  float nMax;

public:

  Maximizer32(const String &mode) { UNUSED(mode); }
  void reset() { nMax = 0; }
  void add(float nValue) { nMax = max<float>(nMax, nValue); }
  float finalize() const { return nMax; }

  void reset_w() { reset(); }
  void add_w(float nValue, float nWeight) { add(nValue*nWeight); }
  float finalize_w() const { return finalize(); }

};

class Rangizer32 {

  float nMin, nMax;

public:

  Rangizer32(const String &mode) { UNUSED(mode); }
  void reset() { nMin = 1.0f; nMax = 0.0f; }
  void add(float nValue) { nMin = min<float>(nMin, nValue); nMax = max<float>(nMax, nValue); }
  float finalize() const { return nMax - nMin; }

  void reset_w() { reset(); }
  void add_w(float nValue, float nWeight) { add(nValue*nWeight); }
  float finalize_w() const { return finalize(); }

};

// for 4K+ videos or high bit depth <int64_t> needed
class Averager32
{

  double nSum;
  int nCount;
  // for weight mode:
  double fSum;
  float fWeightSum;

public:

  Averager32(const String &mode) { UNUSED(mode); }
  void reset() { nSum = 0.0f; nCount = 0; }
  void add(float nValue) { nSum = nSum + nValue; nCount++; }
  float finalize() const { return (float)(nSum / nCount); }

  void reset_w() { fSum = 0.0; fWeightSum = 0.0f; }
  void add_w(float nValue, float nWeight) { fSum += nValue*nWeight; fWeightSum += nWeight; }
  float finalize_w() const { return float(fSum / fWeightSum); }

};

class Deviater32
{
  float nSum;
  int nCount;
  float nSum2;
  // for weight mode:
  double fSum, fSum2;
  float fWeightSum;

public:

  Deviater32(const String &mode) { UNUSED(mode); }
  void reset() { nSum = 0; nSum2 = 0; nCount = 0; }
  void reset(float nValue) { nSum = nValue; nSum2 = nValue * nValue; nCount = 1; }
  void add(float nValue) { nSum += nValue; nSum2 += nValue * nValue; nCount++; }
  float finalize() const { return (float)(sqrt((double(nSum2) * nCount - double(nSum) * double(nSum)) / (double(nCount) * nCount))); }

  void reset_w() { fSum = 0.0; fSum2 = 0.0; fWeightSum = 0.0f; }
  void reset_w(float nValue, float nWeight) { fSum = nValue*nWeight; fSum2 = fSum * fSum; fWeightSum = nWeight; }
  void add_w(float nValue, float nWeight) { fSum += nValue*nWeight; fSum += nValue*nValue*nWeight*nWeight; fWeightSum += nWeight; }
  float finalize_w() const { return (float)(sqrt((double(fSum2) * fWeightSum - double(fSum) * double(fSum)) / (double(fWeightSum) * fWeightSum))); }

};


} } } }

#endif
