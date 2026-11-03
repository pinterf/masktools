#include "edgemask.h"
#include "../functions.h"
#include <cmath>

using namespace Filtering;

namespace Filtering { namespace MaskTools { namespace Filters { namespace Mask { namespace Edge {

namespace {

inline uint8_t convolution(uint8_t a11, uint8_t a21, uint8_t a31, uint8_t a12, uint8_t a22, uint8_t a32, uint8_t a13, uint8_t a23, uint8_t a33, const int16_t matrix[10], int nLowThreshold, int nHighThreshold)
{
   return threshold<uint8_t, int>(abs((a11 * matrix[0] + a21 * matrix[1] + a31 * matrix[2] + 
                                    a12 * matrix[3] + a22 * matrix[4] + a32 * matrix[5] +
                                    a13 * matrix[6] + a23 * matrix[7] + a33 * matrix[8]) / matrix[9]), nLowThreshold, nHighThreshold);
}

inline uint8_t sobel_real(uint8_t a11, uint8_t a21, uint8_t a31, uint8_t a12, uint8_t a22, uint8_t a32, uint8_t a13, uint8_t a23, uint8_t a33, const int16_t matrix[10], int nLowThreshold, int nHighThreshold)
{
   UNUSED(a22); UNUSED(matrix);
   const int horizontal = (a31 - a11) + (a33 - a13) + 2 * (a32 - a12);
   const int vertical = (a13 - a11) + (a33 - a31) + 2 * (a23 - a21);
   const float gx = static_cast<float>(horizontal);
   const float gy = static_cast<float>(vertical);
   // Intentionally use native rounding (normally nearest-even), without Expr's +0.5f bias.
   const int magnitude = static_cast<int>(std::lrint(std::sqrt(gx * gx + gy * gy)));
   return clip<uint8_t, int>(threshold<int, int>(magnitude, nLowThreshold, nHighThreshold, 0, 255));
}

inline uint8_t sobel(uint8_t a11, uint8_t a21, uint8_t a31, uint8_t a12, uint8_t a22, uint8_t a32, uint8_t a13, uint8_t a23, uint8_t a33, const int16_t matrix[10], int nLowThreshold, int nHighThreshold)
{
   UNUSED(a11); UNUSED(a13); UNUSED(a22); UNUSED(a31); UNUSED(a33); UNUSED(matrix); 
   return threshold<uint8_t, int>(abs( (int)a32 + a23 - a12 - a21 ) >> 1, nLowThreshold, nHighThreshold);
}

inline uint8_t roberts(uint8_t a11, uint8_t a21, uint8_t a31, uint8_t a12, uint8_t a22, uint8_t a32, uint8_t a13, uint8_t a23, uint8_t a33, const int16_t matrix[10], int nLowThreshold, int nHighThreshold)
{
   UNUSED(a11); UNUSED(a12); UNUSED(a13); UNUSED(a21); UNUSED(a31); UNUSED(a33); UNUSED(matrix); 
   return threshold<uint8_t, int>(abs( ((int)a22 << 1) - a32 - a23 ) >> 1, nLowThreshold, nHighThreshold);
}

inline uint8_t laplace(uint8_t a11, uint8_t a21, uint8_t a31, uint8_t a12, uint8_t a22, uint8_t a32, uint8_t a13, uint8_t a23, uint8_t a33, const int16_t matrix[10], int nLowThreshold, int nHighThreshold)
{
   UNUSED(matrix); 
   return threshold<uint8_t, int>(abs( ((int)a22 << 3) - a32 - a23 - a11 - a21 - a31 - a12 - a13 - a33 ) >> 3, nLowThreshold, nHighThreshold);
}

inline uint8_t morpho(uint8_t a11, uint8_t a21, uint8_t a31, uint8_t a12, uint8_t a22, uint8_t a32, uint8_t a13, uint8_t a23, uint8_t a33, const int16_t matrix[10], int nLowThreshold, int nHighThreshold)
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

   return threshold<uint8_t, int>( nMax - nMin, nLowThreshold, nHighThreshold );
}

inline uint8_t cartoon(uint8_t a11, uint8_t a21, uint8_t a31, uint8_t a12, uint8_t a22, uint8_t a32, uint8_t a13, uint8_t a23, uint8_t a33, const int16_t matrix[10], int nLowThreshold, int nHighThreshold)
{
   int val = ((int)a21 << 1) - a22 - a31;

   UNUSED(a11); UNUSED(a12); UNUSED(a13); UNUSED(a23); UNUSED(a32); UNUSED(a33); UNUSED(matrix); 

   return val > 0 ? 0 : threshold<uint8_t, int>( -val, nLowThreshold, nHighThreshold );
}

inline uint8_t prewitt(uint8_t a11, uint8_t a21, uint8_t a31, uint8_t a12, uint8_t a22, uint8_t a32, uint8_t a13, uint8_t a23, uint8_t a33, const int16_t matrix[10], int nLowThreshold, int nHighThreshold)
{
   const int p90 = a11 + a21 + a31 - a13 - a23 - a33;
   const int p180 = a11 + a12 + a13 - a31 - a32 - a33;
   const int p45 = a12 + a11 + a21 - a33 - a32 - a23;
   const int p135 = a13 + a12 + a23 - a31 - a32 - a21;

   const int max1 = max<int>( abs<int>( p90 ), abs<int>( p180 ) );
   const int max2 = max<int>( abs<int>( p45 ), abs<int>( p135 ) );
   const int maxv = max<int>( max1, max2 );

   UNUSED(a22); UNUSED(matrix); 

   return threshold<uint8_t, int>( maxv, nLowThreshold, nHighThreshold );
}

inline uint8_t half_prewitt(uint8_t a11, uint8_t a21, uint8_t a31, uint8_t a12, uint8_t a22, uint8_t a32, uint8_t a13, uint8_t a23, uint8_t a33, const int16_t matrix[10], int nLowThreshold, int nHighThreshold)
{
   const int p90 = a11 + 2 * a21 + a31 - a13 - 2 * a23 - a33;
   const int p180 = a11 + 2 * a12 + a13 - a31 - 2 * a32 - a33;
   const int maxv = max<int>( abs<int>( p90 ), abs<int>( p180 ) );

   UNUSED(a22); UNUSED(matrix);
   
   return threshold<uint8_t, int>( maxv, nLowThreshold, nHighThreshold );
}

class Thresholds {
   uint8_t nMinThreshold, nMaxThreshold;
public:
   Thresholds(uint8_t nMinThreshold, uint8_t nMaxThreshold) :
   nMinThreshold(nMinThreshold), nMaxThreshold(nMaxThreshold)
   {
   }

   int minpitch() const { return 0; }
   int maxpitch() const { return 0; }
   void nextRow() { }
   uint8_t min(int x) const { UNUSED(x); return nMinThreshold; }
   uint8_t max(int x) const { UNUSED(x); return nMaxThreshold; }
};

template<Filters::Mask::Operator op>
void mask_t(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, const int16_t matrix[10], int nLowThreshold, int nHighThreshold, int nWidth, int nHeight)
{
   Thresholds thresholds(static_cast<uint8_t>(nLowThreshold), static_cast<uint8_t>(nHighThreshold));

   Filters::Mask::generic_c<op, Thresholds>(pDst, nDstPitch, pSrc, nSrcPitch, thresholds, matrix, nWidth, nHeight);
}

} // namespace

using namespace Filters::Mask;

#define DEFINE_C_VERSIONS(name) \
Processor *name##_c          = &mask_t<name>;

DEFINE_C_VERSIONS(sobel_real)
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