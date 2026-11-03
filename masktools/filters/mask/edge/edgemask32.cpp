#include "edgemask.h"
#include "../functions32.h"
#include <cmath>

using namespace Filtering;

namespace Filtering { namespace MaskTools { namespace Filters { namespace Mask { namespace Edge {

namespace {

// remark: for float mask is 0.0 or 1.0, done in threshold<float, float>

inline float convolution(float a11, float a21, float a31, float a12, float a22, float a32, float a13, float a23, float a33, const float matrix[10], float nLowThreshold, float nHighThreshold)
{
   return threshold<float, float>(abs((a11 * matrix[0] + a21 * matrix[1] + a31 * matrix[2] + 
                                    a12 * matrix[3] + a22 * matrix[4] + a32 * matrix[5] +
                                    a13 * matrix[6] + a23 * matrix[7] + a33 * matrix[8]) / matrix[9]), nLowThreshold, nHighThreshold);
}

inline float sobel_real(float a11, float a21, float a31, float a12, float a22, float a32, float a13, float a23, float a33, const float matrix[10], float nLowThreshold, float nHighThreshold)
{
   UNUSED(a22); UNUSED(matrix);
   const float horizontal = (a31 - a11) + (a33 - a13) + 2.0f * (a32 - a12);
   const float vertical = (a13 - a11) + (a33 - a31) + 2.0f * (a23 - a21);
   const float magnitude = std::sqrt(horizontal * horizontal + vertical * vertical);
   return clip<float, float>(threshold<float, float>(magnitude, nLowThreshold, nHighThreshold), 0.0f, 1.0f);
}

inline float sobel(float a11, float a21, float a31, float a12, float a22, float a32, float a13, float a23, float a33, const float matrix[10], float nLowThreshold, float nHighThreshold)
{
   UNUSED(a11); UNUSED(a13); UNUSED(a22); UNUSED(a31); UNUSED(a33); UNUSED(matrix); 
   return threshold<float, float>(abs(a32 + a23 - a12 - a21 ) / 2.0f, nLowThreshold, nHighThreshold);
}

inline float roberts(float a11, float a21, float a31, float a12, float a22, float a32, float a13, float a23, float a33, const float matrix[10], float nLowThreshold, float nHighThreshold)
{
   UNUSED(a11); UNUSED(a12); UNUSED(a13); UNUSED(a21); UNUSED(a31); UNUSED(a33); UNUSED(matrix); 
   return threshold<float, float>(abs( (a22 * 2.0f) - a32 - a23 ) / 2.0f, nLowThreshold, nHighThreshold);
}

inline float laplace(float a11, float a21, float a31, float a12, float a22, float a32, float a13, float a23, float a33, const float matrix[10], float nLowThreshold, float nHighThreshold)
{
   UNUSED(matrix); 
   return threshold<float, float>(abs( (a22 * 8.0f) - a32 - a23 - a11 - a21 - a31 - a12 - a13 - a33 ) / 8.0f, nLowThreshold, nHighThreshold);
}

inline float morpho(float a11, float a21, float a31, float a12, float a22, float a32, float a13, float a23, float a33, const float matrix[10], float nLowThreshold, float nHighThreshold)
{
   float nMin = a11, nMax = a11;

   UNUSED(matrix); 

   nMin = min<float>( nMin, a21 );
   nMax = max<float>( nMax, a21 );
   nMin = min<float>( nMin, a31 );
   nMax = max<float>( nMax, a31 );
   nMin = min<float>( nMin, a12 );
   nMax = max<float>( nMax, a12 );
   nMin = min<float>( nMin, a22 );
   nMax = max<float>( nMax, a22 );
   nMin = min<float>( nMin, a32 );
   nMax = max<float>( nMax, a32 );
   nMin = min<float>( nMin, a13 );
   nMax = max<float>( nMax, a13 );
   nMin = min<float>( nMin, a23 );
   nMax = max<float>( nMax, a23 );
   nMin = min<float>( nMin, a33 );
   nMax = max<float>( nMax, a33 );

   return threshold<float, float>( nMax - nMin, nLowThreshold, nHighThreshold );
}

inline float cartoon(float a11, float a21, float a31, float a12, float a22, float a32, float a13, float a23, float a33, const float matrix[10], float nLowThreshold, float nHighThreshold)
{
   float val = (a21 * 2.0f) - a22 - a31;

   UNUSED(a11); UNUSED(a12); UNUSED(a13); UNUSED(a23); UNUSED(a32); UNUSED(a33); UNUSED(matrix); 

   return val > 0 ? 0 : threshold<float, float>( -val, nLowThreshold, nHighThreshold );
}

inline float prewitt(float a11, float a21, float a31, float a12, float a22, float a32, float a13, float a23, float a33, const float matrix[10], float nLowThreshold, float nHighThreshold)
{
   const float p90 = a11 + a21 + a31 - a13 - a23 - a33;
   const float p180 = a11 + a12 + a13 - a31 - a32 - a33;
   const float p45 = a12 + a11 + a21 - a33 - a32 - a23;
   const float p135 = a13 + a12 + a23 - a31 - a32 - a21;

   const float max1 = max<float>( abs<float>( p90 ), abs<float>( p180 ) );
   const float max2 = max<float>( abs<float>( p45 ), abs<float>( p135 ) );
   const float maxv = max<float>( max1, max2 );

   UNUSED(a22); UNUSED(matrix); 

   return threshold<float, float>( maxv, nLowThreshold, nHighThreshold );
}

inline float half_prewitt(float a11, float a21, float a31, float a12, float a22, float a32, float a13, float a23, float a33, const float matrix[10], float nLowThreshold, float nHighThreshold)
{
   const float p90 = a11 + 2 * a21 + a31 - a13 - 2 * a23 - a33;
   const float p180 = a11 + 2 * a12 + a13 - a31 - 2 * a32 - a33;
   const float maxv = max<float>( abs<float>( p90 ), abs<float>( p180 ) );

   UNUSED(a22); UNUSED(matrix);
   
   return threshold<float, float>( maxv, nLowThreshold, nHighThreshold );
}

class Thresholds {
   float nMinThreshold, nMaxThreshold;
public:
   Thresholds(float nMinThreshold, float nMaxThreshold) :
   nMinThreshold(nMinThreshold), nMaxThreshold(nMaxThreshold)
   {
   }

   int minpitch() const { return 0; }
   int maxpitch() const { return 0; }
   void nextRow() { }
   float min(int x) const { UNUSED(x); return nMinThreshold; }
   float max(int x) const { UNUSED(x); return nMaxThreshold; }
};

template<Filters::Mask::Operator op>
void mask32_t(float *pDst, ptrdiff_t nDstPitch, const float *pSrc, ptrdiff_t nSrcPitch, const float matrix[10], float nLowThreshold, float nHighThreshold, int nWidth, int nHeight)
{
   Thresholds thresholds(static_cast<float>(nLowThreshold), static_cast<float>(nHighThreshold));

   Filters::Mask::generic32_c<op, Thresholds>(pDst, nDstPitch, pSrc, nSrcPitch, thresholds, matrix, nWidth, nHeight);
}

} // namespace

using namespace Filters::Mask;

#define DEFINE_C_VERSIONS(name) \
Processor32 *name##_32_c          = &mask32_t<name>;

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