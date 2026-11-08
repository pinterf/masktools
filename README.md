### MaskTools 2 ###

**Masktools2 v2.2.30 (20220219)**

mod by pinterf

Mask creation, mask operations, lookup tables and expressions for AviSynth+ and classic AviSynth 2.6: mt_edge, mt_motion,
mt_merge, mt_lut and family, mt_expand and the other morphological filters, and more. 8-16 bit and 32 bit float, YUV(A)
including 4:1:1, 4:4:0 and 4:1:0, planar RGB(A), Y and YA.

**Reference of every filter and parameter:** [masktools/documentation/masktools2.md](masktools/documentation/masktools2.md).
It replaces the former list of differences to MaskTools 2.0b1, the parameter notes and the feature matrix that were here.
The original 2.0 manual is kept for history: [mt_masktools.html](masktools/documentation/mt_masktools.html).

Builds: Windows x86, x64 and ARM64 (`masktools2.dll`), Linux x86-64 and aarch64 (`libmasktools2.so`), macOS not tested
(`libmasktools2.dylib`). Build instructions at the end of this document.

Masktools2 info:
http://avisynth.nl/index.php/MaskTools2

Forum:
https://forum.doom9.org/showthread.php?t=174333

Article by tp7:
http://tp7.github.io/articles/masktools/

Project:
https://github.com/pinterf/masktools/tree/16bit/

Original version: tp7's MaskTools 2 repository.
https://github.com/tp7/masktools/

Changelog
**v2.3.0 (WIP)
- Dual filter signatures removed: one signature per filter, float parameters as float; integer arguments still work.
- Expressions: a constant power of 0.5 is sqrt, as in Expr: x 0.5 ^ on a negative x gives 0, was NaN. An exponent from a clip stays pow.
- scaleb, scalef are level conversions with the arithmetic of AviSynth+ Expr (and ConvertBits), for every base i8..f32: scalef on chroma scales around the center
  (128 scalef at 10 bit: 512 on chroma, was 514; x 5 scalef + on 16 bit U: 33925, was 34053), scaleb between a 10-16 bit base and float uses 255 steps
  (i10 940 scaleb: 0.92157, was 0.91887). Integer lut tables are per luma/chroma when the expression needs it. Docs: they are not for differences on chroma.
- mt_merge 4:2:2, cplace="top_left": 1-2-1 mask as with "mpeg2" (same horizontal siting), was the 1-1 box of "mpeg1".
- mt_merge luma=true on 4:1:1 and 4:1:0: low-pass mask kernels as in AviSynth+ Overlay. Centred: triangle 1-3-5-7-7-5-3-1, co-sited: tent
  1-2-3-4-3-2-1. 4:1:1 mpeg1 8 taps, mpeg2 and top_left 7 taps; 4:1:0 mpeg1 8x8, mpeg2 7x8, top_left 7x7. Filters are clamped to the processed window.
  C, SSE2, AVX2 and NEON at every depth; float results identical to C.
- mt_merge, integer clips: the blend divides exactly by the maximum, as AviSynth+ Merge, Layer and Overlay do. It divided by 256 (2^bits), with only the end points patched.
- mt_merge: the default of cplace is "auto": the _ChromaLocation frame property of clip1 (frame 0), else by format as in AviSynth+: 4:4:0 and 4:1:0 top_left, others mpeg2.
- mt_edge: new "sobel_real" computes the standard 1-2-1 horizontal/vertical derivatives and
  sqrt(Gx*Gx + Gy*Gy), unnormalized, with existing thresholds and mask-range clamping.
  The historical "sobel" calculation and default mode are unchanged. C, SSE2, AVX2 and NEON at every depth.
- mt_polish: numbers are written in their shortest exact form ("0.1", "1e-07", "1e+30"), with '.' whatever
  the locale; they were cut to 6 decimals (0.0000001 gave 0) and to 19 characters (1e30 gave
  1000000000000000019), 1e300 crashed. Numbers are read correctly rounded, independent of the locale.
- mt_polish: integers above 2147483647 are accepted (as a double), the conversion stopped at them.
- mt_polish: an expression which cannot be converted to its end is an error ("syntax error at position n"),
  instead of the converted part only (e.g. "x y" gave "x ", "ceil(x)" gave "").
- mt_polish: infix "!&" gives "&!" (and-not); the "!&" it gave was not known by the luts (since 2.2.4).
- Linux (and other LP64 builds): mt_luts mode "avg" and mt_lutf mode "average" round the average again, they
  truncated it (8 and 16 bit).
- mt_infix: an invalid expression ("", "x +", "x y") is an error instead of a crash or garbage; dup and swap
  are expanded ("x dup *" gives "(x*x)", was "(*dup(x))").
- "%" is the floating point remainder (fmod) as in AviSynth+ Expr: 5.7 % 2.2 = 1.3, -5.7 % 2.2 = -1.3. It was the
  integer remainder of the rounded operands (5.7 % 2.2 gave 0), a divisor below 0.5 crashed. x % 0 is NaN
  (0 in integer formats).
- mt_logic mode "andn": ~x & y everywhere, as the SIMD versions always computed it (x86 pandn operand order,
  since MaskTools 2.0). The C version gave x & ~y (the old documentation), on narrow planes, with sse2=false and
  in the last columns of a row when the width was not a multiple of 16 (32 with AVX2). Documentation corrected.
- mt_expand, mt_inpand mode="square" with thY/thC < 255 (8 bit SSE2, 10-16 bit SSE4.1/AVX2): the result could
  go up to 2 * thY away from the source pixel instead of thY (two separately limited passes); now as in C.
- YV411 (4:1:1): the chroma planes were processed as 4x wide and 1/4 high (swapped ratios): wrong output in every
  filter, rows left unwritten (since 2.2.1).
- mt_edge prewitt, hprewitt, cartoon at 10-14 bit (SSE2, AVX2): values above the high threshold gave more than the
  max pixel value instead of the max; now as in C.
- mt_edge custom matrix at 10-14 bit (SSE2/SSE4.1/AVX2): sums above 32767 + half range gave 0 instead of the max
  pixel value; 16 bit with thY1/thC1 = 65535: prewitt, hprewitt, cartoon, custom matrix values above 65535 gave 0
  instead of the max. Now as in C.
- mt_edge 8 bit and mt_expand, mt_inpand, mt_inflate, mt_deflate 8 bit: a plane exactly one vector wide (16, AVX2
  32 pixels) got wrong left/right edge pixels (memory dependent for mt_edge); such planes use the C version.
- mt_merge float luma=true on a CPU without SSE3: illegal instruction (SSE3 in the SSE2 version).
- stacked=true: processed internally as native 16 bit (the SIMD versions serve it too). Now possible: mt_lut
  realtime=true, mt_merge cplace="top_left" and 4:1:1 (YV411). mt_lut use_expr is ignored for stacked clips (Expr
  computed the double height 8 bit clip: wrong result).
- mt_luts, mt_lutsx, mt_lutspa: an invalid expression or scale_inputs could crash (heap corruption) instead of
  giving the error message; mt_lutsx did not free its pixel list (memory leak).
- mt_luts 16 bit with weights (wexpr) and realtime=false on 64 bit non-x86 (aarch64): the weights were ignored.
- mt_lutxyza, mt_lutxy, mt_lutf, mt_luts realtime=false: an error message when the lut memory cannot be allocated
  (4 GB for mt_lutxyza, 8 GB for 16 bit), instead of an unhandled exception.
- mt_expand, mt_inpand (modes square, horizontal, both) SSE2/SSE4.1/AVX2: when the width was not a multiple of 16
  (8 bit) or 8/16 (16 bit), read up to a vector past the end of the row (possible crash at the frame end).
- mt_edge custom matrix, SSE2/SSE4.1: planes 8 or 16 pixels wide (8 bit) or 8 wide (16 bit) were read outside
  the plane (possible crash); such planes now use the C version.
- mt_mappedblur 10-16 bit and float, MSVC builds (the release DLLs): the mask was read from wrong rows (1/2 or 1/4
  row apart), partly from the row padding (wrong output, unstable
  on some widths); now as in the ClangCL and Linux builds.
- offx/offy/w/h windows: a window empty after the snapping to the chroma grid (e.g. YV12 offx=0, w=1) crashed
  every filter; now nothing is processed.
- mt_convolution: works on planes and windows smaller than the kernel (a 1 pixel wide window corrupted the
  heap, smaller clips were an error). Even vertical kernels: the last rows use the clamped bottom row (was the
  row above it).
- mt_edge, mt_mappedblur, mt_expand, mt_inpand, mt_inflate, mt_deflate: planes or windows 1 pixel wide or high
  were read outside the plane; 1 pixel high ones also got a row written below them (possible crash).
- mt_lutspa: windows not aligned to the chroma grid read outside the lut (wrong output, possible crash); the
  alpha plane of YUVA 4:2:0 / 4:2:2 / 4:1:1 too (since alpha support).
- mt_hysteresis: a window crashed (wrote past the frame end); unstable with multithreading (one shared buffer);
  corners=false at 10-32 bit gave wrong output.
- mt_binarize 10-16 bit mode "t x" and "x t": illegal instruction on CPUs without SSE4.1 (SSE4.1 in the SSE2
  version); now SSE2.
- mt_binarize: AVX2 versions (8, 10-16 bit, float).
- mt_logic 10-16 bit: the AVX2 version is used; it was registered with the speed of SSE2 and never selected.
- mt_luts float with weights: weight expressions are not implicitly clamped to 0..1. An uninitialized clamp
  setting could change the result depending on memory layout (present in 2.2.30).
- YA formats (Y + alpha) at 8/10/12/14/16 bit and float, including alpha controls and LUT expressions.
- YUV/YUVA 4:1:1 at all supported bit depths; mt_merge luma-mask reduction includes float and native
  10-16-bit SSE2/AVX2 kernels.
- YUV/YUVA 4:4:0 at all supported bit depths. mt_merge luma masks use vertical pair averaging for
  mpeg1/mpeg2, or a vertical 1-2-1 filter with top-edge replication for top_left/topleft;
  C, SSE2, AVX2 and NEON kernels, including float.
- YUV/YUVA 4:1:0 at all supported bit depths, with mt_merge luma masks at every placement.
- mt_merge: complete the remaining luma-mask SIMD paths: float 4:1:1 and top_left 4:2:0
  have SSE2/AVX2/NEON kernels; MPEG2 4:2:0/4:2:2 has NEON at every depth.
- Expressions: dup1..dup9 give `a b dup1` = `a b a`, as documented and in Expr; the top of the stack was overwritten, `a a a` (since 2.2.25).
- mt_edge, mt_motion: thC1, thC2 are scaled as differences, like thY1, thY2. On float YUV and RGB an explicit value with the default
  paramscale went negative and made the chroma mask all 1; paramscale="f32" on integer YUV was wrong too.
- mt_luts, mt_lutsx: an empty or odd pixels list is an error; empty crashed in "avg" (the default) and was meaningless otherwise.
- sse2, sse3, ssse3, sse4, avx, avx2 replaced by opt: 0 auto, 1 C, 2 SSE2, 3 SSE4.1, 4 AVX, 5 AVX2; aarch64 1 C, else NEON. Naming an old one is
  now an error. A, alpha, paramscale and the parameters after them move 5 positions forward.
- mt_motion: no overflow of the scene change sum on very wide planes (worst case from width 32776 at 16 bit).
- mt_motion: the scene change decision belongs to each frame. Under Prefetch the chroma of a frame could use another frame's decision;
  with luma not processed it read an uninitialized value, now luma still decides for the whole frame. 8 bit: the limit overflowed
  on very large frames (from thT=65 at 7680x4320), no scene change was detected.
- Expressions (mt_lut, mt_lutxy, mt_lutxyz, mt_lutxyza, mt_lutf, mt_luts, mt_lutsx, mt_lutspa, mt_polish, mt_infix):
  new sqrt (negative: 0), neg, sgn, not; pow, and, or, xor as names of ^, &, |, @.
  Booleans stay 1 / -1 and round stays as it was (Expr: 1 / 0, nearest even).
- Expressions: new width, height (of the plane), sx, sy, sxr, syr (position in it; sxr, syr 0..1 inclusive, 0 on a
  single column or row), frameno, time (frameno / (frame count - 1), 0 on a single frame). As in AviSynth+ Expr.
  With offx, offy, w, h they refer to the window: the result equals the filter on the cropped clip. x, y of mt_lutspa
  are unchanged. Such expressions are computed per pixel whatever realtime says, and with use_expr=2 go to Expr.
- use_expr with offx, offy, w or h: one error message, only the full frame can be passed to Expr.
- aarch64 with NEON on Linux and Windows (llvm-mingw).
- More AVX2: mt_motion, mt_average and the morphological filters at all depths.
- AviSynth 2.5 is no longer supported, the minimum is classic AviSynth 2.6. Boost is no longer used, mt_polish and mt_infix are in every build.
- Reference: masktools/documentation/masktools2.md.

**v2.2.30 (20220218)
- mt_hysteresis new parameter: 
    bool corners (default true)
  When set to false, mask expanding is ignoring corner-only connections.
  corners = true (default, old working mode):
    + + +
    + O +
    + + +
  false:
    - + -
    + O +
    - + -
  Where the '+' pixels indicate neighbors of the central 'O'
  (See https://github.com/pinterf/masktools/issues/23 )
- Add atan2 two operand function to lut expression evaluation. Calls atan2(y,x) math library function.

**v2.2.29 (20211116)
- floating point number formats are local independent again. Regression since 2.2.26
- mt_merge accept "top_left" chroma placement ("topleft" is still valid)
- (avx2: stop using _mm256_zeroupper - compilers are do that automatically, 
  mainly because LLVM (12.0) generated suboptimal save-restore-all-xmm registers in
  prologue and epilogue when encountered with it.)

**v2.2.28 (20211005)
- allow TAB, CR and LF characters as symbol separators in expression strings

**v2.2.27 (20210909)
- fix zero=false case for shape helper function (mt_rectangle, mt_circle, mt_diamond etc...)
- lut expressions: report obvious script error (unbalanced stack, invalid keyword or variable, etc)
- mt_lut: reuse LUTs across planes if they are the same like in e.g. mt_lutxyz.
- 1D LUT expressions: occupy only the necessary size for 10-14 bit LUT tables (was: buffer was always reserved for 16 bit data)
- mt_merge: error is luma=false, mask is greyscale but clip is not greyscale
- mt_merge new parameter hint for chroma placement when luma=true and 4:2:0: "topleft"
  "topleft" is a new option for 4:2:0 videos only 
  
  Refreshing memories for 'cplace' kinds:
  
  String 'cplace': possible values "mpeg1", "mpeg2" (default) or "topleft"
  ("mpeg1" is center and "mpeg2" is left placement)
  Parameter is effective only for 420 and 422 formats (topleft is 420-only), otherwise ignored.

  Default "mpeg1" is using fast 2x2 pixel (1x2 for 4:2:2) averaging when converting a 4:4:4 mask to a 4:2:0 or 4:2:2 format (old behaviour)
  420 schema:
   +------+------+
   | 0.25 | 0.25 |
   |------+------|
   | 0.25 | 0.25 |
   +------+------+

  "mpeg2" is using 2x3 (1x3 for 4:2:2) pixel weighted averaging when converting a 4:4:4 mask to a 4:2:0 or 4:2:2 format
  420 schema:
   ------+------+-------+
   0.125 | 0.25 | 0.125 |
   ------|------+-------|
   0.125 | 0.25 | 0.125 |
   ------+------+-------+

  "topleft" is using 3x3 pixel weighted averaging when converting a 4:4:4 mask to a 4:2:0 format
  
   1/16  | 1/8  | 1/16  |
   ------+------+-------+
   1/8   | 1/4  | 1/8   |
   ------|------+-------|
   1/16  | 1/8  | 1/16  |
   ------+------+-------+

- get the source built with LLVM (not clangCl) again

**no new version (20210209) does not appear in release, just another test fork update
- Fetching the new 'cuda' branch from Nekopanda's masktools fork
  git fetch git://github.com/nekopanda/masktools.git cuda:cuda
  (Note: _only_ for experimenting and adjusting to latest Avisynth+ Cuda-aware version. 
   Cuda version implements only mt_lut.)

- Source syntax update for GCC (20201229)
- CMake build environment, builds on Linux, at least on my (pinterf) Ubuntu 19.10 WSL
  (INTEL_INTRINSICS handling not implemented in the source, so it compiles only for Intel at the moment)
  (Neither is boost library incorporated: mt_infix is unavaliable, MT_HAVE_BOOST_SPIRIT is not defined - maybe later)
  For build instructions see the end of this readme.

  git clone https://github.com/pinterf/masktools.git
  cd masktools
  mkdir build
  cd build
  cmake ..
  sudo make install

**v2.2.26 (20200904)
- mt_lut family: ignore "use_expr" parameter if "Expr" function does not exist in ancient AviSynth versions.
- more friendly error messages when an exception occurs during "Expr" call.

**v2.2.25 (20200813)
- Introduce swap1 to swap9, dup0 to dup9 keywords besides dup and swap
- mt_infix duplicates the expression string when 'dup' found.
  swap, swap1..9 and dup1..9 in mt_infix appear as functions
  This is in no way a proper solution and does not result in valid expression.
  The effect of stack operations on the resulting expression is not easy to stringify.
  At least now dup and swap will appear somehow in infix.
- mt_edge, mt_logic: fix possible use of AVX2 instructions on AVX-only processors (such as on i7-3930)

**v2.2.24 (20200619)
- fix: mt_convolution divbyzero when greyscale + forced U=3,V=3 (process chroma)

**v2.2.23 (20200513)
- fix mt_polish for nested ternary operators
  Example: 
    mt_polish("x > 128 ? 255 : x > 64 ? 128 : 0")
  results: 
    "x 128 > 255 x 64 > 128 0 ? ?" 
  (was: "x 128 > 255 x 64 > ? 128 0 ?")
 
**v2.2.22 (20200422)
- Add AVX2 to mt_expand and mt_inpand 10-16 bit processing
- Add Avisynth+ interface V8 frame property copy support (test)

**v2.2.21 (20200410)
- Fix: mt_hysteresis for 10+ bits 

**v2.2.20 (20200303)
- new "yscalef" and "yscaleb" keywords similar to "scalef" and "scaleb" but scaling is forced to use rules for Y (non-UV) planes 
- mt_lutspa: add parameters "scale_inputs", "clamp_float" and "clamp_float_UV"
- new predefined constants in expressions: yrange_min, yrange_half, yrange_max
  Unlike range_min, range_half, range_max the y-prefixed versions do not depend on whether the currently
  processed plane is luma(Y) or chroma(U/V). They are always returning the values of Y plane.
- new parameter to lut functions: Boolean "clamp_float_UV": default false, as an addition to clamp_float (since v2.2.20).
- New: Parameter "scale_inputs" can now be set to "floatUV" (chroma pre-shift by 0.5 for 32 bit float pixels)
- Fix: mt_motion mask contained out-of range pixels for 10-14 bit inputs
- Fix: mt_edge convolution mode incorrect result on 10-32 bits when normalizer weight is not power of 2
  e.g. mode = "1 2 1 0 0 0 -1 -2 -1 15.0" (normalizer: 10th parameter or maximum of (sum_positive/sum_negative))
  Note: when processing chroma (u=3,v=3) on 32 bit float clip will result 0..1.0 ranged masks in chroma planes as well.
- Source: 
  - add LLVM-clangCl to VC project configuration (built-in clang support in VS2019)
  - fix LLVM build for VS2019
  - silence many warnings
  - project configurations: use current SDK version 10.0.18362.0

**v2.2.19 (20190710 - not released)
- Fix: mt_infix to recognize scaleb and scalef
- Fix: mt_infix to recognize ymin, ymax, abs, atan, etc... tokens beginning with 'a' and 'y' were not converted
- Move project to VS2019 v142 toolset, xp builds still at v141_xp
- update current Avisynth+ headers
- update source to use boost 1.70 lib v142 for non-xp builds

**v2.2.18 (20180905)
- mt_merge: fix right side artifacts for non-mod16 width, AVX2 and luma=false (regression in 2.2.16)
- mt_adddiff: fix 32 bit chroma (still used 0.5 centered chroma instead of 0.0)

**v2.2.17 (20180710)
- mt_convolution: check plane dimensions to exceed convolution horizontal/vertical size
- lut functions: plane order to RGBA from BGRA like in Expr.
  expr parameters y-u-v-a naming matches now to r-g-b-a
- for luts: use_expr=3: always send expression(s) to Expr

**v2.2.16 (20180702)
- mt_merge new parameter hint for chroma placement when luma=true and 4:2:0/4:2:2
  String 'cplace': possible values "mpeg1" or "mpeg2" (default)
  Parameter is effective only for 420 and 422 formats, otherwise ignored.
  Default "mpeg1" is using fast 2x2 pixel (1x2 for 4:2:2) averaging when converting a 4:4:4 mask to a 4:2:0 or 4:2:2 format (old behaviour)
  420 schema:
   +------+------+
   | 0.25 | 0.25 |
   |------+------|
   | 0.25 | 0.25 |
   +------+------+

  "mpeg2" is using 2x3 (1x3 for 4:2:2) pixel weighted averaging when converting a 4:4:4 mask to a 4:2:0 or 4:2:2 format
  420 schema:
   ------+------+-------+
   0.125 | 0.25 | 0.125 |
   ------|------+-------|
   0.125 | 0.25 | 0.125 |
   ------+------+-------+

- 32 bit float U and V chroma channels are now zero based (+/-0.5 for full scale). Was: 0..1, same as luma
  Since internal format changed, use Avisynth+ r2724 or newer for this masktools2 2.2.16.
  Affected predefined expression constants when plane is U or V: 
  cmin and cmax (limited range (16-128)/255 and (240-128)/255 instead of 16/255.0 and 240/255.0
  range_max: 0.5 instead of 1.0
  new: introduce range_min: -0.5 for float U/V chroma, 0 otherwise
  range_half (0.0 instead of 0.5)
  (range_size remained 1.0)
- New expression syntax for Lut expressions: autoscale any input (x,y,z,a) bit depths to 8-16 bits for internal 
  expression use. The primary reason of this feature is the "easy" usage of formerly written 8 bit optimized expressions.

  New parameters for lut functions: 
    String "scale_inputs": "all","allf","int","intf","float","floatUV","floatf","none", default "none"
  and 
    Boolean "clamp_float": default false, but treated as always true (and thus ignored) when scale_inputs involves a float autoscale.
  and 
    Boolean "use_expr": default 0, calls fast JIT-compiled "Expr" in Avisynth+ for mt_lut, lutxy, lutxyz, lutxyza
    0: no Expr, use slow internal realtime calc if needed (as before)
    1: call Expr for bits>8 or lutxyza
    2: call Expr, when masktools would do its slow realtime calc (see 'realtime' column in the table above)

  Extends and replaces experimental clamp_xxxx keywords.

**v2.2.15 (skipped, test versions)

**v2.2.14 (20180225)
- Fix: mt_convolution invalid instruction on processors below SSE4.1

**v2.2.13 (20180201)
- Fix: rare crash in multithreading environment at the very first frames 
  (keeping XP compatibility with /Z:threadsafeinit- caused troubles!)
- mt_edge: AVX2 (1.4-1.9x speed) for 8 and 10-16 bits
- fix: "chroma" parameter with negative (memset) values were not working properly for 10-14 bits and 32bit float

**v2.2.12 (20180107)
- Fix: mt_merge 10-16 bits: right side artifacts when clip is non-mod 8 (non-AVX2) or mod16 (AVX2) widths

**v2.2.11 (20180105)
- Fix: mt_merge luma=true: broken output when: 8-16 bits AVX2, 32 bit float: SSE2, AVX
- move project to VS2017, vs141_xp toolset

**v2.2.10 (20170612)
- Fix: luts internal buffer overflow (crash)
- Speed: mt_inpand/mt_expand: 10-16 bits SSE4 (10-15x speed)
- Speed: mt_inflate/mt_deflate 10-16 bits SSE4 (4x speed)

**v2.2.9 (20170608)
- Add "none" and "ignore" to valid values for "chroma" and "alpha" parameters.
- Report error for invalid "chroma" or "alpha" parameter values instead of exception

**v2.2.8 (20170427)
- Fix: "chroma" and "alpha" parameter should be scaled like "Y","U","V" and "A" when providing negative (memset) values

**v2.2.7 (20170421)
- fix: mt_edge 10,12,14 bits: clamp mask value from 65535 to 1023 (10 bits), 4095 (12 bits) and 16383 (14 bits)
- fix: mt_merge 10-16 bits + non mod-16 width + luma=true + 4:2:2 colorspace, correct right side pixels
- fix: mt_merge 8 bit clips: keep original pixels from clip1/2 when mask is exactly 0 or 255
- YUVA, RGBAP support 8-32 bits
  - "A" parameter like "Y", "U" and "V". Default value for "A" is 1 (do nothing, same as for "U" and "V")
  - "alpha" parameter like "chroma" - overrides default plane mode
  - aExpr parameter for lut-type filters like uExpr, and vExpr
  - awExpr parameter for mt_luts like uwExpr, and vwExpr
  - dual signature filters (both integer and float) are provided in separate binaries
    In some cases specifying two different parameter lists with the same variables can cause troubles.
    (dual signature version can be used to override an earlierly loaded different masktools version
    (e.g. a 2.5 plugin) by defining the filters with both integer parameters _AND_ the new float parameter lists)
- Make "paramscale" to work consistent with all filters and parameters:
  parameters "Y","U","V" and "A" negative (memset) values are scaled automatically to the current bit depth from a default 8-bit value.
- New plane mode: 6 (copy from fourth clip) for "Y", "U", "V" and "A"
  New "chroma" and "alpha" plane mode override: "copy fourth"
  Use for mt_lutxyza which has four clips

**v2.2.6 (20170401)
- fix: >>u operator AV error

**v2.2.5 (20170330)
- Change #F and #B operators to @B and @F (# is reserved for Avisynth in-string comment character)
- Alias scaleb for @B
- Alias scalef for @F
- New: automatic scaling of parameters (threshold-like, sc_value) from the usual 8 bit range
  Scripts need no extra measures to work for all bit depths with the same "command line"
- New parameter "paramscale" for filters working with threshold-like parameters
  Filters: mt_binarize, mt_edge, mt_inpand, mt_expand, mt_inflate, mt_deflate, mt_motion, mt_logic, mt_clamp
  paramscale can be "i8" (default), "i10", "i10", "i12", "i14", "i16", "f32" or "none" or ""
  Using "paramscale" tells the filter that parameters are given at what bit depth range.
- dual function signatures (float and int), for backward compatibility with integer-type parameter list, prevent usage of earlier plugin-loaded masktools version
- keep old parameter ordering: parameters which are non-existant in 2.0b1 are inserted at the end of the parameter list, not before the common parameters Y, U, V
- new: "swap" keyword in expressions
- new: "dup" keyword in expressions
- a bit faster realtime lut calculation for 10+ bit depths
  
**v2.2.4 (20170304)
- mt_polish to recognize:
  - new v2.2.x constants and variables
    a, bitdepth, sbitdepth, range_half, range_max, range_size, ymin, ymax, cmin, cmax
  - v2.2.x scaling functions (written as #B(expression) and #F(expression) for mt_polish)
    #B() #F
  - single operand unsigned and signed negate introduced in 2.0.48
    ~u and ~s (written as ~u(expression)and ~s(expression) for mt_polish)
  - other operators introduced in 2.0.48:
    @, &u, |u, °u, @u, >>, >>u, <<, <<u, &s, |s, °s, @s, >>s, <<s
- mt_infix: don't put extra parameter after #F( and #B(
- new expression syntax: auto scale modifiers for float clips (test for real.finder):
  Keyword at the beginning of the expression:
  - clamp_f_i8, clamp_f_i10, clamp_f_i12, clamp_f_i14 or clamp_f_i16 for scaling and clamping
  - clamp_f_f32 or clamp_f: for clamping the result to 0..1
  
  Input values 'x', 'y', 'z' and 'a' are autoscaled by 255.0, 1023.0, ... 65535.0 before the expression evaluation,
  so the working range is similar to native 8, 10, ... 16 bits. The predefined constants 'range_max', etc. will behave for 8, 10,..16 bits accordingly.
  
  The result is automatically scaled back to 0..1 _and_ is clamped to that range.
  When using clamp_f_f32 (or clamp_f) the scale factor is 1.0 (so there is no scaling), but the final clamping will be done anyway.
  No integer rounding occurs.

**v2.2.3 (20170227)**
- mt_logic to 32 bit float (final filter lacking it)
- get CpuInfo from Avisynth (avx/avx2 preparation)
  Note: AVX/AVX2 prequisites
  - recent Avisynth+ which reports extra CPU flags
  - 64 bit OS (but Avisynth can be 32 bits)
  - Windows 7 SP1 or later
- mt_merge: 8-16 bit: AVX2, float:AVX
- mt_logic: 8-16 bit: AVX2, float:AVX
- mt_edge: 10-16 bit and 32 bit float: SSE2/SSE4 optimization
- mt_edge: 32 bit float AVX
- new: mt_luts: weight expressions as an addon for then main expression(s) (martin53's idea)
    - wexpr
    - ywExpr, uwExpr, vwExpr
  If the relevant parameter strings exist, the weighting expression is evaluated
  for each source/neighborhood pixel values (lut or realtime, depending on the bit depth and the "realtime" parameter). 
  Then the usual lut result is premultiplied by this weight factor before it gets accumulated. 

**v2.2.2 (20170223)** completed high bit depth support
- All filters work in 10,12,14,16 bits and float (except mt_logic which is 8-16 only)
- mt_lutxyza 4D lut available with realtime=false! (4 GBytes LUT table, slower initial lut table calculation)
  Allowed only on x64. When is it worth?
  Number of expression evaluations for 4G lut calculation equals to realtime calculation on 
  2100 frames (1920x1080 plane). Be warned, it would take be minutes.
- mt_gradient 10-16 bit / float
- mt_convolution 10-16 bit / float
- mt_motion 10-16 bit / float
- mt_xxpand and mt_xxflate to float
- mt_clamp to float
- mt_merge to float
- mt_binarize to float
- mt_invert to float
- mt_makediff and mt_adddiff to float
- mt_average to float
- Expression syntax supporting bit depth independent expressions: 
  Added configuration keywords i8, i10, i12, i14, i16 and f32 in order to inform the expression evaluator 
  about bit depth of the values that are to scale by #B and #F operators.
 
**v2.2.1 (20170218)** initial high bit depth release

- project moved to Visual Studio 2015 Update 3
  Requires VS2015 Update 3 redistributables
- Fix: mt_merge (and probably other multi-clip filters) may result in corrupted results 
  under specific circumstances, due to using video frame pointers which were already released from memory
- no special function names for high bit depth filters
- filters are auto registering their mt mode as MT_NICE_FILTER for Avisynth+
- Avisynth+ high bit depth support (incl. planar RGB, but color spaces with alpha plane are not yet supported)
  For accepted bit depth, see table below.
- YV411 (8 bit 4:1:1) support
- mt_merge accepts 4:2:2 clips when luma=true (8-16 bit)
- mt_merge accepts 4:1:1 clips when luma=true
- mt_merge to discard U and V automatically when input is greyscale
- new: mt_lutxyza. Accepts four clips. 4th variable name is 'a' (besides x, y and z)
- expression syntax supporting bit depth independent expressions
  - bit-depth aware scale operators #B and #F
  - pre-defined bit depth aware constants
    bitdepth (automatic silent parameter of the lut expression)
    range_half, range_max, range_size, ymin, ymax, cmin, cmax
  - parameter "stacked" (default false) for filters with stacked format support
  - parameter "realtime" for lut-type filters to override default behaviour
  
Build instructions
==================

Windows, Visual Studio 2026 (MSVC or ClangCL)
* Open `masktools.slnx`. Configurations: `Release` (MSVC), `Rel_Clang` (ClangCL, needs the "C++ Clang tools for Windows"
  component), `Debug`; platforms `Win32` and `x64`. From the command line:

        msbuild masktools.slnx -m -p:Configuration=Release;Platform=x64

* Output: `Build\x64\Release\masktools2.dll` (and `Build\Win32\...`, `Build\x64\Rel_Clang\...`).
* The MSVC builds need the Microsoft Visual C++ Redistributable.

Windows XP (`v141_xp`)
* `masktools.sln` (Visual Studio 2022) with the `Release_v141_xp` configuration. It needs the v141_xp toolset installed.

Windows with CMake: MinGW GCC or Clang (x64), llvm-mingw (ARM64)
* x64, from the project root:

        cmake -S . -B build/mingw -G Ninja -DCMAKE_BUILD_TYPE=Release
        cmake --build build/mingw

* ARM64 with llvm-mingw (https://github.com/mstorsjo/llvm-mingw, UCRT release), built on an x64 PC:

        cmake -S . -B build/arm64_mingw -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/llvm-mingw-aarch64.cmake -DLLVM_MINGW_ROOT=C:/path/to/llvm-mingw-ucrt-x86_64
        cmake --build build/arm64_mingw

* A MinGW or llvm-mingw built DLL needs a host built the same way: it cannot be used with an MSVC built AviSynth+.

Linux (x86-64 or aarch64), with GCC or Clang
* Clone and build:

        git clone https://github.com/pinterf/masktools
        cd masktools
        cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
        cmake --build build

* The SIMD code follows the target: `ENABLE_INTEL_SIMD` on x86, `ENABLE_NEON_SIMD` on aarch64. For a C only build turn the
  option of the target off, e.g. `-DENABLE_INTEL_SIMD=OFF`.
* Output: `build/masktools/libmasktools2.so`
* Install into the AviSynth+ plugin directory (`lib/avisynth` under the install prefix), and uninstall:

        sudo cmake --install build
        sudo cmake --build build --target uninstall

macOS: the CMake build above (`libmasktools2.dylib`) is supported, but has not been tested.
