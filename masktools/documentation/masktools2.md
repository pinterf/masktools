# MaskTools 2 reference

MaskTools 2 version 2.3.0, for AviSynth+ and classic AviSynth 2.6.

Original authors: Kurosu, Manao, mg262 (MaskTools 2.0), tp7 (MaskTools 2 repository). High bit depth, later
formats and ports: pinterf.

Project: <https://github.com/pinterf/masktools> (development branch `16bit`) ·
Forum: <https://forum.doom9.org/showthread.php?t=174333> ·
Article by tp7: <http://tp7.github.io/articles/masktools/>

This document is the complete reference. It replaces the original manual (`mt_masktools.html`, kept for history)
and the "differences to MaskTools 2.0" list. The full version history is in the project's `README.md`.

## Contents

1. [What MaskTools does](#1-what-masktools-does)
2. [Platforms and requirements](#2-platforms-and-requirements)
3. [Formats](#3-formats)
4. [Common parameters](#4-common-parameters)
5. [Filter list](#5-filter-list)
6. [Mask creation](#6-mask-creation): mt_edge, mt_motion
7. [Mask operations](#7-mask-operations): mt_invert, mt_binarize, mt_logic, mt_hysteresis
8. [Merging](#8-merging): mt_merge, luma masks on subsampled chroma
9. [Morphological filters](#9-morphological-filters): mt_expand, mt_inpand, mt_inflate, mt_deflate
10. [Lookup and expression filters](#10-lookup-and-expression-filters): mt_lut and family
11. [Expression language](#11-expression-language)
12. [Comparison with AviSynth+ Expr](#12-comparison-with-avisynth-expr)
13. [Difference filters](#13-difference-filters): mt_makediff, mt_adddiff, mt_average, mt_clamp
14. [Convolutions](#14-convolutions): mt_convolution, mt_mappedblur, mt_gradient
15. [Helpers](#15-helpers): shape strings, mt_polish, mt_infix
16. [Stacked 16 bit](#16-stacked-16-bit)
17. [Changes in 2.3.0](#17-changes-in-230)

## 1. What MaskTools does

MaskTools is a set of filters to create, manipulate and use masks. In video processing a mask gives a relative
importance to each pixel: for example, select only the edges of a picture, sharpen a copy, and merge the sharpened
copy back only where the mask is set.

Every filter treats a clip as independent planes. A mask clip therefore holds one mask per plane, and each plane
can be processed, copied, filled or ignored separately ([plane modes](#plane-modes-y-u-v-a)).

Masks use the full range on **every** plane: 0 to the maximum pixel value for integer formats, 0 to 1.0 for float.
U and V are included, so a float mask is not in the signed -0.5..0.5 range of float chroma.

## 2. Platforms and requirements

### AviSynth host

| Host | Notes |
|---|---|
| AviSynth+ | The recommended host. High bit depth, planar RGB and alpha need it. |
| Classic AviSynth 2.6 | The minimum. 8 bit Y8, YV12, YV16, YV24 and YV411 only, plus stacked 16 bit. AviSynth 2.5 is no longer supported. |

YUV(A) 4:4:0 and 4:1:0, 4:1:1 above 8 bit and with alpha, and the YA formats exist only in AviSynth+ versions with
interface V13 or newer. On a host with frame properties (interface V8 or newer) they are copied from the first clip, and mt_merge reads
`_ChromaLocation` for its [cplace](#cplace-where-the-chroma-samples-sit).

The filters register as `MT_NICE_FILTER`, so `Prefetch` needs no `SetFilterMTMode` call.

### Builds

| Platform | Compiler | CPU code |
|---|---|---|
| Windows x86, x64 | MSVC, ClangCL | SSE2, AVX, AVX2, chosen at run time |
| Windows x86, x64, XP toolset (`v141_xp`) | MSVC | The same code; Windows XP reports no AVX, so SSE2 runs there |
| Windows x64 | MinGW GCC, Clang | SSE2, AVX, AVX2, chosen at run time |
| Windows ARM64 | llvm-mingw | NEON |
| Linux x86-64 | GCC, Clang | SSE2, AVX, AVX2, chosen at run time |
| Linux aarch64 | GCC, Clang | NEON |
| macOS x86-64, not tested | Clang | SSE2, AVX, AVX2, chosen at run time |
| macOS arm64, not tested | Clang | NEON |
| Other | any C++17 compiler | C |

The file is `masktools2.dll` on Windows (x86, x64, ARM64), `libmasktools2.so` on Linux and `libmasktools2.dylib` on
macOS. The MSVC builds need the Microsoft Visual C++ Redistributable. The ARM64 build is made with llvm-mingw and
needs no extra DLLs. The macOS build is supported by the CMake project, but it has not been tested.

A C++ plugin must match the host's ABI. A MinGW or llvm-mingw built DLL cannot be used with an MSVC-built AviSynth+,
so on Windows ARM64 the host must be an llvm-mingw build too.

### CPU selection

Every x86 build requires SSE2. Each filter picks the fastest version for each plane from the CPU flags reported by
the host, so `SetMaxCPU()` limits it as usual: `SetMaxCPU("none")` gives the plain C code everywhere, on ARM too. A single
call can be limited with [opt](#opt).

All integer versions give the same result as C. Float versions can differ from C by rounding only (the last bit),
because some older x86 float kernels compute the same formula in another order. mt_merge's 4:1:1 and 4:1:0 luma masks
are identical to C in float too.

These filters and paths use C on every CPU: all `mt_lut*` expression evaluation, `mt_convolution`, `mt_mappedblur`,
`mt_hysteresis`, `mt_gradient`, and the morphological filters at 32 bit float.

## 3. Formats

Any of the following at 8, 10, 12, 14, 16 bit integer and 32 bit float, except where noted:

| Family | AviSynth `pixel_type` examples | Planes |
|---|---|---|
| Greyscale | `Y8`, `Y10`..`Y16`, `Y32` | Y |
| Greyscale + alpha | `YA8`, `YA10`..`YA16`, `YAS` | Y, A |
| 4:4:4, 4:2:2, 4:2:0 | `YV24`, `YV16`, `YV12`, `I420`, `YUV420P10`, `YUV444PS` | Y, U, V |
| 4:1:1 | `YV411`, `YUV411P10`, `YUV411PS` | Y, U, V |
| 4:4:0 | `YUV440`, `YUV440P10`, `YUV440PS` | Y, U, V |
| 4:1:0 | `YUV410`, `YUV410P10`, `YUV410PS` | Y, U, V |
| The same with alpha | `YUVA420`, `YUVA420P10`, `YUVA411PS`, `YUVA410` | Y, U, V, A |
| Planar RGB | `RGBP8`, `RGBP10`..`RGBP16`, `RGBPS` | R, G, B |
| Planar RGB + alpha | `RGBAP8`, `RGBAP10`..`RGBAP16`, `RGBAPS` | R, G, B, A |

Packed formats (`RGB24`, `RGB32`, `RGB48`, `RGB64`, `YUY2`) are an error: convert them first.

**Planar RGB:** the parameters named after Y, U, V act on R, G, B in this order (`y`, `u`, `v`, `yExpr`, `uExpr`,
`vExpr`, `thY`, `thC`...). `u` and `v` default to 1, so **G and B are not processed by default**: use `u=3, v=3`.

**Float pixel values:** luma, RGB and alpha are 0..1.0; U and V are -0.5..0.5, centred on 0.0. This matches AviSynth+
r2724 and newer.

### Which parameters each filter has

"Realtime default" is the bit depth where an expression is evaluated per pixel by default instead of through a lookup
table ([realtime and memory](#realtime-and-lookup-tables)).

| Filter | stacked | realtime default | use_expr |
|---|---|---|---|
| mt_invert | no | | |
| mt_binarize, mt_logic | yes | | |
| mt_inflate, mt_deflate, mt_inpand, mt_expand | yes | | |
| mt_lut | yes | float | yes |
| mt_lutxy | no | 14 bit and up | yes |
| mt_lutxyz | no | 10 bit and up | yes |
| mt_lutxyza | no | always | yes |
| mt_luts, mt_lutf | no | 14 bit and up | no |
| mt_lutsx | no | 10 bit and up | no |
| mt_lutspa | no | | no |
| mt_merge | yes | | |
| mt_makediff, mt_adddiff, mt_average, mt_clamp | yes | | |
| mt_convolution, mt_mappedblur, mt_gradient | no | | |
| mt_motion, mt_edge, mt_hysteresis | no | | |

## 4. Common parameters

All filters except the [helpers](#15-helpers) share these. They come after the filter's own parameters, in this order:
`Y, U, V, chroma, offX, offY, w, h, opt, A, alpha, paramscale`.
Parameter names are not case sensitive, so `y=3` and `Y=3` are the same.

### Plane modes: Y, U, V, A

`Y` defaults to 3, `U`, `V` and `A` to 1.

| Value | Meaning |
|---|---|
| -255..0 | Fill the plane with the negated value (scaled to the bit depth, see [paramscale](#paramscale)) |
| 1 | Not processed. See below. |
| 2 | Copy the plane of the first clip |
| 3 | Process the plane |
| 4 | Copy the plane of the second clip (when the filter has one) |
| 5 | Copy the plane of the third clip (when the filter has one) |
| 6 | Copy the plane of the fourth clip (when the filter has one) |

A mode naming a clip the filter does not have falls back to the last clip it has.

The result of mode 1 depends on the filter:

- **The first clip's plane stays** in filters working in place: mt_invert, mt_binarize, mt_logic, mt_merge, mt_motion,
  mt_makediff, mt_adddiff, mt_average, mt_clamp, and all `mt_lut*` except mt_lutspa.
- **The plane is undefined** (garbage) in the others: mt_edge, mt_expand, mt_inpand, mt_inflate, mt_deflate,
  mt_convolution, mt_mappedblur, mt_gradient, mt_hysteresis, mt_lutspa. Use `u=2, v=2` to keep it.

To read such a clip as a picture, do not leave garbage planes in it. Most scripts use luma-only masks for this reason.

### chroma, alpha

String overrides of `U` + `V`, and of `A`. Takes precedence over the numeric modes.

| Value | Same as |
|---|---|
| `"process"` | 3 |
| `"copy"`, `"copy first"` | 2 |
| `"copy second"` | 4 |
| `"copy third"` | 5 |
| `"copy fourth"` | 6 |
| `"none"`, `"ignore"` | 1 |
| `"-128"` (a number) | Fill with 128 |

The fill number must be zero or negative, anything else is an error. The original manual's `chroma="128"` is an error.

### offX, offY, w, h

The processed box: `offX`, `offY` default to 0, `w`, `h` to -1. A negative `w` or `h` extends the box to the right or
bottom edge; so does a box reaching past it. The default is the whole picture.

On subsampled formats the box snaps inward to the chroma grid: both its start and its end are rounded down to a
multiple of the subsampling. On YV12, `offX=3, w=9` becomes `offX=2, w=8`. A box empty after this does nothing.

Outside the box the frame follows the same rule as plane mode 1: source pixels stay in the in-place filters, it is
undefined in the others.

A filter's result in a box equals the same filter on the clip cropped to that box. Neighbouring pixels are not read
across the box's edge: edges are treated like plane edges.

`use_expr` cannot be combined with a box, see [use_expr](#use_expr).

### paramscale

Threshold-like parameters and negative plane fill values are given on an 8 bit scale by default, and are scaled to
the clip's bit depth, so the same script works at any depth.

| Value | Parameters are given as |
|---|---|
| `"i8"` (default) | 8 bit: 0..255 |
| `"i10"`, `"i12"`, `"i14"`, `"i16"` | That bit depth |
| `"f32"` | Float: 0..1.0 |
| `"none"`, `""` | Already at the clip's bit depth, no scaling |

Scaling is by bit shift for YUV, with the maximum kept: 255 becomes 1023 at 10 bit, not 1020. It is a full range
stretch for RGB and for alpha fill values.

The scaled parameters are the thresholds and the like of mt_binarize, mt_edge, mt_motion (with `sc_value`), mt_expand,
mt_inpand, mt_inflate, mt_deflate, mt_logic and mt_clamp, plus the plane fill values. Not scaled: mt_gradient's `min`
and `max`, mt_convolution's `total`.

```avs
c.ConvertBits(16).mt_binarize(threshold=80*256, paramscale="i16") # threshold on a 16 bit scale
c.ConvertBits(16).mt_binarize(threshold=80)                       # the same: 8 bit scale by default
c.ConvertBits(32).mt_edge(thY1=0.1, thY2=0.1, paramscale="f32")   # thresholds in float
```

### opt

A debug parameter of each filter: the highest code path it may use. It never goes above what the CPU and `SetMaxCPU()` allow.

| Value | x86 | aarch64 |
|---|---|---|
| 0 (default) | The best available | NEON |
| 1 | C | C |
| 2 | Up to SSE2 | NEON |
| 3 | Up to SSE4.1 | NEON |
| 4 | Up to AVX | NEON |
| 5 | Up to AVX2 | NEON |

Other values are an error. Filters without the chosen path use the next lower one; no filter has an SSE4.1 path at present, so 3 gives
the same as 2. The numbering is this plugin's own: an `opt` parameter elsewhere, even within one plugin, can mean something else.

This replaces the former `sse2`, `sse3`, `ssse3`, `sse4`, `avx` and `avx2` parameters.

## 5. Filter list

| Group | Filter | Purpose |
|---|---|---|
| Mask creation | [mt_edge](#mt_edge) | Edge masks |
| | [mt_motion](#mt_motion) | Motion masks |
| Mask operations | [mt_invert](#mt_invert) | Inverts masks |
| | [mt_binarize](#mt_binarize) | Soft mask to hard mask |
| | [mt_logic](#mt_logic) | Combines masks with logic operators |
| | [mt_hysteresis](#mt_hysteresis) | Grows one mask into another |
| Merging | [mt_merge](#mt_merge) | Merges two clips according to a mask |
| Morphological | [mt_expand, mt_inpand](#mt_expand-mt_inpand) | Local maximum, minimum |
| | [mt_inflate, mt_deflate](#mt_inflate-mt_deflate) | Local average, up or down only |
| Expressions | [mt_lut](#mt_lut) | Expression on one clip |
| | [mt_lutxy, mt_lutxyz, mt_lutxyza](#mt_lutxy-mt_lutxyz-mt_lutxyza) | On two, three, four clips |
| | [mt_lutf](#mt_lutf) | With a value collected from a whole frame |
| | [mt_luts](#mt_luts) | With a neighbourhood |
| | [mt_lutsx](#mt_lutsx) | With two neighbourhood summaries |
| | [mt_lutspa](#mt_lutspa) | From pixel coordinates |
| Differences | [mt_makediff, mt_adddiff](#mt_makediff-mt_adddiff) | Subtracts two clips, adds a difference back |
| | [mt_average](#mt_average) | Average of two clips |
| | [mt_clamp](#mt_clamp) | Limits a clip between two others |
| Convolutions | [mt_convolution](#mt_convolution) | Separable convolution |
| | [mt_mappedblur](#mt_mappedblur) | 3x3 convolution limited by a map |
| | [mt_gradient](#mt_gradient) | Block difference map |
| Helpers | [shape strings](#shape-strings) | Coordinate lists for neighbourhoods |
| | [mt_polish, mt_infix](#mt_polish-mt_infix) | Converts expressions between notations |

## 6. Mask creation

### mt_edge

```
mt_edge(clip, string "mode", float "thY1", float "thY2", float "thC1", float "thC2", ...)
```

Creates an edge mask with a 3x3 operator.

| Parameter | Default | Meaning |
|---|---|---|
| mode | `"sobel"` | Operator, see below, or a custom kernel |
| thY1, thC1 | 10 | Low threshold (Y, chroma; alpha uses the Y one): this and below become 0 |
| thY2, thC2 | 10 | High threshold: above this becomes the maximum. Between them the value stays. |

With the default thresholds the result is a hard mask. Use `thY1=0, thY2=255` to see the raw values.

The neighbours are named by position:

```
a11 a21 a31     top left     top     top right
a12 a22 a32  =  left         center  right
a13 a23 a33     bottom left  bottom  bottom right
```

| mode | Value before thresholding (integer division truncates) |
|---|---|
| `"sobel"` | `abs(right + bottom - left - top) / 2`. **Not a real Sobel**, see below |
| `"sobel_real"` | `sqrt(Gx² + Gy²)`, the real Sobel, see below |
| `"roberts"` | `abs(2*center - right - bottom) / 2` |
| `"laplace"` | `abs(8*center - sum of the 8 neighbours) / 8` |
| `"cartoon"` | With `v = 2*top - center - top_right`: 0 when `v > 0`, else `-v` (negative edges only) |
| `"min/max"` | Local contrast: maximum minus minimum of the 9 pixels |
| `"hprewitt"` | The larger of the horizontal and vertical `1 2 1` Prewitt-style kernels |
| `"prewitt"` | The largest of four directional kernels. More robust. |

`"hprewitt"` is equivalent to:

```avs
mt_logic(c.mt_edge("1 2 1 0 0 0 -1 -2 -1 1", 0, 255), c.mt_edge("1 0 -1 2 0 -2 1 0 -1 1", 0, 255), mode="max")
```

`"prewitt"` is equivalent to:

```avs
a = mt_logic(c.mt_edge("1 1 0 1 0 -1 0 -1 -1 1", 0, 255), c.mt_edge("1 1 1 0 0 0 -1 -1 -1 1", 0, 255), mode="max")
b = mt_logic(c.mt_edge("1 0 -1 1 0 -1 1 0 -1 1", 0, 255), c.mt_edge("0 -1 -1 1 0 -1 1 1 0 1", 0, 255), mode="max")
mt_logic(a, b, mode="max")
```

**Custom kernel:** 9 coefficients row by row, and optionally a 10th, the divisor:

```avs
c.mt_edge("1 1 1 1 -8 1 1 1 1 8")
```

The value is `abs(sum of coefficient * pixel) / divisor`. Without a divisor it is the larger of the sum of the positive
coefficients and the sum of the negative ones; on integer clips this is also rounded up to a power of two. Integer clips
use integer coefficients. The divisor must be positive, and at most 32767 on integer clips.

Named modes have an equivalent kernel: `"sobel"` = `"0 -1 0 -1 0 1 0 1 0"`, `"roberts"` = `"0 0 0 0 2 -1 0 -1 0"`,
`"laplace"` = `"1 1 1 1 -8 1 1 1 1"`.

Plane edges use the nearest pixels.

#### "sobel" is a quick approximation, "sobel_real" is the real one

The default `"sobel"` is the historical MaskTools operator. It takes one combined difference of the four direct
neighbours, with no corners, no 1-2-1 weights, no separate directions and no magnitude, then halves it. It costs
almost nothing, which mattered when it was written, and it stays the default, unchanged, because existing scripts and
their thresholds depend on it.

It has weaknesses a real Sobel does not have. Differences of opposite sign cancel: a diagonal running from the top
right to the bottom left gives 0, since `right - left` and `bottom - top` have opposite signs there.

`"sobel_real"` computes the standard operator:

```
Gx = (top_right + 2*right + bottom_right) - (top_left + 2*left + bottom_left)
Gy = (bottom_left + 2*bottom + bottom_right) - (top_left + 2*top + top_right)
value = sqrt(Gx*Gx + Gy*Gy)
```

It is not normalized (no division by 8), and the thresholds then apply as usual, values above the maximum being clamped.
The magnitude is therefore much larger than that of `"sobel"`, so **thresholds tuned for one do not suit the other**.
Integer clips round the magnitude to the nearest integer; float clips keep the fraction. The same computation as an
expression:

```avs
c.Expr("x[1,-1]:c x[-1,-1]:c - x[1,1]:c x[-1,1]:c - + x[1,0]:c x[-1,0]:c - 2 * + dup * x[-1,1]:c x[-1,-1]:c - x[1,1]:c x[1,-1]:c - + x[0,1]:c x[0,-1]:c - 2 * + dup * + sqrt")
```

### mt_motion

```
mt_motion(clip, float "thY1", float "thY2", float "thC1", float "thC2", float "thT", float "sc_value", ...)
```

Creates a motion mask: the absolute difference between the frame and the previous frame (frame 0 is compared with
frame 1), thresholded as in mt_edge.

| Parameter | Default | Meaning |
|---|---|---|
| thY1, thC1 | 10 | Low threshold: this and below become 0 |
| thY2, thC2 | 10 | High threshold: above this becomes the maximum |
| thT | 10 | Scene change limit: the average absolute difference per pixel. Above it the frame is a scene change. |
| sc_value | 0 | The value filling the mask on a scene change |

The luma plane decides for all planes of the frame, also when luma itself is not processed.

## 7. Mask operations

### mt_invert

```
mt_invert(clip, ...)
```

Computes `max - x` on every processed plane: `255 - x` at 8 bit, `2^bits - 1 - x` at 10-16 bit, `1.0 - x` in float.

This is a mask operation. On a picture it does not treat chroma as a signed value around its neutral point, so inverted
chroma is not the opposite colour; use AviSynth's `Invert` for pictures. It does not handle limited range either:
16-235 is not mapped onto itself.

### mt_binarize

```
mt_binarize(clip, float "threshold", bool "upper", string "mode", ..., bool "stacked")
```

Turns a soft mask into a hard one. Equivalent to the expression `x t > a b ?`, where `t` is the threshold and `a b`
comes from the mode: pixels above `t` become `a`, the others `b`.

| Parameter | Default | Meaning |
|---|---|---|
| threshold | 128 | Scaled by [paramscale](#paramscale). The same value on all planes, not centred on float chroma. |
| upper | false | Kept for compatibility: `true` is `mode="upper"`. |
| mode | `"lower"` | Values below |

| mode (`a b`) | Pixel above t | Other pixels |
|---|---|---|
| `"255 0"`, `"lower"` | max | 0 |
| `"0 255"`, `"upper"` | 0 | max |
| `"0 x"` | 0 | x |
| `"x 0"` | x | 0 |
| `"0 t"` | 0 | t |
| `"t 0"` | t | 0 |
| `"t x"` | t | x |
| `"x t"` | x | t |
| `"255 x"` | max | x |
| `"x 255"` | x | max |
| `"255 t"` | max | t |
| `"t 255"` | t | max |

`255` here means the maximum at any bit depth.

### mt_logic

```
mt_logic(clip, clip, string "mode", float "th1", float "th2", ..., bool "stacked")
```

Combines two masks. `mode` is required, with no default. `x` is the first clip, `y` the second.

| mode | Result | Example, x = 11, y = 5 (or the other way, see andn) |
|---|---|---|
| `"and"` | `x & y`, bitwise | 1011 & 0101 = 1 |
| `"or"` | `x \| y`, bitwise | 1011 \| 0101 = 15 |
| `"xor"` | `x ^ y`, bitwise | 1011 ^ 0101 = 14 |
| `"andn"` | `~x & y`, bitwise | x = 5, y = 11: ~0101 & 1011 = 10 |
| `"min"` | `min(x + th1, y + th2)` | |
| `"max"` | `max(x + th1, y + th2)` | |

`th1`, `th2` default to 0, may be negative, are scaled by [paramscale](#paramscale), and only affect `"min"` and `"max"`.

On float clips the bitwise modes work on the raw bit pattern of the 32 bit value. That is only meaningful for hard masks holding 0 and 1.0; `"min"` and `"max"` are the
usual choice for soft float masks.

**`"andn"` negates the first clip.** The name comes from the x86 instruction (`PANDN`), and MaskTools has always computed
`~x & y`. Elsewhere "a and not b" usually means `a & ~b`; for that, swap the clips: `mt_logic(y, x, "andn")`. The
original manual described the other order, wrongly. It is not the same as the expression operator `&!`, which is logical
and means "x and not y".

### mt_hysteresis

```
mt_hysteresis(clip small_mask, clip big_mask, bool "corners", ...)
```

Grows the small mask into the big mask: every connected area of nonzero pixels of `big_mask` that contains a nonzero
pixel of `small_mask` as well (at the same position) becomes the maximum, everything else 0. It builds more robust edge
masks: a strict mask gives the seeds, a loose one the extent.

| Parameter | Default | Meaning |
|---|---|---|
| corners | true | true: 8 neighbours are connected. false: only 4, no corner-only connections. |

```
corners=true   corners=false
  + + +          - + -
  + O +          + O +
  + + +          - + -
```

```avs
strict = c.mt_edge("sobel", 40, 40)
loose = c.mt_edge("sobel", 10, 10)
mt_hysteresis(strict, loose)
```

## 8. Merging

### mt_merge

```
mt_merge(clip clip1, clip clip2, clip mask, bool "luma", ..., bool "stacked", string "cplace")
```

The backbone of the framework: merges two clips according to the mask. The larger the mask value, the more of `clip2`.
A mask at 0 gives exactly `clip1`, at the maximum exactly `clip2`.

| Bit depth | Formula, m is the mask value, max the maximum pixel value |
|---|---|
| Integer | `(clip1 * (max - m) + clip2 * m + max / 2) / max`, an exact integer division; `max / 2` truncates |
| Float | `(1 - m) * clip1 + m * clip2`, m in 0..1 |

This is the same blend as in AviSynth+ `Merge`, `Layer` and `Overlay`.

| Parameter | Default | Meaning |
|---|---|---|
| luma | false | Use the mask's luma for every plane, see below. Forces `u=3, v=3`. |
| cplace | `"auto"` | Chroma placement for the luma mask: `"auto"`, `"mpeg1"`, `"mpeg2"`, `"top_left"` (or `"topleft"`), see [cplace](#cplace-where-the-chroma-samples-sit) |

All three clips must have the same bit depth. Y and YA clips have no chroma: `U` and `V` are ignored there.

With `luma=false`, each plane uses the same plane of the mask, and a greyscale mask is an error when chroma or alpha is
processed. Since mt_merge works in place, the default `u=1, v=1` keeps `clip1`'s chroma.

With `luma=true`, `U` and `V` are always processed, and the mask only needs a full size luma plane. Alpha is not affected:
with `A=3` it needs its own alpha plane in the mask, not the luma one.

#### cplace: where the chroma samples sit

`cplace="auto"`, the default (an empty string means the same), chooses in this order:

1. The `_ChromaLocation` frame property of `clip1`, read from frame 0.
2. The default of the format, as in AviSynth+'s ConvertToXXXX: `left` for 4:2:0, 4:2:2 and 4:1:1, `top` for 4:4:0, `top_left` for 4:1:0.

A name given in `cplace` overrides both.

- Only `clip1` is read. `clip2` and `mask` are assumed to be sited the same way: their properties are not used, and a difference is not an error.
- A missing or invalid property (not an integer from 0 to 6) falls back to the format default, and so does a host without frame properties, like classic AviSynth 2.6.
- Frame 0 decides for the whole clip. With `"auto"`, frame 0 of `clip1` is requested when the filter is created, as in Overlay.
- To set it in a script: `clip1.propSet("_ChromaLocation", 1)` (1 is center).

Each location selects the nearest kernel, the same way as Overlay and Layer in AviSynth+:

| `_ChromaLocation` | 4:2:0, 4:2:2, 4:1:1, 4:1:0 | 4:4:0 |
|---|---|---|
| 0 `left`, 4 `bottom_left`, 6 `DV` | `mpeg2` | `mpeg1` |
| 1 `center`, 5 `bottom` | `mpeg1` | `mpeg1` |
| 3 `top` | `mpeg1` | `top_left` |
| 2 `top_left` | `top_left` | `top_left` |

#### Luma masks on subsampled chroma: boxes, tents and triangles

A chroma plane smaller than luma needs a smaller mask. mt_merge computes it from the luma mask, according to where a chroma
sample sits among the luma samples, per direction:

- **Centred between luma samples:** at 2:1 a **box**, the plain average of the 2 pixels it covers (weights 1-1). At 4:1 a
  **triangle** centred between the middle two pixels of the block, reaching 2 pixels into both neighbouring blocks (weights 1-3-5-7-7-5-3-1).
- **Co-sited with a luma sample:** a **tent** centred on it (weights 1-2-1, or 1-2-3-4-3-2-1), because that
  luma sample should count most and its neighbours on both sides less and less.

| Placement | Horizontal position | Vertical position |
|---|---|---|
| `"mpeg1"` | centred | centred |
| `"mpeg2"` | co-sited (left) | centred |
| `"top_left"` | co-sited (left) | co-sited (top) |

Each format uses only the directions it subsamples, so two names can mean the same kernel:

| Format | mpeg1 | mpeg2 | top_left | Default without a property |
|---|---|---|---|---|
| 4:2:0 | Box 2x2 | Tent 1-2-1 across, box over 2 rows | Tent 1-2-1 both ways | mpeg2 |
| 4:2:2 | Box 1-1 | Tent 1-2-1 | Same as mpeg2 | mpeg2 |
| 4:4:0 | Box 1-1 vertically | Same as mpeg1 | Tent 1-2-1 vertically | top_left |
| 4:1:1 | Triangle, 8 columns | Tent 1-2-3-4-3-2-1 | Same as mpeg2 | mpeg2 |
| 4:1:0 | Triangle both ways, 8x8 | Tent across, triangle over 8 rows | Tent both ways, 7x7 | top_left |
| 4:4:4, RGB | The luma mask itself | | | |

Weights in the 4:2:0 case, `*` marks the position of the chroma sample:

```
mpeg1              mpeg2                     top_left
* sits in the      * sits on the middle      * sits on the center
  center             column, between rows

  1/4 1/4          1/8 2/8 1/8               1/16 2/16 1/16
  1/4 1/4          1/8 2/8 1/8               2/16 4/16 2/16
                                             1/16 2/16 1/16
```

Tents and triangles need pixels outside the block. At the edge of the plane or box, those are replaced by the
nearest pixel.

Integer masks are rounded to nearest once, at the end, except where the table below says otherwise. Float masks use the same
weights without rounding.

| Format, placement | Integer mask from luma mask pixels `a b` / `c d` (rows) |
|---|---|
| 4:2:0 mpeg1 | `(((a + c + 1) / 2) + ((b + d + 1) / 2) + 1) / 2`: pairs rounded first |
| 4:2:2 mpeg1 | `(a + b + 1) / 2` |
| 4:4:0 mpeg1, mpeg2 | `(a + c + 1) / 2` |
| 4:2:0 mpeg2 | Sum of weights 8, rounded once |
| 4:2:0 top_left | Sum of weights 16, rounded once |
| 4:2:2 mpeg2 | Sum of weights 4, rounded once |
| 4:4:0 top_left | Sum of weights 4, rounded once |
| 4:1:1 mpeg1, mpeg2 and top_left | Sums of weights 32, 16, rounded once |
| 4:1:0 mpeg1, mpeg2, top_left | Sums of weights 1024, 512, 256, rounded once |

## 9. Morphological filters

### mt_expand, mt_inpand

```
mt_expand(clip, float "thY", float "thC", string "mode", ..., bool "stacked")
mt_inpand(clip, float "thY", float "thC", string "mode", ..., bool "stacked")
```

mt_expand replaces each pixel with the local maximum, mt_inpand with the local minimum.

| Parameter | Default | Meaning |
|---|---|---|
| thY, thC | 255 | The maximum change of a pixel. Scaled by [paramscale](#paramscale); 255 means no limit at any bit depth. Alpha uses thY. |
| mode | `"square"` | The neighbourhood |

| mode | Neighbourhood |
|---|---|
| `"square"` | 3x3 |
| `"horizontal"` | 3x1 |
| `"vertical"` | 1x3 |
| `"both"` | A cross: `"horizontal"` + `"vertical"` |
| A list of coordinates | Custom. `"0 0 -1 0 1 0"` equals `"horizontal"`. See [shape strings](#shape-strings). An odd number of values is an error; an empty list leaves the plane unchanged. |

```avs
c.mt_expand(mode=mt_circle(2))
```

### mt_inflate, mt_deflate

```
mt_inflate(clip, float "thY", float "thC", ..., bool "stacked")
mt_deflate(clip, float "thY", float "thC", ..., bool "stacked")
```

mt_inflate computes the average of the 8 neighbours (not the center) and keeps it only if higher than the pixel: `max(x,
average)`. mt_deflate keeps it only if lower: `min(x, average)`. The integer average truncates.

`thY`, `thC` limit the change as above.

## 10. Lookup and expression filters

### Common to the family

| Parameter | Default | Meaning |
|---|---|---|
| expr | | The [expression](#11-expression-language) for every plane |
| yExpr, uExpr, vExpr, aExpr | | The expression of one plane (R, G, B on RGB), over `expr` |
| realtime | see below | Evaluate per pixel instead of through a lookup table |
| scale_inputs | `"none"` | Scales inputs to a working bit depth, see [scale_inputs](#scale_inputs) |
| clamp_float | false | Clamps float results, see [clamp_float](#clamp_float-clamp_float_uv) |
| clamp_float_UV | false | Clamp float chroma as 0..1 |
| use_expr | 0 | Passes the work to AviSynth+ Expr: [use_expr](#use_expr). mt_lut, mt_lutxy, mt_lutxyz, mt_lutxyza only. |
| stacked | false | mt_lut only, see [stacked](#16-stacked-16-bit) |

As with all filters, only `y` is processed by default: `u=3, v=3` is needed for chroma.

**A processed plane without any expression** is left as if not processed: it keeps the first clip's plane. In mt_lutxyz
and mt_lutxyza it gets the **third** clip's plane instead, and mt_lutspa fills it with 0. Always give an expression.

An invalid expression (an unknown word) is an error when the filter is created, naming the position. So is an operator lacking
operands when a lookup table is built. In realtime mode a lacking operand is not detected and the result is 0.

#### Realtime and lookup tables

For integer clips an expression can be evaluated in advance for every combination of input values, and then looked up. That is
very fast once built, but the table can be huge. Otherwise it is evaluated per pixel, which is slow; see
[use_expr](#use_expr) for the fast way.

| Inputs | 8 bit | 10 bit | 12 bit | 14 bit | 16 bit |
|---|---|---|---|---|---|
| One (mt_lut) | 256 bytes | 2 KB | 8 KB | 32 KB | 128 KB |
| Two (mt_lutxy, mt_luts, mt_lutf) | 64 KB | 2 MB | 32 MB | 512 MB | 8 GB |
| Three (mt_lutxyz, mt_lutsx) | 16 MB | Not available | | | |
| Four (mt_lutxyza) | 4 GB | Not available | | | |

- Float clips are always realtime.
- The "realtime default" is in the [table in chapter 3](#which-parameters-each-filter-has). You can set `realtime` either way
  where a table exists, e.g. `realtime=true` to save memory, or `realtime=false` for a 16 bit mt_lutxy in 64 bit with 8 GB
  to spare. mt_lutxyza's 4 GB table with `realtime=false` is 64 bit only. A 16 bit table of two inputs needs 64 bit as well.
- Too large a table for the available memory is an error, naming `realtime=true`.
- Expressions using `width`, `height`, `sx`, `sy`, `sxr`, `syr`, `frameno` or `time` cannot use a table, and always run in
  realtime, whatever `realtime` says.
- Tables are shared between planes with the same expression.

#### use_expr

Passes the work to AviSynth+'s `Expr` filter, whose JIT compiler is much faster than the evaluator here for realtime
work, and mostly independent of the bit depth.

| Value | Meaning |
|---|---|
| 0 | Never (default) |
| 1 | When the clip is above 8 bit; always in mt_lutxyza |
| 2 | When MaskTools would use realtime |
| 3 | Always |

Expressions, `scale_inputs`, `clamp_float` and `clamp_float_UV` are passed on. Plane modes become expressions: fill values a
constant, copies `x`, `y`, `z` or `a`.

- **Expr computes in 32 bit float inside**, MaskTools in 64 bit double. Results can differ slightly, and large values lose precision: a 32 bit float holds integers exactly only up to 2^24, which matters for large intermediate values and for `frameno` on very long clips.
- **Frame properties come from Expr:** from the first clip the expression uses, not always from the first clip as in MaskTools,
  and a constant expression gives a frame without properties. For example `mt_lutxy(a, b, "y 2 *", use_expr=3)` takes them from `b`.
- **Only the full frame can be passed.** With `offX`, `offY`, `w` or `h` selecting less than the frame, it is an error whenever the
  call would go to Expr.
- `use_expr` is ignored when the host has no `Expr`, and for stacked clips.
- Expr parses the expression, so anything it does not know is an error from Expr; and results follow Expr's
  rules. See the [differences](#12-comparison-with-avisynth-expr).

```avs
c.ConvertBits(16).mt_lutxyz(c.ConvertBits(16), c.ConvertBits(16), expr="x y + z + 3 /", use_expr=2)
```

#### Neighbourhood modes

mt_lutf, mt_luts and mt_lutsx summarize many values with a `mode`:

| mode | Result |
|---|---|
| `"avg"`, `"average"` | Average (the default) |
| `"std"`, `"standard deviation"` | Standard deviation |
| `"min"`, `"minimum"` | Minimum |
| `"max"`, `"maximum"` | Maximum |
| `"range"` | Maximum minus minimum |
| `"med"`, `"median"` | Median |
| A list of numbers, for mt_luts and mt_lutsx | A weighted sum of the values in `pixels` order, divided by the sum of the weights |

### mt_lut

```
mt_lut(clip, string "expr", string "yExpr", string "uExpr", string "vExpr", ..., string "aExpr", bool "stacked",
       bool "realtime", string "scale_inputs", bool "clamp_float", int "use_expr", bool "clamp_float_UV")
```

Applies an expression of the pixel value `x`.

```avs
c.mt_lut("x 128 > 255 0 ?")            # a threshold
c.mt_lut(yExpr="x ymin - ymax ymin - / range_max *", u=2, v=2)  # limited to full range luma, at any bit depth
```

### mt_lutxy, mt_lutxyz, mt_lutxyza

```
mt_lutxy(clip x, clip y, string "expr", ...)
mt_lutxyz(clip x, clip y, clip z, string "expr", ...)
mt_lutxyza(clip x, clip y, clip z, clip a, string "expr", ...)
```

Apply an expression of the pixel values of two, three or four clips. Only mt_lutxy and mt_lutxyz use a table by default, see
above.

```avs
d = c.mt_invert()
mt_lutxy(c, d, "x y - abs")
```

### mt_lutf

```
mt_lutf(clip x, clip y, string "mode", string "expr", ...)
```

Collects one value from all pixels of the first clip's plane, according to `mode` ([modes](#neighbourhood-modes), without the
weighted list). Then applies the expression to each pixel of the second clip, with `x` the collected value and `y` the pixel.

```avs
mt_lutf(c, c, mode="range", expr="y 128 - 256 * x / 128 +")   # stretches contrast adaptively
```

### mt_luts

```
mt_luts(clip x, clip y, string "mode", string "pixels", string "expr", ...,
        string "wexpr", string "ywExpr", string "uwExpr", string "vwExpr", string "awExpr")
```

For each pixel, evaluates the expression once per position in `pixels`, with `x` the pixel of the first clip and `y` the pixel of
the second clip at that position. Then summarizes the results with `mode`.

`pixels` is a list of `x y` coordinate pairs relative to the current pixel, usually from a [shape helper](#shape-strings). It is
required: an empty list or an odd number of values is an error. Positions outside the plane are replaced by the nearest pixel.

```avs
c.mt_luts(c, mode="avg", pixels=mt_square(1), expr="y")            # 3x3 box blur
c.mt_luts(c, mode="min", pixels=mt_square(1), expr="y")            # same as mt_inpand()
c.mt_luts(c, mode="range", pixels=mt_square(1), expr="y")          # same as mt_edge("min/max", 0, 255)
c.mt_luts(c, mode="std", pixels=mt_square(1), expr="y")            # local standard deviation
c.mt_luts(c, mode="max", pixels=mt_square(1), expr="x y - abs")    # largest difference from the center
c.mt_luts(c, mode="med", pixels=mt_square(1), expr="y")            # median
c.mt_luts(c, mode="1 1 1 1 1 1 1 1 1", pixels=mt_square(1), expr="y")  # same as mode="avg"
```

**Weights:** `wexpr`, or `ywExpr`, `uwExpr`, `vwExpr`, `awExpr` for a plane, is a second expression of the same `x` and `y`. It gives
a float weight for each value. Each value is multiplied by its weight before it is collected. For `"avg"` and `"std"` the weights
are also summed up, so the average is `sum(value * weight) / sum(weight)`; other modes do not normalize. A weight of 1 everywhere
gives the same result as no weight. Weights are not clamped.

```avs
c.mt_luts(c, mode="avg", pixels=mt_square(2), expr="y", wexpr="1 x y - abs 16 / -")   # blur, similar pixels count more
```

### mt_lutsx

```
mt_lutsx(clip, clip y, clip z, string "mode", string "mode2", string "pixels", string "expr", ...)
```

For each pixel, summarizes the neighbourhood `pixels` in the second clip with `mode`, and in the third clip with `mode2`. Then
evaluates the expression with `x` the pixel of the first clip, `y` and `z` the two summaries. `pixels` is required, as in mt_luts.
**Set `mode2`:** it defaults to `"none"`, which gives `z` = 0.

```avs
mt_lutsx(c, c, c, mode="min", mode2="max", pixels=mt_square(1), expr="z y -")                 # local max - min
mt_lutsx(c, c, c, mode="min", mode2="max", pixels=mt_square(1), expr="x y - 256 * z y - /")   # local contrast stretch
```

### mt_lutspa

```
mt_lutspa(clip, string "mode", bool "relative", bool "biased", string "expr", ..., string "aExpr",
          string "scale_inputs", bool "clamp_float", bool "clamp_float_UV")
```

Computes each pixel from its coordinates, ignoring the clip's pixels. In the expression `x` and `y` are the column and the row,
transformed according to `mode`, relative to the [box](#offx-offy-w-h) if there is one.

| mode | x, y range | Same as |
|---|---|---|
| `"relative"` (default), `"relative opened"`, `"relative exclusive"` | 0 to 1, excluded | `relative=true, biased=true` |
| `"relative closed"`, `"relative inclusive"` | 0 to 1, included | `relative=true, biased=false` |
| `"absolute"` | 0 to width - 1, 0 to height - 1 | `relative=false` |

`relative` and `biased` are the legacy form; `mode` overrides them. Both default to true.

There is no automatic scaling in integer modes, except what `scale_inputs` does. The result is a table computed once, unless
the expression uses `frameno` or `time`: then it is computed for each frame.

```avs
c.mt_lutspa(mode="relative", expr="x y + 256 * 2 /", chroma="-128")   # a diagonal gradient
```

The context variables `sx`, `sy`, `sxr`, `syr`, `width`, `height`, `frameno`, `time` also work in every other filter of the family.

## 11. Expression language

Expressions are written in reverse polish notation (RPN), the same family as AviSynth+ `Expr`. An operator comes after its operands:
`x + y` is `x y +`, and `(3 + 5) * x` is `3 5 + x *`. Reading left to right, values go on a stack and each operator takes its operands from
the top of it, so no parentheses are needed. [mt_polish](#mt_polish-mt_infix) converts ordinary infix notation to this.

Tokens are separated by spaces, tabs or line breaks. Numbers are read independently of the locale, with `.` as the decimal point.

All computation is in double precision (64 bit) floats. For integer clips the final result is clamped to the valid range and
rounded to the nearest integer (half up). For float clips it is not rounded, and not clamped unless asked with
[clamp_float](#clamp_float-clamp_float_uv).

### Variables

| Token | Value |
|---|---|
| `x`, `y`, `z`, `a` | The input pixels, as far as the filter has them. See each filter; `x` and `y` of mt_lutspa are coordinates. |
| `width`, `height` | Size of the current plane, or of the box in it |
| `sx`, `sy` | Column and row of the current pixel in the plane, or box. Starts at 0. |
| `sxr`, `syr` | The same relative: `sx / (width - 1)`, 0 to 1 inclusive; 0 on a single column or row |
| `frameno` | The current frame number |
| `time` | `frameno / (frame count - 1)`, 0 to 1 inclusive; 0 on a single frame clip |

`width`, `height`, `sx`, `sy`, `sxr` and `syr` are **per plane**: on 4:2:0 chroma `width` is half the luma width. They are also
**relative to the box** (`offX`, `offY`, `w`, `h`): the result in a box equals the same filter on the cropped clip. Without a box they
are the same as in Expr. On a stacked clip they describe the native 16 bit plane, half the clip's height.

### Bit depth aware constants

| Token | 8 bit | 10 bit | 16 bit | Float luma, RGB, alpha | Float chroma |
|---|---|---|---|---|---|
| `bitdepth` | 8 | 10 | 16 | 32 | 32 |
| `sbitdepth` | The base for scaling, see below; 8 by default | | | | |
| `range_min` | 0 | 0 | 0 | 0 | -0.5 |
| `range_half` | 128 | 512 | 32768 | 0.5 | 0.0 |
| `range_max` | 255 | 1023 | 65535 | 1.0 | 0.5 |
| `range_size` | 256 | 1024 | 65536 | 1.0 | 1.0 |
| `yrange_min`, `yrange_half`, `yrange_max` | As above | | | The luma values on every plane: 0, 0.5, 1.0 | |
| `ymin`, `ymax` | 16, 235 | 64, 940 | 4096, 60160 | 16/255, 235/255 | 16/255, 235/255 |
| `cmin`, `cmax` | 16, 240 | 64, 960 | 4096, 61440 | (16-128)/255, (240-128)/255 | (16-128)/255, (240-128)/255 |
| `pi` | 3.1415927 | | | | |

### Converting levels: scaleb, scalef

`scaleb` and `scalef` convert a **level**, a position on the pixel value scale, from the base bit depth (8 by default) to the
clip's depth. `128 scaleb` is neutral chroma and `235 scaleb` is limited range white at any depth. They are the same calculations
as `ConvertBits` and `scaleb`, `scalef` in AviSynth+ Expr: `(x - source offset) * factor + target offset`, in 32 bit float.

| Token | Rules | Use for | Example at 10 bit |
|---|---|---|---|
| `scaleb` | Limited range | Limited range YUV | `235 scaleb` = 940 |
| `scalef` | Full range | RGB, full range YUV | `255 scalef` = 1023 |
| `yscaleb`, `yscalef` | The same, always with the rules for luma, also on chroma | | |

`@B` and `@F` are deprecated names of `scaleb` and `scalef`.

The base bit depth is set by one of `i8`, `i10`, `i12`, `i14`, `i16`, `f32` anywhere in the expression. If several are given, the last
counts. It is also the working depth of [scale_inputs](#scale_inputs).

The name "bit shift" of `scaleb` only describes the result between integer depths, where the offsets cancel out:

| Plane | Clip | Calculation for an 8 bit operand | Reduces to |
|---|---|---|---|
| Luma | 10-16 bit | `(x - 16) * 2^(bits-8) + (16 << (bits-8))` | `x << (bits-8)` |
| Luma | 32 bit float | `(x - 16) / 255 + 16/255` | `x / 255` |
| Chroma | 10-16 bit | `(x - 128) * 2^(bits-8) + (128 << (bits-8))` | `x << (bits-8)` |
| Chroma | 32 bit float | `(x - 128) / 255` | `(x - 128) / 255` |

`scalef` on chroma scales around the center as well, with a span of `(2^bits - 1) / 2` on each side:

| Expression | Plane | 8 bit | 10 bit | 16 bit | Float |
|---|---|---|---|---|---|
| `235 scaleb` | Y | 235 | 940 | 60160 | 0.92157 |
| `235 scaleb` | U, V | 235 | 940 | 60160 | 0.41961 |
| `128 scaleb` | U, V | 128 | 512 | 32768 | 0.0 |
| `i10 940 scaleb` | Y | 235 | 940 | 60160 | 0.92157 |
| `f32 0.92157 scaleb` | Y | 235 | 940 | 60160 | 0.92157 |
| `235 scalef` | Y | 235 | 943 | 60395 | 0.92157 |
| `128 scalef` | Y | 128 | 514 | 32896 | 0.50196 |
| `128 scalef` | U, V | 128 | 512 | 32768 | 0.0 |

**They are for levels, not for differences.** Do not use them to give a difference, an amount to add to or subtract from a pixel, such
as an overshoot limit. The conversion subtracts neutral chroma from the operand, so `5 scaleb` on float U or V is -0.48235, not 5 steps of
0.01961, and `5 scalef` on 16 bit U is 1157, not 1285. With `scale_inputs="floatUV"` the float result looks almost right, but it is
about 10% low. On luma and at integer depths with `scaleb` a difference comes out right only because the offsets cancel there.

For a difference in 8 bit limited range steps, scale by the size of the range. This gives the correct value at every depth, with or
without `"floatUV"`:

- Chroma: `x 5 224 / cmax cmin - * +`, where 224 = 240 - 16.
- Luma: `x 5 219 / ymax ymin - * +`, where 219 = 235 - 16.

| Source U (neutral) | Expression | `scale_inputs` | Result | Correct value |
|---|---|---|---|---|
| 512 (10 bit) | `x 5 scaleb +` | | 532 | 532 |
| 0.0 (float) | `x 5 scaleb +` | | **-0.48235** | 0.01961 |
| 0.0 (float) | `x 5 scaleb +` | `"floatUV"` | **0.01765** | 0.01961 |
| 512 (10 bit) | `x 5 224 / cmax cmin - * +` | | 532 | 532 |
| 0.0 (float) | `x 5 224 / cmax cmin - * +` | | 0.01961 | 0.01961 |
| 0.0 (float) | `x 5 224 / cmax cmin - * +` | `"floatUV"` | 0.01961 | 0.01961 |

With `scale_inputs="all"` the expression works on 8 bit values: there `5` itself is the difference, and the operators have nothing to scale.

### No range is assumed

The filters do not read the `_ColorRange` frame property, and the constants do not depend on the range of the clip. `ymin`, `ymax`, `cmin`,
`cmax` and `scaleb` are always limited range values, also on a full range clip. The script chooses by name: `ymin`, `ymax` and `scaleb` for
limited range, `range_min`, `range_max` and `scalef` for full range.

| On the Y plane, `_ColorRange` 0 or 1 | Always means | 10 bit | Float |
|---|---|---|---|
| `ymin`, `ymax` | Limited range luma limits, 16 and 235 | 64, 940 | 0.0627, 0.9216 |
| `cmin`, `cmax` | Limited range chroma limits, 16 and 240 | 64, 960 | -0.4392, 0.4392 |
| `255 scaleb` | A level, by limited range rules | 1020 | 1.0 |
| `255 scalef` | A level, by full range rules | 1023 | 1.0 |
| `range_min`, `range_max` | The whole range of the plane | 0, 1023 | 0.0, 1.0 |
| `range_half` | The middle of the range | 512 | 0.5 |

To follow the clip, read the property in the script and choose the expression there. Expressions here have no frame property syntax, but
with `use_expr=3` the string goes to AviSynth+ Expr, where this gives 940 on a limited range 10 bit clip and 1023 on a full range one:

```avs
c.mt_lut("x._ColorRange 1 == ymax range_max ?", y=3, use_expr=3)
```

A single expression serves every processed plane, but its result can differ per plane: `scaleb`, `scalef` and, on float clips, `range_half`,
`range_min`, `range_max` follow the plane being luma or chroma, and `width`, `height`, `sx`, `sy` the size of the plane.

### Three ways to write a level conversion

A multiplier is not a level either: `x y - 256 scaleb *` is `x y - 1024 *` at 10 bit, but on float chroma `256 scaleb` is 0.50196.
Use `range_size` there.


```avs
c.ConvertBits(16).mt_lut("x 4096 - 56064 / 65535 *")                       # only for 16 bit
c.ConvertBits(16).mt_lut("x 16 scaleb - 219 scaleb / 255 scalef *")        # any bit depth, with scale operators
c.ConvertBits(16).mt_lut("x ymin - ymax ymin - / range_max *")             # any bit depth, with constants
```

### Operators and functions

**Arithmetic**

| Token | Operands | Result |
|---|---|---|
| `+` `-` `*` `/` | 2 | |
| `^`, `pow` | 2 | Power, IEEE 754: a negative base with a fractional power is NaN. A constant power of 0.5 is `sqrt`: `x 0.5 ^` gives 0 for a negative `x` |
| `%` | 2 | Floating point remainder, as C `fmod`, sign of the first operand: `5.7 2.2 %` = 1.3. By 0: NaN, so 0 at integer output. |
| `abs`, `neg`, `sgn` | 1 | Absolute value; negation; -1, 0 or 1 by sign |
| `sqrt` | 1 | Square root; 0 for a negative operand or NaN |
| `exp`, `log` | 1 | Natural exponential and logarithm |
| `sin`, `cos`, `tan`, `asin`, `acos`, `atan` | 1 | In radians |
| `atan2` | 2 | `y x atan2` is `atan2(y, x)` |
| `min`, `max` | 2 | |
| `clip` | 3 | `x 16 240 clip` is `min(max(x, 16), 240)` |
| `round` | 1 | To nearest, half away from zero |
| `floor`, `ceil`, `trunc` | 1 | Down, up, towards zero |

**Comparisons and logic**

Comparisons give true or false as a number: **true is 1, false is -1**. Any value above 0 counts as true.

| Token | Result |
|---|---|
| `==`, `=` | Equal, with a tolerance of 0.000001 |
| `!=` | Not equal, with the same tolerance |
| `<` `<=` `>` `>=` | Exact |
| `&`, `and` | Both true |
| `\|`, `or` | At least one true |
| `@`, `°`, `xor` | Exactly one true (`°` depends on the code page, prefer `@` or `xor`) |
| `&!` | Logical and not: `x y &!` is true when x > 0 and y <= 0 |
| `not` | 1 operand: true for 0 and below |
| `?` | Ternary: `c a b ?` gives `a` if c > 0, else `b` |

`&!` is not the same as mt_logic's `"andn"`, which is bitwise and has the operands the other way.

**Bitwise**, on integers

| Token | Operates on | Result |
|---|---|---|
| `&u`, `\|u`, `@u` (`°u`), `~u` | Unsigned: rounded, clipped to 0..2^64-1 | And, or, xor; `~u` has 1 operand, complement |
| `<<`, `<<u`, `>>`, `>>u` | The same | Shifts |
| `&s`, `\|s`, `@s` (`°s`), `~s` | Signed: rounded, clipped to -2^63..2^63-1 | And, or, xor; `~s` has 1 operand, complement |
| `<<s`, `>>s` | The same | Shifts |

The second operand of a shift is always signed: shifting left by a negative amount shifts right.

**Stack**

| Token | Stack before → after |
|---|---|
| `dup`, `dup0` | `a` → `a a` |
| `dup1`..`dup9` | Copies the Nth value below the top: `a b dup1` → `a b a` |
| `swap`, `swap1` | `a b` → `b a` |
| `swap2`..`swap9` | Swaps the top with the Nth value below it: `a b c swap2` → `c b a` |

```
x 3 / x 3 / +    is the same as    x 3 / dup +
x 2 /            is the same as    2 x swap /
```

### scale_inputs

Makes expressions written for one bit depth work at every depth. The inputs `x`, `y`, `z`, `a` are scaled to the working depth (8 by default,
or as `i10`..`i16` sets it), the expression runs, and the result is scaled back, then clamped and rounded. No precision is lost: this is done in
double precision, not as an integer shift.

| Value | Scales | With |
|---|---|---|
| `"none"` (default) | Nothing | |
| `"int"` | Integer clips | Bit shift logic |
| `"intf"` | Integer clips | Full range stretch |
| `"float"`, `"floatf"` | Float clips | |
| `"floatUV"` | Float chroma: `x + 0.5` going in, the result `- 0.5` going out. For expressions written with 0.5 centred chroma. | |
| `"all"` | Integer and float | Bit shift logic, for limited range YUV |
| `"allf"` | Integer and float | Full range stretch, for RGB and full range YUV |

For example, from 16 bit to 8: by shift `result = f(input / 256.0) * 256.0`; full range `result = f(input / 65535.0 * 255.0) / 255.0 * 65535.0`.
Float luma 0..1.0 and chroma -0.5..0.5 scale to 0..255 at the working depth of 8.

- The constants (`range_max`...) follow the working depth. With `"floatUV"`, the constants on chroma are shifted too: `range_half` is 0.5.
- `scaleb`, `scalef` and the like then have nothing to do for the scaled formats.
- One method applies to all inputs and to the result, it cannot differ per clip.
- The context variables (`sx`, `frameno`...) are not scaled.

This replaces the older `clamp_f_i8`..`clamp_f_i16`, `clamp_f_f32`, `clamp_f` keywords in the expression, which still work.

### clamp_float, clamp_float_UV

Float results are not clamped by default. With `clamp_float=true`, luma, RGB and alpha are clamped to 0..1, chroma to -0.5..0.5. With also
`clamp_float_UV=true`, chroma is clamped to 0..1 instead; use it with expressions written for positive-only U and V values.

### Expression tips

- Boolean results in arithmetic: since false is -1, `x 128 > 255 *` gives -255 for false. On integer clips this clamps to 0, on float
  it stays negative. Use `x 128 > 255 0 ?`.
- A value left on the stack is not an error: `x 1` just gives 1.
- There are no comments in expressions, and no user variables.

## 12. Comparison with AviSynth+ Expr

`Expr` is AviSynth+'s built-in expression filter, derived from the same idea. Its language is largely a superset of the arithmetic here, and on x86
a JIT compiler makes it far faster than the realtime evaluator. Its documentation has the mirror of this chapter.

| | MaskTools | Expr |
|---|---|---|
| Evaluation | Lookup table or interpreter | JIT on x86 (SSE2, AVX2), else C; lookup table on request |
| Precision | Double (64 bit) | Float (32 bit) |
| Input clips | 2 to 4, or 1 | Up to 26, bit depths can differ |
| Boolean false | **-1** | **0** |
| `=`, `!=` | Tolerance 0.000001 | Exact |
| `round` | Half away from zero | Nearest even |
| `sqrt` of a negative or NaN | 0 | 0, and -0 gives +0 |
| `^` with a negative base | IEEE 754, as `pow`: `-2 3 ^` is -8, `-2 2.5 ^` is NaN | The same in recent versions; older JIT versions gave about 2.4e38 |
| Bitwise operators, shifts | `&u`, `<<`, `&s`... | None |
| `&!`, `@`, `°` | Yes | No (`xor` and the others yes) |
| Same names | `and`, `or`, `xor`, `not`, `pow`, `neg`, `sgn`, `sqrt`, `sx`, `sy`, `sxr`, `syr`, `width`, `height`, `frameno`, `time`, `scaleb`, `scalef`, `yscaleb`, `yscalef`, `dup`, `swap`, `i8`..`f32`, `range_*`, `ymin`... | The same |
| Constant of another clip | No | `range_max_y`, `ymin_z`... |
| User variables | No | `A@`, `A^`, `A` |
| Frame properties | No | `x._ColorRange` |
| Relative pixels | No (see mt_luts, mt_lutsx) | `x[-1,0]`, with edge modes |
| Part of a frame | `offX`, `offY`, `w`, `h` | Full frame only |
| Plane fill | `y=-128` | A constant expression |
| Stacked 16 bit | Yes for mt_lut | No |
| `scale_inputs`, `clamp_float`, `clamp_float_UV` | The same | The same |

Comparison operators, `%`, `?`, rounding of the final integer result and the variables give the same results otherwise.

Consequently an expression can differ when passed with `use_expr`. Expressions whose boolean result is used as a number differ
on float output, and very rarely a result differs by the last bit or, on integer output, by a rounding step, because of the float
precision. That is expected.

For filters that are not expressions in MaskTools:

| MaskTools | Expr |
|---|---|
| `mt_makediff(a, b)` | `Expr(a, b, "x y - range_half +")` |
| `mt_adddiff(a, b)` | `Expr(a, b, "x y + range_half -")` |
| `mt_invert(a)` | `Expr(a, "range_max x -")` (not on float chroma: it is 1.0 - x here) |
| `mt_lutspa(a, mode="absolute", expr="x y +")` | `Expr(a, "sx sy +")` |
| `mt_expand(a, mode="horizontal")` | `Expr(a, "x x[-1,0]:c max x[1,0]:c max")` |
| `mt_edge(a, "sobel", 0, 255)` | `Expr(a, "x[1,0]:c x[0,1]:c + x[-1,0]:c - x[0,-1]:c - abs 2 / floor")` |

## 13. Difference filters

### mt_makediff, mt_adddiff

```
mt_makediff(clip x, clip y, ..., bool "stacked")
mt_adddiff(clip x, clip y, ..., bool "stacked")
```

mt_makediff computes `x - y + half`, and mt_adddiff computes `x + y - half`, so `mt_adddiff(a, mt_makediff(a, b))` gives `b` back as long as nothing was clipped.
`half` is 128 at 8 bit, `2^(bits-1)` above, 0.5 for float luma and RGB, and 0.0 for float chroma. Integer results are clipped to the valid range. Float results are not clipped.

### mt_average

```
mt_average(clip x, clip y, ..., bool "stacked")
```

The rounded average `(x + y + 1) / 2`; in float `(x + y) / 2`.

### mt_clamp

```
mt_clamp(clip c, clip bright_limit, clip dark_limit, float "overshoot", float "undershoot", ..., bool "stacked")
```

Limits `c` between `dark_limit - undershoot` and `bright_limit + overshoot`: first `min(c, bright_limit + overshoot)`, then `max(result, dark_limit - undershoot)`. If the limits cross, the dark one wins.
`overshoot` and `undershoot` default to 0 and are scaled by [paramscale](#paramscale).

## 14. Convolutions

### mt_convolution

```
mt_convolution(clip, string "horizontal", string "vertical", bool "saturate", float "total", ...)
```

Applies the separable convolution defined by `horizontal` and `vertical`, by default `"1 1 1"`, so a 3x3 box blur.

| Parameter | Default | Meaning |
|---|---|---|
| horizontal, vertical | `"1 1 1"` | The coefficients. Use an odd length, so the kernel has a center. |
| saturate | true | true: the result is clipped to the valid range. false: its absolute value is. |
| total | | Overrides the normalization, by default the sum of the absolute values of the coefficients |

Computation is in float when any coefficient contains a `.`, or the clip is float. Plane edges use the nearest pixels. The plane can be smaller than the kernel.

### mt_mappedblur

```
mt_mappedblur(clip, clip map, string "kernel", string "mode", ...)
```

A 3x3 convolution limited per pixel by `map`: neighbours differing from the center by `map` or more do not count as they are.

| Parameter | Default | Meaning |
|---|---|---|
| kernel | `"1 1 1 1 1 1 1 1 1"` | 9 coefficients and optionally a 10th, the divisor. By default it is the sum of absolute values. |
| mode | `"replace"` | `"replace"`: such a neighbour is replaced by the center value. `"dump"`: it is left out, and the divisor follows. |

### mt_gradient

```
mt_gradient(clip, clip, string "distorsion", int "size_x", int "size_y", float "min", float "max", int "precision", ...)
```

For each position in the second clip, the sum of absolute differences of a 16x16 block against one fixed block of the first clip, mapped to the pixel range: `(sad - min) * max_pixel / (max - min)`.

| Parameter | Default | Meaning |
|---|---|---|
| distorsion | `"sad"` | The only choice |
| size_x, size_y | 0 | Position of the fixed block in the first clip. Clamped into the plane; scaled down for subsampled planes. |
| min, max | 0, `16*16*range` | The mapped range, not scaled by paramscale. `range` is `2^bits`, 1 in float. |
| precision | 1 | Sample every Nth pixel in the block |

## 15. Helpers

These return a string, so they have none of the [common parameters](#4-common-parameters).

### Shape strings

Build coordinate lists for `pixels` of mt_luts, mt_lutsx, and for `mode` of mt_expand and mt_inpand. `zero` tells whether the center (0, 0) is in the list, and defaults to true.

| Function | Shape |
|---|---|
| `mt_square(int "radius", bool "zero")` | Square, radius 1 is 3x3 |
| `mt_circle(int "radius", bool "zero")` | Circle |
| `mt_diamond(int "radius", bool "zero")` | Diamond |
| `mt_rectangle(int "hor_radius", int "ver_radius", bool "zero")` | Rectangle |
| `mt_ellipse(int "hor_radius", int "ver_radius", bool "zero")` | Ellipse |
| `mt_losange(int "hor_radius", int "ver_radius", bool "zero")` | Lozenge |
| `mt_freerectangle(int top_x, int top_y, int bottom_x, int bottom_y, bool "zero")` | Rectangle at any place |
| `mt_freeellipse(int top_x, int top_y, int bottom_x, int bottom_y, bool "zero")` | Ellipse at any place |
| `mt_freelosange(int top_x, int top_y, int bottom_x, int bottom_y, bool "zero")` | Lozenge at any place |

The radius defaults to 1. The order is column by column: `mt_square(1)` is `"-1 -1 -1 0 -1 1 0 -1 0 0 0 1 1 -1 1 0 1 1"`.

### mt_polish, mt_infix

```
mt_polish(string "infix")
mt_infix(string "polish")
```

mt_polish converts an ordinary expression to reverse polish notation for the filters above; mt_infix converts the other way.

```avs
c.mt_lut(mt_polish("x > 128 ? 255 : x > 64 ? 128 : 0"))   # "x 128 > 255 x 64 > 128 0 ? ?"
```

In infix, operators by decreasing precedence: `^`; `*` `/`; `%`; `+` `-`; `==` `!=` `<=` `>=` `<` `>`; `!&` (and not, gives `&!`), `|`, `&`, `@`, `°`, `&u`, `|u`, `@u`, `°u`, `>>`, `>>u`, `<<`, `<<u`, `&s`, `|s`, `@s`, `°s`, `>>s`, `<<s`. Lowest is the ternary
`a ? b : c`. Functions are written with parentheses: `sin(x)`, `atan2(y, x)`, `clip(x, 16, 235)`, `pow(x, 2)`, `sqrt(x)`, `scaleb(16)`, `~u(x)`.
All functions, variables and constants of the [expression language](#11-expression-language) work.

- The words `and`, `or`, `xor` are not infix operators; use `not(x)`, and `&`, `|`, `@`.
- **`x&sy` means `x &s y`.** Since `&s` is an operator, with no space `sx` and `sy` after `&`, `|`, `@`, `°`, `<<`, `>>` read as the signed operator on `x` or `y`. Write a space for the variables: `x & sy`. mt_infix writes the space.
- An expression not convertible to its end is an error, naming the position.
- Numbers are written in their shortest exact form, with `.` whatever the locale.
- mt_infix has no notion of `dup` and `swap`: it expands them to what they compute.

## 16. Stacked 16 bit

`stacked=true` is a compatibility mode for Dither-era scripts, from when AviSynth had no 16 bit formats. A stacked clip is an ordinary **8 bit clip of double height**, which holds 16 bit
data: in each plane, the upper half has the most significant bytes (MSB), and the lower half the least significant bytes (LSB).

```
plane of a stacked clip, height 2h
+---------------------+
| MSB, rows 0..h-1    |    pixel at (x, y) = MSB(x, y) * 256 + LSB(x, y)
+---------------------+
| LSB, rows 0..h-1    |
+---------------------+
```

Supported by mt_binarize, mt_logic, mt_inflate, mt_deflate, mt_inpand, mt_expand, mt_lut, mt_merge, mt_makediff, mt_adddiff, mt_average and mt_clamp. Other filters will not get it. The clip must be 8 bit, or it is an
error. All clips of the call are stacked.

For every frame and plane, the filter joins the two halves into a native 16 bit plane, runs the ordinary 16 bit code on it (SIMD included), and splits the result back. It needs no 16 bit format in the host, so it also
works on classic AviSynth 2.6. It is slower than a native 16 bit clip, because of the conversions.

- Thresholds and fill values follow [paramscale](#paramscale), as for a 16 bit clip.
- `offY` and `h` count native rows, not rows of the double height clip: `offY=2, h=2` is rows 2 and 3 of both halves.
- Expression tokens see the native plane: `height` is half the clip's height, `sy` counts within one half.
- `use_expr` is ignored on stacked clips, since Expr would see the 8 bit double height clip.
- mt_merge's `cplace` and 4:1:1 work on stacked clips, as do `realtime=true` in mt_lut.

On AviSynth+ use a native 16 bit clip when you can. The ConvertStacked plugin of AviSynth+ (`ConvertToStacked`, `ConvertFromStacked`) and Dither's tools convert between the two.

## 17. Changes in 2.3.0

The complete history since 2.0 is in `README.md`.

### Scripts that need a change

- **`sse2`, `sse3`, `ssse3`, `sse4`, `avx`, `avx2` are replaced by [opt](#opt).** A script naming one of them gets an error ("does not have a named argument"). Remove it, or use `opt`.
  The parameters after them (`A`, `alpha`, `paramscale`, `stacked`, `realtime`, `use_expr`...) move 5 positions forward; a call giving so many arguments by position needs a change as well.

### Behaviour that can change the output of existing scripts

- **mt_merge, integer clips:** the blend divides exactly by the maximum, as AviSynth+ `Merge`, `Layer` and `Overlay` do. It divided by 256 (2^bits), with only the end points patched.
- **`%` in expressions** is the floating point remainder, as in Expr: `5.7 2.2 %` = 1.3. It was the integer remainder of the rounded operands (0 here), and a divisor below 0.5 crashed. Integer operands give the
  same result as before, unless negative or very large.
- **mt_logic `"andn"`:** `~x & y` everywhere, as the SIMD code always computed it. C code computed `x & ~y`, seen on narrow planes, at the row ends of some widths, and when the C code was forced.
- **mt_expand, mt_inpand `"square"` with thY or thC below the maximum:** the limit applies once. In SIMD code the result could go up to twice the limit from the source pixel.
- **mt_luts `"avg"`, mt_lutf `"average"` on Linux and other LP64 builds, 8 and 16 bit:** rounds again; it truncated.
- **mt_edge at 10-16 bit:** values above the maximum give the maximum in `prewitt`, `hprewitt`, `cartoon`, and custom kernels. They gave too large a value or 0 in SIMD code.
- **mt_convolution:** even vertical kernels use the clamped bottom row at the end (was the row above it). Planes and windows smaller than the kernel work.
- **YV411 (4:1:1):** chroma was processed as 4x wide and 1/4 high (swapped ratios), with wrong output in every filter. Fixed.
- **mt_merge `cplace`:** the default is `"auto"`, which reads `_ChromaLocation` of `clip1`, then the format default. [Details](#cplace-where-the-chroma-samples-sit). A clip tagged other than `left` can get another mask than with the old fixed `"mpeg2"`.
- **mt_merge on 4:1:1:** with `luma=true`, low-pass masks as in AviSynth+ Overlay: a triangle with `"mpeg1"`, a tent with `"mpeg2"` and `"top_left"`. It was the average of the 4 luma pixels. 4:1:0 has the same filters from the start.
- **`scaleb`, `scalef` are level conversions, as in AviSynth+ Expr.** `scalef` on chroma scales around the center: `128 scalef` at 10 bit is 512 on chroma (was 514), `x 5 scalef +` on 16 bit U is 33925 (was 34053). From or to a 10-16 bit base and float, `scaleb` uses 255 steps: `i10 940 scaleb` is 0.92157 (was 0.91887). See [Converting levels](#converting-levels-scaleb-scalef). Unchanged at 8 bit base between integer depths with `scaleb`, on luma and RGB with `scalef`.
- **mt_merge `cplace="top_left"` on 4:2:2:** the 1-2-1 mask, as `"mpeg2"`: the horizontal siting is the same. It was the box of `"mpeg1"`.
- **`dup1`..`dup9` in expressions:** `a b dup1` gives `a b a` as documented and as in Expr. It overwrote the top of the stack and gave `a a a` (since 2.2.25). `dup` and `dup0` were right.
- **mt_edge, mt_motion `thC1`, `thC2` on float clips:** scaled as a difference, like `thY1`, `thY2`. On float YUV and float RGB an explicit value with the default `paramscale` was scaled as a chroma level and went negative, so the chroma mask was all 1 (since float support). With `paramscale="f32"` on integer YUV they were wrong as well.
- **Expressions are checked when the filter is created**, as in Expr: not enough operands (`1 +`) or more than one value left at the end (`1 2`) is an error with the reason. Before, only the 8-16 bit tables caught missing operands; realtime and float gave 0, and leftover values were ignored.
- **mt_expand, mt_inpand `mode` as a list:** an odd number of values is an error. The last pair read one value past the list.
- **mt_luts, mt_lutsx `pixels`:** an empty list or an odd number of values is an error. An empty list crashed in `"avg"` mode, the default, and gave a meaningless result in the others; an odd one read past the list.
- **mt_motion with luma not processed:** the luma plane still decides on a scene change for the whole frame. It read an uninitialized value.
- **mt_motion on very wide planes:** the scene change sum overflowed, in the worst case from width 32776 at 16 bit.
- **mt_motion at 8 bit on very large frames:** the scene change limit overflowed (from `thT=65` at 7680x4320), so no scene change was detected.
- **mt_polish:** an expression not convertible to its end is an error, it gave the part converted so far. Very small and very large numbers are kept exactly.
- **mt_lut with `use_expr` on stacked clips** runs the internal code. Expr computed the 8 bit double height clip.

### New

- **Formats:** YA at all depths; YUV(A) 4:1:1, 4:4:0 and 4:1:0 at all depths, with mt_merge's [luma mask](#luma-masks-on-subsampled-chroma-boxes-tents-and-triangles).
- **mt_edge `"sobel_real"`:** [the real Sobel](#sobel-is-a-quick-approximation-sobel_real-is-the-real-one) at all bit depths.
- **Expressions:** `sqrt`, `neg`, `sgn`, `not`; `pow`, `and`, `or`, `xor`; `width`, `height`, `sx`, `sy`, `sxr`, `syr`, `frameno`, `time`.
- **Platforms:** aarch64 with NEON on Linux and Windows.
- **mt_merge:** SSE2, AVX2 and NEON for every luma mask.
- **`opt`:** one parameter selects the code path, [on x86 and on aarch64](#opt).
- **`use_expr` is as fast as Expr itself:** Expr is set up once and takes the place of the filter, not set up for every frame. At 1080p it ran at about a sixth of Expr's speed before.
- **More AVX2:** in mt_binarize, mt_motion, mt_average and the morphological filters, at all depths, and AVX2 in mt_logic at 10-16 bit actually runs.

### Fixed crashes and wrong results without a compatibility side

- Boxes (`offX`, `offY`, `w`, `h`): a box empty after snapping crashed every filter. mt_lutspa, mt_hysteresis crashed on boxes as well. 1 pixel wide or high planes or boxes were read outside the plane in mt_edge, mt_mappedblur,
  and the morphological filters.
- mt_expand, mt_inpand SIMD read up to a vector past the end of the row. mt_edge custom kernels read outside planes 8 or 16 pixels wide. Planes exactly one vector wide got wrong edge pixels in the 8 bit SIMD code of mt_edge and the morphological filters.
- mt_mappedblur in the MSVC builds at 10-16 bit and float read the mask from wrong rows.
- mt_hysteresis was unstable with multithreading, and wrong with `corners=false` at 10-32 bit.
- mt_motion was unstable with multithreading: under `Prefetch`, the chroma of a frame could take the scene change decision of another frame. The filter stays `MT_NICE_FILTER`, the decision now belongs to each frame.
- mt_binarize at 10-16 bit `"t x"` and `"x t"`, and mt_merge float with `luma=true`, needed newer instructions than the CPU they were selected for.
- mt_luts, mt_lutsx, mt_lutspa: an invalid expression could crash instead of an error; mt_luts with weights could change results depending on memory contents.
- mt_infix: an invalid expression is an error, not a crash; `dup` and `swap` expand correctly. mt_polish: `"!&"` gives the `&!` the filters know.
- Allocating a too large table is an error message, not an unhandled exception.
- mt_luts at 16 bit with weights and `realtime=false` ignored the weights on aarch64.
- mt_polish accepts integers above 2147483647, the conversion stopped at them. Numbers above about 19 characters are no longer cut, and `1e300` no longer crashes.
- `use_expr` with `offX`, `offY`, `w` or `h` gives one clear error message: only the full frame can be passed to Expr.

### Internal

- AviSynth 2.5 is no longer supported; the minimum is classic AviSynth 2.6.
- Boost is no longer used, so `mt_polish` and `mt_infix` are in every build, including XP and Linux.
- Stacked processing goes through the native 16 bit code.
- One function signature, float parameters as float. The integer duplicates (dual signatures, against an older MaskTools with
  integer-only parameters loaded at the same time) are gone; integer arguments still work.
