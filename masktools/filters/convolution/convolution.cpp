#include "convolution.h"

using namespace Filtering;

// returns either int or float (depending on the convolution parameters)
// regardless of the bit depths
// bit depth is templated to pixel_t
template<class Type, class pixel_t>
class Left {
   const pixel_t *pSrc;
public:
   Left(const pixel_t *pSrc) : pSrc(pSrc) { }
   inline Type get(int nX, int nOffset) { if ( nX + nOffset < 0 ) return pSrc[0]; else return pSrc[nX + nOffset]; }
};

template<class Type, class pixel_t>
class Right {
   const pixel_t *pSrc;
   int nWidth;
public:
   Right(const pixel_t *pSrc, int nWidth) : pSrc(pSrc), nWidth(nWidth) { }
   inline Type get(int nX, int nOffset) { if ( nX + nOffset >= nWidth ) return pSrc[nWidth-1]; else return pSrc[nX + nOffset]; }
};

template<class Type, class pixel_t>
class Center {
   const pixel_t *pSrc;
public:
   Center(const pixel_t *pSrc) : pSrc(pSrc) { }
   inline Type get(int nX, int nOffset) { return pSrc[nX + nOffset]; }
};

template<class Type, class PixelFetcher>
Type compute_horizontal_vector_t(PixelFetcher &pf, const Type *horizontal, const int nHorizontal, int nOffset)
{
   Type nSum = 0;
   for ( int i = 0; i < nHorizontal; i++ )
      nSum += pf.get(i, nOffset) * horizontal[i];
   return nSum;
}

template<class Type, class pixel_t>
void compute_line_t(const pixel_t *pSrc, const Type *horizontal, const int nHorizontal, int nWidth, Type *vectors)
{
   Left<Type, pixel_t> left(pSrc);
   for ( int i = -(nHorizontal / 2); i < 0; i++ )
      vectors[i + nHorizontal / 2] = compute_horizontal_vector_t<Type, Left<Type, pixel_t> >(left, horizontal, nHorizontal, i);

   Center<Type, pixel_t> center(pSrc);
   for ( int i = 0; i < nWidth - nHorizontal; i++ )
      vectors[i + nHorizontal / 2] = compute_horizontal_vector_t<Type, Center<Type, pixel_t> >(center, horizontal, nHorizontal, i);

   Right<Type, pixel_t> right(pSrc, nWidth);
   for ( int i = nWidth - nHorizontal; i < nWidth - (nHorizontal / 2); i++ )
      vectors[i + nHorizontal / 2] = compute_horizontal_vector_t<Type, Right<Type, pixel_t> >(right, horizontal, nHorizontal, i);
}

template<class Type, class pixel_t, int bits_per_pixel>
pixel_t clamp(Type a);

template<> uint8_t clamp<int, uint8_t, 8>(int a) { return a < 0 ? 0 : ( a > 255 ? 255 : uint8_t(a)); }
template<> uint8_t clamp<float, uint8_t, 8>(float a) { return a < 0.0 ? 0 : ( a > 255.0 ? 255 : uint8_t(a+0.5) ); }
template<> uint16_t clamp<int, uint16_t, 10>(int a) { return a < 0 ? 0 : (a > 1023 ? 1023 : uint16_t(a)); }
template<> uint16_t clamp<float, uint16_t, 10>(float a) { return a < 0.0 ? 0 : (a > 1023 ? 1023 : uint16_t(a + 0.5)); }
template<> uint16_t clamp<int, uint16_t, 12>(int a) { return a < 0 ? 0 : (a > 4095 ? 4095 : uint16_t(a)); }
template<> uint16_t clamp<float, uint16_t, 12>(float a) { return a < 0.0 ? 0 : (a > 4095 ? 4095 : uint16_t(a + 0.5)); }
template<> uint16_t clamp<int, uint16_t, 14>(int a) { return a < 0 ? 0 : (a > 16383 ? 16383 : uint16_t(a)); }
template<> uint16_t clamp<float, uint16_t, 14>(float a) { return a < 0.0 ? 0 : (a > 16383 ? 16383 : uint16_t(a + 0.5)); }
template<> uint16_t clamp<int, uint16_t, 16>(int a) { return a < 0 ? 0 : (a > 65535 ? 65535 : uint16_t(a)); }
template<> uint16_t clamp<float, uint16_t, 16>(float a) { return a < 0.0 ? 0 : (a > 65535 ? 65535 : uint16_t(a + 0.5)); }
template<> float clamp<float, float, 32>(float a) { return a; /*< 0.0f ? 0 : (a > 1.0f ? 1.0f : a);*/ } // no clamp for float

template<class Type>
struct NOP{
   static Type function(Type x) { return x; }
};

template<class Type>
struct MIRROR {
   static Type function(Type x) { return x < 0 ? -x : x; }
};

template<class Type, class SaturateOp, class pixel_t, int bits_per_pixel>
class HorizontalVectors {
   Type **horizontals;
   Type **uncircled_horizontals;
   int nVertical;
   int nIdx;
public:
   HorizontalVectors() : horizontals(NULL), uncircled_horizontals(NULL) {}
   HorizontalVectors(int nWidth, int nVertical) : nVertical(nVertical), nIdx(0)
   {
      horizontals = new Type*[nVertical];
      for ( int i = 0; i < nVertical; i++ )
         horizontals[i] = new Type[nWidth];
      uncircled_horizontals = new Type*[nVertical];
   }
   ~HorizontalVectors()
   {
      if ( horizontals )
      {
         for ( int i = 0; i < nVertical; i++ )
            if ( horizontals[i] )
               delete[] horizontals[i];
         delete[] horizontals;
      }
   }
   Type *get_line(int _nIdx)
   {
      if ( _nIdx + this->nIdx < nVertical )
         return horizontals[_nIdx + this->nIdx];
      else
         return horizontals[_nIdx + this->nIdx - nVertical];
   }
   void next_line()
   {
      nIdx++;
      if ( nIdx >= nVertical )
         nIdx = 0;
   }
   void compute_verticals(pixel_t *pDst, const Type *vertical, int nWidth, Type nNormalization)
   {
      for ( int i = 0; i < nVertical; i++ )
         uncircled_horizontals[i] = get_line(i);

      for ( int i = 0; i < nWidth; i++ )
      {
         Type nSum = 0;
         for ( int j = 0; j < nVertical; j++ )
            nSum += uncircled_horizontals[j][i] * vertical[j];
         pDst[i] = clamp<Type, pixel_t, bits_per_pixel>((SaturateOp::function(nSum) + (sizeof(pixel_t) == 4 ? 0 : (nNormalization / 2))) / nNormalization);
         // for float (sizeof==4): don't add normalization/2. This is a rounding for integer. If normalization = 1.0, (totals parameter)
         // then it would shift all float values to with 0.5 which is half of its whole range!)
      }
   }
};

template<class Type> bool isNull(Type val) { UNUSED(val); return false; }
template<> bool isNull<double>(double val) { return val < 0.001f && val > -0.001f; }
template<> bool isNull<float>(float val) { return val < 0.001f && val > -0.001f; }
template<> bool isNull<int>(int val) { return val == 0; }

template<class Type>
Type compute_norm_t(const Type *horizontal, const Type *vertical, int nHorizontal, int nVertical)
{
   Type nSum = 0;
   for ( int i = 0; i < nVertical; i++ )
      for ( int j = 0; j < nHorizontal; j++ )
         nSum += horizontal[j] * vertical[i];

   if ( isNull<Type>(nSum) )
      return 1;

   return nSum;
}


template<class Type, class SaturateOp, class pixel_t, int bits_per_pixel>
void convolution_t(pixel_t *pDst, ptrdiff_t nDstPitch, const pixel_t *pSrc, ptrdiff_t nSrcPitch, 
                   void *_horizontal, void *_vertical, void *_total, const int nHorizontal, const int nVertical,
                   int nWidth, int nHeight)
{
   nDstPitch /= sizeof(pixel_t);
   nSrcPitch /= sizeof(pixel_t);

   const Type *horizontal = static_cast<const Type*>(_horizontal);
   const Type *vertical   = static_cast<const Type*>(_vertical);

   HorizontalVectors<Type, SaturateOp, pixel_t, bits_per_pixel> *horizontals = NULL;
   const Type nNormalization = _total ? *static_cast<const Type*>(_total) : compute_norm_t<Type>(horizontal, vertical, nHorizontal, nVertical);

   if ( !horizontals )
      horizontals = new HorizontalVectors<Type, SaturateOp, pixel_t, bits_per_pixel>(nWidth, nVertical);

   for ( int i = 0; i < nVertical / 2; i++ )
      compute_line_t<Type, pixel_t>(pSrc, horizontal, nHorizontal, nWidth, horizontals->get_line(i));

   for ( int i = nVertical / 2; i < nVertical - 1; i++ )
   {
      compute_line_t<Type, pixel_t>(pSrc, horizontal, nHorizontal, nWidth, horizontals->get_line(i));
      pSrc += nSrcPitch;
   }

   for ( int i = 0; i < nHeight - (nVertical / 2); i++ )
   {
      compute_line_t<Type, pixel_t>(pSrc, horizontal, nHorizontal, nWidth, horizontals->get_line(nVertical-1));
      horizontals->compute_verticals(pDst, vertical, nWidth, nNormalization);
      horizontals->next_line();
      pSrc += nSrcPitch;
      pDst += nDstPitch;
   }

   pSrc -= nSrcPitch;

   for ( int i = nHeight - (nVertical / 2); i < nHeight; i++ )
   {
      compute_line_t<Type, pixel_t>(pSrc, horizontal, nHorizontal, nWidth, horizontals->get_line(nVertical-1));
      horizontals->compute_verticals(pDst, vertical, nWidth, nNormalization);
      horizontals->next_line();
      pDst += nDstPitch;
   }

   delete horizontals;
}

namespace Filtering { namespace MaskTools { namespace Filters { namespace Convolution {

Processor *convolution_i_s_c = &::convolution_t<int, struct NOP<int> , uint8_t, 8>;
Processor *convolution_f_s_c = &::convolution_t<float, struct NOP<float>, uint8_t, 8 >;
Processor *convolution_i_m_c = &::convolution_t<int, struct MIRROR<int>, uint8_t, 8 >;
Processor *convolution_f_m_c = &::convolution_t<float, struct MIRROR<float>, uint8_t, 8>;

Processor16 *convolution_i_s_10_c = &::convolution_t<int, struct NOP<int>, uint16_t, 10>;
Processor16 *convolution_f_s_10_c = &::convolution_t<float, struct NOP<float>, uint16_t, 10 >;
Processor16 *convolution_i_m_10_c = &::convolution_t<int, struct MIRROR<int>, uint16_t, 10 >;
Processor16 *convolution_f_m_10_c = &::convolution_t<float, struct MIRROR<float>, uint16_t, 10>;

Processor16 *convolution_i_s_12_c = &::convolution_t<int, struct NOP<int>, uint16_t, 12>;
Processor16 *convolution_f_s_12_c = &::convolution_t<float, struct NOP<float>, uint16_t, 12 >;
Processor16 *convolution_i_m_12_c = &::convolution_t<int, struct MIRROR<int>, uint16_t, 12 >;
Processor16 *convolution_f_m_12_c = &::convolution_t<float, struct MIRROR<float>, uint16_t, 12>;

Processor16 *convolution_i_s_14_c = &::convolution_t<int, struct NOP<int>, uint16_t, 14>;
Processor16 *convolution_f_s_14_c = &::convolution_t<float, struct NOP<float>, uint16_t, 14 >;
Processor16 *convolution_i_m_14_c = &::convolution_t<int, struct MIRROR<int>, uint16_t, 14 >;
Processor16 *convolution_f_m_14_c = &::convolution_t<float, struct MIRROR<float>, uint16_t, 14>;

Processor16 *convolution_i_s_16_c = &::convolution_t<int, struct NOP<int>, uint16_t, 16>;
Processor16 *convolution_f_s_16_c = &::convolution_t<float, struct NOP<float>, uint16_t, 16 >;
Processor16 *convolution_i_m_16_c = &::convolution_t<int, struct MIRROR<int>, uint16_t, 16 >;
Processor16 *convolution_f_m_16_c = &::convolution_t<float, struct MIRROR<float>, uint16_t, 16>;

// 32 bit: no int version
//Processor32 *convolution_i_s_32_c = &::convolution_t<int, struct NOP<int>, float, 32>;
Processor32 *convolution_f_s_32_c = &::convolution_t<float, struct NOP<float>, float, 32 >;
//Processor32 *convolution_i_m_32_c = &::convolution_t<int, struct MIRROR<int>, float, 32 >;
Processor32 *convolution_f_m_32_c = &::convolution_t<float, struct MIRROR<float>, float, 32>;

} } } }