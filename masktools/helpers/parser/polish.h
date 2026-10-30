#ifndef __Mt_Helpers_Polish_H__
#define __Mt_Helpers_Polish_H__

#include "../../../common/utils/utils.h"

namespace Filtering { namespace MaskTools { namespace Helpers { namespace PolishConverter {

typedef String(StringConverter)(const String &);

StringConverter Converter; // infix -> reverse polish (mt_polish)
StringConverter Infix;     // reverse polish -> infix (mt_infix)

} } } }

#endif
