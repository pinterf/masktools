#include "edgemask.h"
#include "../functions16.h"
#include <limits.h>

using namespace Filtering;

namespace Filtering { namespace MaskTools { namespace Filters { namespace Mask { namespace Edge {

namespace {

template<int bits_per_pixel>
inline uint16_t convolution(uint16_t a11, uint16_t a21, uint16_t a31, uint16_t a12, uint16_t a22, uint16_t a32, uint16_t a13, uint16_t a23, uint16_t a33, const int16_t matrix[10], int nLowThreshold, int nHighThreshold)
{
   return threshold16<bits_per_pixel>(abs((a11 * matrix[0] + a21 * matrix[1] + a31 * matrix[2] + 
                                    a12 * matrix[3] + a22 * matrix[4] + a32 * matrix[5] +
                                    a13 * matrix[6] + a23 * matrix[7] + a33 * matrix[8]) / matrix[9]), nLowThreshold, nHighThreshold);
}

template<int bits_per_pixel>
inline uint16_t sobel(uint16_t a11, uint16_t a21, uint16_t a31, uint16_t a12, uint16_t a22, uint16_t a32, uint16_t a13, uint16_t a23, uint16_t a33, const int16_t matrix[10], int nLowThreshold, int nHighThreshold)
{
   UNUSED(a11); UNUSED(a13); UNUSED(a22); UNUSED(a31); UNUSED(a33); UNUSED(matrix); 
   return threshold16<bits_per_pixel>(abs( (int)a32 + a23 - a12 - a21 ) >> 1, nLowThreshold, nHighThreshold);
}

template<int bits_per_pixel>
inline uint16_t roberts(uint16_t a11, uint16_t a21, uint16_t a31, uint16_t a12, uint16_t a22, uint16_t a32, uint16_t a13, uint16_t a23, uint16_t a33, const int16_t matrix[10], int nLowThreshold, int nHighThreshold)
{
   UNUSED(a11); UNUSED(a12); UNUSED(a13); UNUSED(a21); UNUSED(a31); UNUSED(a33); UNUSED(matrix); 
   return threshold16<bits_per_pixel>(abs( ((int)a22 << 1) - a32 - a23 ) >> 1, nLowThreshold, nHighThreshold);
}

template<int bits_per_pixel>
inline uint16_t laplace(uint16_t a11, uint16_t a21, uint16_t a31, uint16_t a12, uint16_t a22, uint16_t a32, uint16_t a13, uint16_t a23, uint16_t a33, const int16_t matrix[10], int nLowThreshold, int nHighThreshold)
{
   UNUSED(matrix); 
   return threshold16<bits_per_pixel>(abs( ((int)a22 << 3) - a32 - a23 - a11 - a21 - a31 - a12 - a13 - a33 ) >> 3, nLowThreshold, nHighThreshold);
}

template<int bits_per_pixel>
inline uint16_t morpho(uint16_t a11, uint16_t a21, uint16_t a31, uint16_t a12, uint16_t a22, uint16_t a32, uint16_t a13, uint16_t a23, uint16_t a33, const int16_t matrix[10], int nLowThreshold, int nHighThreshold)
{
   int nMin = a11, nMax = a11;

   UNUSED(matrix); 

   nMin = min<int>( nMin, a21 );
   nMax = max<int>( nMax, a21 );
   nMin = min<int>( nMin, a31 );
   nMax = max<int>( nMax, a31 );
   nMin = min<int>( nMin, a12 );
   nMax = max<int>( nMax, a12 );
   nMin = min<int>( nMin, a22 );
   nMax = max<int>( nMax, a22 );
   nMin = min<int>( nMin, a32 );
   nMax = max<int>( nMax, a32 );
   nMin = min<int>( nMin, a13 );
   nMax = max<int>( nMax, a13 );
   nMin = min<int>( nMin, a23 );
   nMax = max<int>( nMax, a23 );
   nMin = min<int>( nMin, a33 );
   nMax = max<int>( nMax, a33 );

   return threshold16<bits_per_pixel>( nMax - nMin, nLowThreshold, nHighThreshold );
}

template<int bits_per_pixel>
inline uint16_t cartoon(uint16_t a11, uint16_t a21, uint16_t a31, uint16_t a12, uint16_t a22, uint16_t a32, uint16_t a13, uint16_t a23, uint16_t a33, const int16_t matrix[10], int nLowThreshold, int nHighThreshold)
{
   int val = ((int)a21 << 1) - a22 - a31;

   UNUSED(a11); UNUSED(a12); UNUSED(a13); UNUSED(a23); UNUSED(a32); UNUSED(a33); UNUSED(matrix); 

   return val > 0 ? 0 : threshold16<bits_per_pixel>( -val, nLowThreshold, nHighThreshold );
}

template<int bits_per_pixel>
inline uint16_t prewitt(uint16_t a11, uint16_t a21, uint16_t a31, uint16_t a12, uint16_t a22, uint16_t a32, uint16_t a13, uint16_t a23, uint16_t a33, const int16_t matrix[10], int nLowThreshold, int nHighThreshold)
{
   const int p90 = a11 + a21 + a31 - a13 - a23 - a33;
   const int p180 = a11 + a12 + a13 - a31 - a32 - a33;
   const int p45 = a12 + a11 + a21 - a33 - a32 - a23;
   const int p135 = a13 + a12 + a23 - a31 - a32 - a21;

   const int max1 = max<int>( abs<int>( p90 ), abs<int>( p180 ) );
   const int max2 = max<int>( abs<int>( p45 ), abs<int>( p135 ) );
   const int maxv = max<int>( max1, max2 );

   UNUSED(a22); UNUSED(matrix); 

   return threshold16<bits_per_pixel>( maxv, nLowThreshold, nHighThreshold );
}

template<int bits_per_pixel>
inline uint16_t half_prewitt(uint16_t a11, uint16_t a21, uint16_t a31, uint16_t a12, uint16_t a22, uint16_t a32, uint16_t a13, uint16_t a23, uint16_t a33, const int16_t matrix[10], int nLowThreshold, int nHighThreshold)
{
   const int p90 = a11 + 2 * a21 + a31 - a13 - 2 * a23 - a33;
   const int p180 = a11 + 2 * a12 + a13 - a31 - 2 * a32 - a33;
   const int maxv = max<int>( abs<int>( p90 ), abs<int>( p180 ) );

   UNUSED(a22); UNUSED(matrix);
   
   return threshold16<bits_per_pixel>( maxv, nLowThreshold, nHighThreshold );
}

class Thresholds {
   uint16_t nMinThreshold, nMaxThreshold;
public:
   Thresholds(uint16_t nMinThreshold, uint16_t nMaxThreshold) :
   nMinThreshold(nMinThreshold), nMaxThreshold(nMaxThreshold)
   {
   }

   int minpitch() const { return 0; }
   int maxpitch() const { return 0; }
   void nextRow() { }
   uint16_t min(int x) const { UNUSED(x); return nMinThreshold; }
   uint16_t max(int x) const { UNUSED(x); return nMaxThreshold; }
};

template<Filters::Mask::Operator op>
void mask16_t(uint16_t *pDst, ptrdiff_t nDstPitch, const uint16_t *pSrc, ptrdiff_t nSrcPitch, const int16_t matrix[10], int nLowThreshold, int nHighThreshold, int nWidth, int nHeight)
{
   Thresholds thresholds(static_cast<uint16_t>(nLowThreshold), static_cast<uint16_t>(nHighThreshold));

   Filters::Mask::generic16_c<op, Thresholds>(pDst, nDstPitch, pSrc, nSrcPitch, thresholds, matrix, nWidth, nHeight);
}

} // namespace

using namespace Filters::Mask;

#define DEFINE_C_VERSIONS(name) \
Processor16 *name##_10_c = &mask16_t<name<10>>; \
Processor16 *name##_12_c = &mask16_t<name<12>>; \
Processor16 *name##_14_c = &mask16_t<name<14>>; \
Processor16 *name##_16_c = &mask16_t<name<16>>;

DEFINE_C_VERSIONS(sobel)
DEFINE_C_VERSIONS(roberts)
DEFINE_C_VERSIONS(laplace)
DEFINE_C_VERSIONS(prewitt)
DEFINE_C_VERSIONS(half_prewitt)
DEFINE_C_VERSIONS(convolution)
DEFINE_C_VERSIONS(morpho)
DEFINE_C_VERSIONS(cartoon)
#undef DEFINE_C_VERSIONS

} } } } }