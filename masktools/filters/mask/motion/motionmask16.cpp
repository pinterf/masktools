#include "motionmask.h"
#include "motionmask16_c.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Mask { namespace Motion {

Processor16* mask16_c = &mask16_t<sad16_c_op, mask16_c_op>;

bool scene_change16_c(uint8_t *pCurrent, ptrdiff_t nCurrentPitch, const uint8_t *pPrevious, ptrdiff_t nPreviousPitch, int nWidth, int nHeight, int nMotionThreshold)
{
    return sad16_chunked<sad16_c_op>(pCurrent, nCurrentPitch, pPrevious, nPreviousPitch, nWidth, nHeight) > (uint64_t)nMotionThreshold * nWidth * nHeight;
}

} } } } }