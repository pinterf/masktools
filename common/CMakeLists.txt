# common: static library of the RPN parser, the dispatch constraints and helpers, linked into masktools2
file(GLOB_RECURSE common_Sources RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" CONFIGURE_DEPENDS "*.cpp" "*.h")

handle_all_arch_flags(common_Sources)
group_sources(${common_Sources})

add_library(common STATIC ${common_Sources})
set_target_properties(common PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
# dedicated include dir for avisynth.h
target_include_directories(common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include)
