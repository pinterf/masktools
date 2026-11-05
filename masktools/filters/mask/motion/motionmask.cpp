#include "motionmask.h"
#include "motionmask_c.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Mask { namespace Motion {

Processor *mask_c    = &mask_t<sad_c_op, mask_c_op>;

bool scene_change_c(uint8_t *pCurrent, ptrdiff_t nCurrentPitch, const uint8_t *pPrevious, ptrdiff_t nPreviousPitch, int nWidth, int nHeight, int nMotionThreshold)
{
    return sad_c_op(pCurrent, nCurrentPitch, pPrevious, nPreviousPitch, nWidth, nHeight) > (uint64_t)nMotionThreshold * nWidth * nHeight;
}

} } } } }