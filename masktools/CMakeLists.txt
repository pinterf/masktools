set(PluginName "masktools2")

file(GLOB_RECURSE masktools_Sources RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" CONFIGURE_DEPENDS "*.cpp" "*.h")
# the shared sources (RPN parser, dispatch constraints, helpers) are compiled in, no separate library
file(GLOB_RECURSE common_Sources RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" CONFIGURE_DEPENDS "../common/*.cpp" "../common/*.h")
list(APPEND masktools_Sources ${common_Sources})

handle_all_arch_flags(masktools_Sources)
group_sources(${masktools_Sources})

# masktools2.dll, libmasktools2.so
add_library(${PluginName} SHARED ${masktools_Sources})

if(MINGW)
  set_target_properties(${PluginName} PROPERTIES PREFIX "")
  set_target_properties(${PluginName} PROPERTIES IMPORT_PREFIX "")
endif()

# version resource
if(WIN32)
  enable_language(RC)
  target_sources(${PluginName} PRIVATE "common/mt_resource.rc")
endif()

target_include_directories(${PluginName} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
# dedicated include dir for avisynth.h
target_include_directories(${PluginName} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)

install(TARGETS ${PluginName}
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}/avisynth")
