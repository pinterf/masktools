#ifndef __Mt_COMMON_H__
#define __Mt_COMMON_H__

#include "../../common/utils/utils.h"

namespace Filtering {

// processor template parameters, also of C versions (the left and right edge of a 3x3 kernel)
enum class MemoryMode {
  SSE2_UNALIGNED,
  SSE2_ALIGNED
};

enum class Border {
  Left,
  Right,
  None
};

}

#endif // __Mt_COMMON_H__
