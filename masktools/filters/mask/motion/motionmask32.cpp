#include "motionmask.h"
#include "motionmask32_c.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Mask { namespace Motion {

Processor32 *mask32_c = &mask32_t<sad32_c_op, mask32_c_op>;

bool scene_change32_c(uint8_t *pCurrent, ptrdiff_t nCurrentPitch, const uint8_t *pPrevious, ptrdiff_t nPreviousPitch, int nWidth, int nHeight, float nMotionThreshold)
{
    return sad32_c_op(pCurrent, nCurrentPitch, pPrevious, nPreviousPitch, nWidth, nHeight) > (double)(nMotionThreshold * nWidth * nHeight);
}

} } } } }