# MaskTools2 CMake build: MinGW (GCC, Clang, llvm-mingw), Linux, macOS.
# MSVC uses the solutions: masktools.slnx (Visual Studio 2026) and masktools.sln (Visual Studio 2022, XP build).
#
# Examples:
#   cmake -B build -S . -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake_minimum_required(VERSION 3.16...3.31)

project("masktools2" LANGUAGES CXX)
include(GNUInstallDirs)

# Avoid uselessly linking to unused libraries
set(CMAKE_STANDARD_LIBRARIES "" CACHE STRING "" FORCE)
set(CMAKE_C_STANDARD_LIBRARIES "" CACHE STRING "" FORCE)
set(CMAKE_CXX_STANDARD_LIBRARIES "" CACHE STRING "" FORCE)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_EXTENSIONS FALSE)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Target processor: Intel SIMD or NEON SIMD is turned on automatically.
# CMAKE_GENERATOR_PLATFORM covers the Visual Studio generators (-A ARM64).
string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" ARCHID)
if(CMAKE_GENERATOR_PLATFORM)
  string(TOLOWER "${CMAKE_GENERATOR_PLATFORM}" ARCHID)
endif()
message(STATUS "Target processor: ${ARCHID}")
set(INTEL_SIMD "OFF")
set(NEON_SIMD "OFF")
if(ARCHID MATCHES "^(x86|x64|i[3-6]86|amd64|x86_64|win32)$")
  set(INTEL_SIMD "ON")
elseif(ARCHID MATCHES "^(aarch64|arm64)$")
  set(NEON_SIMD "ON")
endif()

option(ENABLE_INTEL_SIMD "Enable SIMD intrinsics for Intel processors" "${INTEL_SIMD}")
option(ENABLE_NEON_SIMD "Enable SIMD intrinsics for AArch64 processors" "${NEON_SIMD}")
if(ENABLE_INTEL_SIMD AND ENABLE_NEON_SIMD)
  message(FATAL_ERROR "ENABLE_INTEL_SIMD and ENABLE_NEON_SIMD are mutually exclusive")
endif()
if(NOT ENABLE_INTEL_SIMD)
  # the x86 intrinsics are not separated from the C code yet
  message(FATAL_ERROR "Only the x86/x64 build with ENABLE_INTEL_SIMD=ON is possible for now")
endif()

if(MSVC)
  message(FATAL_ERROR "Use masktools.slnx (Visual Studio 2026) or masktools.sln (Visual Studio 2022) for the MSVC builds.")
endif()

if(ENABLE_INTEL_SIMD)
  # -msse4.1: until the x86 SIMD is split from the C code; then SSE2 base (32 bit x86: -msse2), higher ISA
  # levels per file or per function
  add_compile_options(-msse4.1)
  # float: a*b+c is not contracted to FMA in the AVX2 files, as with MSVC
  add_compile_options(-ffp-contract=off)
endif()
if(WIN32)
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--enable-stdcall-fixup")
elseif(APPLE)
  # macOS uses Clang's linker, doesn't like --no-undefined
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-undefined,error")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  # make sure there are no undefined symbols
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--no-undefined")
endif()

if(ENABLE_INTEL_SIMD)
  add_compile_definitions(INTEL_INTRINSICS)
elseif(ENABLE_NEON_SIMD)
  add_compile_definitions(NEON_INTRINSICS)
endif()

# Per-file ISA flags by file name suffix (from Avisynth+ avs_core/CMakeLists.txt), for the sources listed in
# SOURCES_VAR of the calling directory.
function(handle_arch_flags SOURCES_VAR ARCH_SUFFIX FLAGS)
  string(TOLOWER "${ARCH_SUFFIX}" ARCH_SUFFIX_LOWER)
  set(SRCS ${${SOURCES_VAR}})
  list(FILTER SRCS INCLUDE REGEX ".*_${ARCH_SUFFIX_LOWER}\\.cpp$")
  if(SRCS)
    set_source_files_properties(${SRCS} PROPERTIES COMPILE_FLAGS "${FLAGS}")
  endif()
endfunction()

function(handle_all_arch_flags SOURCES_VAR)
  if(ENABLE_INTEL_SIMD)
    handle_arch_flags(${SOURCES_VAR} SSSE3 " -mssse3 ")
    handle_arch_flags(${SOURCES_VAR} SSE41 " -msse4.1 ")
    handle_arch_flags(${SOURCES_VAR} AVX " -mavx ")
    handle_arch_flags(${SOURCES_VAR} AVX2 " -mavx2 -mfma ")
  elseif(ENABLE_NEON_SIMD)
    handle_arch_flags(${SOURCES_VAR} NEON " -march=armv8-a ")
  endif()
endfunction()

# Source groups by the directory structure (IDEs)
function(group_sources)
  foreach(FILE ${ARGN})
    get_filename_component(PARENT_DIR "${FILE}" PATH)
    string(REGEX REPLACE "(\\./)" "" GROUP "${PARENT_DIR}")
    string(REPLACE "/" "\\" GROUP "${GROUP}")
    if("${FILE}" MATCHES ".*\\.cpp")
      set(GROUP "Source Files\\${GROUP}")
    elseif("${FILE}" MATCHES ".*\\.h")
      set(GROUP "Header Files\\${GROUP}")
    endif()
    source_group("${GROUP}" FILES "${FILE}")
  endforeach()
endfunction()

add_subdirectory("common")
add_subdirectory("masktools")

# uninstall target
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
