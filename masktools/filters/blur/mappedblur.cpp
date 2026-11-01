#include "mappedblur.h"
#include "../../filters/mask/functions.h"

using namespace Filtering;

namespace Filtering { namespace MaskTools { namespace Filters { namespace Blur {

inline uint8_t convolution_all_c(uint8_t a11, uint8_t a21, uint8_t a31, uint8_t a12, uint8_t a22, uint8_t a32, uint8_t a13, uint8_t a23, uint8_t a33, const int16_t matrix[10], int nLowThreshold, int nHighThreshold)
{
   int nSum = a22 * matrix[4];

   UNUSED(nHighThreshold);

   if ( abs(a11 - a22) < nLowThreshold ) nSum += a11 * matrix[0]; else nSum += a22 * matrix[0];
   if ( abs(a21 - a22) < nLowThreshold ) nSum += a21 * matrix[1]; else nSum += a22 * matrix[1];
   if ( abs(a31 - a22) < nLowThreshold ) nSum += a31 * matrix[2]; else nSum += a22 * matrix[2];
   if ( abs(a12 - a22) < nLowThreshold ) nSum += a12 * matrix[3]; else nSum += a22 * matrix[3];
   if ( abs(a32 - a22) < nLowThreshold ) nSum += a32 * matrix[5]; else nSum += a22 * matrix[5];
   if ( abs(a13 - a22) < nLowThreshold ) nSum += a13 * matrix[6]; else nSum += a22 * matrix[6];
   if ( abs(a23 - a22) < nLowThreshold ) nSum += a23 * matrix[7]; else nSum += a22 * matrix[7];
   if ( abs(a33 - a22) < nLowThreshold ) nSum += a33 * matrix[8]; else nSum += a22 * matrix[8];

   return clip<uint8_t, int>((nSum + matrix[9] / 2)/ matrix[9], 0, 255);
}

inline uint8_t convolution_below_c(uint8_t a11, uint8_t a21, uint8_t a31, uint8_t a12, uint8_t a22, uint8_t a32, uint8_t a13, uint8_t a23, uint8_t a33, const int16_t matrix[10], int nLowThreshold, int nHighThreshold)
{
   int nSum = a22 * matrix[4];
   int nSumCoeff = matrix[4];

   UNUSED(nHighThreshold);

   if ( abs(a11 - a22) < nLowThreshold ) { nSum += a11 * matrix[0]; nSumCoeff += matrix[0]; }
   if ( abs(a21 - a22) < nLowThreshold ) { nSum += a21 * matrix[1]; nSumCoeff += matrix[1]; }
   if ( abs(a31 - a22) < nLowThreshold ) { nSum += a31 * matrix[2]; nSumCoeff += matrix[2]; }
   if ( abs(a12 - a22) < nLowThreshold ) { nSum += a12 * matrix[3]; nSumCoeff += matrix[3]; }
   if ( abs(a32 - a22) < nLowThreshold ) { nSum += a32 * matrix[5]; nSumCoeff += matrix[5]; }
   if ( abs(a13 - a22) < nLowThreshold ) { nSum += a13 * matrix[6]; nSumCoeff += matrix[6]; }
   if ( abs(a23 - a22) < nLowThreshold ) { nSum += a23 * matrix[7]; nSumCoeff += matrix[7]; }
   if ( abs(a33 - a22) < nLowThreshold ) { nSum += a33 * matrix[8]; nSumCoeff += matrix[8]; }

   if ( nSumCoeff ) return clip<uint8_t, int>((nSum + nSumCoeff / 2) / nSumCoeff, 0, 255);
   else return a22;
}

// anonymous: mappedblur16.cpp / mappedblur32.cpp have their own Thresholds (same name, other element type)
namespace {
class Thresholds {
   const uint8_t *pThreshold;
   ptrdiff_t nPitch;
public:
   Thresholds(const uint8_t *pThreshold, ptrdiff_t nPitch) :
   pThreshold(pThreshold), nPitch(nPitch)
   {
   }

   ptrdiff_t minpitch() const { return nPitch; }
   ptrdiff_t maxpitch() const { return nPitch; }
   void nextRow() { pThreshold += nPitch; }
   uint8_t min(int x) const { return pThreshold[x]; }
   uint8_t max(int x) const { return pThreshold[x]; }
};
} // namespace

template<Filters::Mask::Operator op>
void mask_t(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, const uint8_t *pMask, ptrdiff_t nMaskPitch, const int16_t matrix[10], int nWidth, int nHeight)
{
   Thresholds thresholds(pMask, nMaskPitch);
   Filters::Mask::generic_c<op, Thresholds>(pDst, nDstPitch, pSrc, nSrcPitch, thresholds, matrix, nWidth, nHeight);
}

Processor *mapped_below_c = &mask_t<convolution_below_c>;
Processor *mapped_all_c = &mask_t<convolution_all_c>;

} } } }