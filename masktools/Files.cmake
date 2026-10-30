FILE(GLOB masktools_Sources RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
  "*.c"
  "*.cpp"
  "*.h"
  "../common/clip/*.h"
  "../common/utils/*.h"
  "avsheaders/*.h"
  "avsheaders/avs/*.h"
  "avs2x/*.cpp"
  "avs2x/*.cpp"
  "common/base/*.h"
  "common/clip/*.h"
  "common/clip/*.cpp"
  "common/params/*.h"
#parser: mt_polish, mt_infix
  "helpers/parser/*.cpp"
  "helpers/parser/*.h"
  "filters/binarize/*.cpp"
  "filters/binarize/*.h"
  "filters/blur/*.cpp"
  "filters/blur/*.h"
  "filters/convolution/*.cpp"
  "filters/convolution/*.h"
  "filters/gradient/*.cpp"
  "filters/gradient/*.h"
  "filters/invert/*.cpp"
  "filters/invert/*.h"
  "filters/logic/*.cpp"
  "filters/logic/*.h"
  "filters/lut/lut/*.cpp"
  "filters/lut/lut/*.h"
  "filters/lut/lutf/*.cpp"
  "filters/lut/lutf/*.h"
  "filters/lut/luts/*.cpp"
  "filters/lut/luts/*.h"
  "filters/lut/lutspa/*.cpp"
  "filters/lut/lutspa/*.h"
  "filters/lut/lutsx/*.cpp"
  "filters/lut/lutsx/*.h"
  "filters/lut/lutxy/*.cpp"
  "filters/lut/lutxy/*.h"
  "filters/lut/lutxyz/*.cpp"
  "filters/lut/lutxyz/*.h"
  "filters/lut/lutxyza/*.cpp"
  "filters/lut/lutxyza/*.h"
  "filters/lut/*.h"
  "filters/mask/edge/*.cpp"
  "filters/mask/egde/*.h"
  "filters/mask/hysteresis/*.cpp"
  "filters/mask/hysteresis/*.h"
  "filters/mask/motion/*.cpp"
  "filters/mask/motion/*.h"
  "filters/mask/*.h"
  "filters/merge/*.cpp"
  "filters/merge/*.h"
  "filters/morphologic/deflate/*.cpp"
  "filters/morphologic/deflate/*.h"
  "filters/morphologic/expand/*.cpp"
  "filters/morphologic/expand/*.h"
  "filters/morphologic/inflate/*.cpp"
  "filters/morphologic/inflate/*.h"
  "filters/morphologic/inpand/*.cpp"
  "filters/morphologic/inpand/*.h"
  "filters/morphologic/*.h"
  "filters/support/adddiff/*.cpp"
  "filters/support/adddiff/*.h"
  "filters/support/adddiff16/*.cpp"
  "filters/support/adddiff16/*.h"
  "filters/support/average/*.cpp"
  "filters/support/average/*.h"
  "filters/support/average16/*.cpp"
  "filters/support/average16/*.h"
  "filters/support/clamp/*.cpp"
  "filters/support/clamp/*.h"
  "filters/support/clamp16/*.cpp"
  "filters/support/clamp16/*.h"
  "filters/support/makediff/*.cpp"
  "filters/support/makediff/*.h"
  "filters/support/makediff16/*.cpp"
  "filters/support/makediff16/*.h"
  "helpers/avs2x/*.cpp"
  "helpers/avs2x/*.h"
  "helpers/forms/*.cpp"
  "helpers/forms/*.h"
)

message("${masktools_Sources}")

IF( MSVC OR MINGW )
    # Export definitions in general are not needed on x64 and only cause warnings,
    # unfortunately we still must need a .def file for some COM functions.
    # NO C interface for this plugin
    # if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    #  LIST(APPEND masktools_Sources "masktools64.def")
    # else()
    #  LIST(APPEND masktools_Sources "masktools.def")
    # endif() 
ENDIF()

IF( MSVC_IDE )
    # Ninja, unfortunately, seems to have some issues with using rc.exe
    LIST(APPEND masktools_Sources "common/mt_resource.rc")
ENDIF()
