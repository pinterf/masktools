#include "symbol.h"
#include <math.h>

using namespace Filtering;
using namespace Filtering::Parser;

// basic math
static double addition        (double x, double y) { return x + y; }
static double multiplication  (double x, double y) { return x * y; }
static double division        (double x, double y) { return x / y; }
static double substraction    (double x, double y) { return x - y; }
static double power           (double x, double y) { return pow(x, y); }
static double modulo          (double x, double y) { return double(convert<Int64, double>( x ) % convert<Int64, double>( y )); }
// Ternary operator helper
static double interrogation   (double x, double y, double z) { return x > 0 ? y : z; }
// comparison
static double equal           (double x, double y) { return abs(x - y) < 0.000001 ? 1 : -1; }
static double notEqual        (double x, double y) { return abs(x - y) >= 0.000001 ? 1 : -1; }
static double inferior        (double x, double y) { return x <= y ? 1 : -1; }
static double inferiorStrict  (double x, double y) { return x < y ? 1 : -1; }
static double superior        (double x, double y) { return x >= y ? 1 : -1; }
static double superiorStrict  (double x, double y) { return x > y ? 1 : -1; }
// bool
static double _and             (double x, double y) { return x > 0 && y > 0 ? 1 : -1; }
static double _or              (double x, double y) { return x > 0 || y > 0 ? 1 : -1; }
static double andNot          (double x, double y) { return x > 0 && y <= 0 ? 1 : -1; }
static double _xor             (double x, double y) { return (x > 0 && y <= 0) || (x <= 0 && y > 0)? 1 : -1; }
// Unsigned Bit arithmetic
static double andUB           (double x, double y) { return double(clip<Uint64, double>(x) & clip<Uint64, double>(y)); }
static double orUB            (double x, double y) { return double(clip<Uint64, double>(x) | clip<Uint64, double>(y)); }
static double xorUB           (double x, double y) { return double(clip<Uint64, double>(x) ^ clip<Uint64, double>(y)); }
static double negateUB        (double x) { return double(~clip<Uint64, double>(x)); }
static double posshiftUB      (double x, double y) { return y >= 0 ? double(clip<Uint64, double>(x) << clip<Int64, double>(y)) : double(clip<Uint64, double>(x) >> clip<Int64, double>(-y)); }
static double negshiftUB      (double x, double y) { return y >= 0 ? double(clip<Uint64, double>(x) >> clip<Int64, double>(y)) : double(clip<Uint64, double>(x) << clip<Int64, double>(-y)); }
// Signed Bit arithmetic
static double andSB           (double x, double y) { return double(clip<Int64, double>(x) & clip<Int64, double>(y)); }
static double orSB            (double x, double y) { return double(clip<Int64, double>(x) | clip<Int64, double>(y)); }
static double xorSB           (double x, double y) { return double(clip<Int64, double>(x) ^ clip<Int64, double>(y)); }
static double negateSB        (double x) { return double(~clip<Int64, double>(x)); }
static double posshiftSB      (double x, double y) { return y >= 0 ? double(clip<Int64, double>(x) << clip<Int64, double>(y)) : double(clip<Int64, double>(x) >> clip<Int64, double>(-y)); }
static double negshiftSB      (double x, double y) { return y >= 0 ? double(clip<Int64, double>(x) >> clip<Int64, double>(y)) : double(clip<Int64, double>(x) << clip<Int64, double>(-y)); }
// Math
static double mtcos             (double x) { return cos(x); }
static double mtsin             (double x) { return sin(x); }
static double mttan             (double x) { return tan(x); }
static double mtexp             (double x) { return exp(x); }
static double mtlog             (double x) { return log(x); }
static double mtmabs            (double x) { return abs(x); }
static double mtacos            (double x) { return acos(x); }
static double mtasin            (double x) { return asin(x); }
static double mtatan            (double x) { return atan(x); }
static double mtround           (double x) { return double(convert<Int64, double>( x )); }
static double mtclip            (double x, double y, double z) { return clip<double, double>( x, y, z ); }
static double mtmin             (double x, double y) { return min<double>( x, y ); }
static double mtmax             (double x, double y) { return max<double>( x, y ); }
static double mtfloor           (double x) { return floor(x); }
static double mtceil            (double x) { return ceil(x); }
static double mttrunc           (double x) { return double(Int64(x)); }
// bit depth conversion helpers. x:value on 8 bit scale y: target bit depth 8-32 z:base bit depth
static double do_upscaleByShift(double x, int y, int z, bool chroma, bool shift_float)
{
  const int target_bit_depth = y;
  const int source_bit_depth = z;

  if (target_bit_depth == source_bit_depth) return x; // same bit 
  
  if (target_bit_depth == 32) { // target float
    if (chroma) {
      const int src_middle_chroma = 1 << (source_bit_depth - 1);
      const int max_pixel_value_source = (1 << source_bit_depth) - 1;
#ifdef FLOAT_CHROMA_IS_HALF_CENTERED
      return (x - src_middle_chroma) / max_pixel_value_source + 0.5;
#else
      double result = (x - src_middle_chroma) / max_pixel_value_source;
      if (shift_float) result += 0.5; // "floatUV"
      return result;
#endif
    }
    else {
      const int max_pixel_value_source = (1 << source_bit_depth) - 1;
      return x / max_pixel_value_source;
    }
  }
  if (source_bit_depth == 32) {
    // treat original as float, target 8-16 bits
    // for 32bit float: same as scaleb
    if (chroma) {
      const int target_middle_chroma = 1 << (target_bit_depth - 1);
      const int max_pixel_value_target = (1 << target_bit_depth) - 1;
#ifdef FLOAT_CHROMA_IS_HALF_CENTERED
      return (x - 0.5) * max_pixel_value_target + target_middle_chroma;
#else
      return (x - 0.0) * max_pixel_value_target + target_middle_chroma;
#endif
    }
    else {
      const int max_pixel_value_target = (1 << target_bit_depth) - 1;
      return x * max_pixel_value_target;
    }
  }
  // 8-16 <-> 8-16
  if (target_bit_depth > source_bit_depth) // e.g. 8-->10
    return double(x * (1 << (target_bit_depth - source_bit_depth))); // upscale, mul by 2^N bits
  else
    return double(x / (1 << (source_bit_depth - target_bit_depth))); // downscale: div by 2^N bits
}

static double upscaleByShift(double x, int y, int z, bool chroma, bool shift_float)
{
  return do_upscaleByShift(x, y, z, chroma, shift_float);
}

static double upscaleByShiftY(double x, int y, int z, bool chroma, bool shift_float)
{
  UNUSED(chroma);
  return do_upscaleByShift(x, y, z, false, shift_float); // force non-chroma
}

static double do_upscaleByStretch(double x, int y, int z, bool chroma, bool shift_float) // e.g. 8->10 bit rgb: x/255*1023
{
  const int target_bit_depth = y;
  const int source_bit_depth = z;
  if (target_bit_depth == source_bit_depth) return x; // same bit 
  
  if (target_bit_depth == 32) { // target float
    // for 32bit float: same as scaleb
    if (chroma) {
      const int src_middle_chroma = 1 << (source_bit_depth - 1);
      const int max_pixel_value_source = (1 << source_bit_depth) - 1;
#ifdef FLOAT_CHROMA_IS_HALF_CENTERED
      return (x - src_middle_chroma) / max_pixel_value_source + 0.5;
#else
      double result = (x - src_middle_chroma) / max_pixel_value_source;
      if (shift_float) result += 0.5; // "floatUV"
      return result;
#endif
    }
    else {
      const int max_pixel_value_source = (1 << source_bit_depth) - 1;
      return x / max_pixel_value_source;
    }
  }
  if (source_bit_depth == 32) {
    // treat original as float, target 8-16 
    // for 32bit float: same as scaleb
    if (chroma) {
      const int target_middle_chroma = 1 << (target_bit_depth - 1);
      const int max_pixel_value_target = (1 << target_bit_depth) - 1;
#ifdef FLOAT_CHROMA_IS_HALF_CENTERED
      return (x - 0.5) * max_pixel_value_target + target_middle_chroma;
#else
      return (x - 0.0) * max_pixel_value_target + target_middle_chroma;
#endif
    }
    else {
      const int max_pixel_value_target = (1 << target_bit_depth) - 1;
      return x * max_pixel_value_target;
    }
  }
  // 8-16 <-> 8-16
  const int max_pixel_value_source = (1 << source_bit_depth) - 1;
  const int max_pixel_value_target = (1 << target_bit_depth) - 1;
  return double(x * max_pixel_value_target / max_pixel_value_source); // upscale or downscale
}

static double upscaleByStretch(double x, int y, int z, bool chroma, bool shift_float) // e.g. 8->10 bit rgb: x/255*1023
{
  return do_upscaleByStretch(x, y, z, chroma, shift_float);
}

static double upscaleByStretchY(double x, int y, int z, bool chroma, bool shift_float) // e.g. 8->10 bit rgb: x/255*1023
{
  UNUSED(chroma);
  return do_upscaleByStretch(x, y, z, false, shift_float); // for non-chroma
}

Symbol Symbol::Addition       ("+" , OPERATOR, addition);
Symbol Symbol::Multiplication ("*" , OPERATOR, multiplication);
Symbol Symbol::Division       ("/" , OPERATOR, division);
Symbol Symbol::Substraction   ("-" , OPERATOR, substraction);
Symbol Symbol::Power          ("^" , OPERATOR, power);
Symbol Symbol::Modulo         ("%" , OPERATOR, modulo);
Symbol Symbol::Interrogation  ("?" , TERNARY , interrogation);
Symbol Symbol::Equal          ("==", OPERATOR, equal);
Symbol Symbol::Equal2         ("=", OPERATOR, equal);
Symbol Symbol::NotEqual       ("!=", OPERATOR, notEqual);
Symbol Symbol::Inferior       ("<=", OPERATOR, inferior);
Symbol Symbol::InferiorStrict ("<" , OPERATOR, inferiorStrict);
Symbol Symbol::Superior       (">=", OPERATOR, superior);
Symbol Symbol::SuperiorStrict (">" , OPERATOR, superiorStrict);
Symbol Symbol::And            ("&" , OPERATOR, _and);
Symbol Symbol::Or             ("|" , OPERATOR, _or);
Symbol Symbol::AndNot         ("&!", OPERATOR, andNot);
Symbol Symbol::Xor            ("\xB0" , "@", OPERATOR, _xor); // degree-sign 0xB0, has codepage problems, kept only for compatibility
Symbol Symbol::AndUB          ("&u" , OPERATOR, andUB);
Symbol Symbol::OrUB           ("|u" , OPERATOR, orUB);
Symbol Symbol::XorUB          ("\xB0u" , "@u", OPERATOR, xorUB); // degree-sign 0xB0, has codepage problems, kept only for compatibility
Symbol Symbol::NegateUB       ("~u" , FUNCTION, negateUB);
Symbol Symbol::PosShiftUB     ("<<", "<<u", OPERATOR, posshiftUB);
Symbol Symbol::NegShiftUB     (">>", ">>u", OPERATOR, negshiftUB);
Symbol Symbol::AndSB          ("&s" , OPERATOR, andSB);
Symbol Symbol::OrSB           ("|s" , OPERATOR, orSB);
Symbol Symbol::XorSB          ("\xB0s" , "@s", OPERATOR, xorSB); // degree-sign 0xB0, has codepage problems, kept only for compatibility
Symbol Symbol::NegateSB       ("~s" , FUNCTION, negateSB);
Symbol Symbol::PosShiftSB     ("<<s", OPERATOR, posshiftSB);
Symbol Symbol::NegShiftSB     (">>s", OPERATOR, negshiftSB);
Symbol Symbol::Pi             ("pi", 3.1415927, NUMBER  , 0, NULL);
// Lut variables
Symbol Symbol::X              ("x" , VARIABLE, VARIABLE_X);  
Symbol Symbol::Y              ("y" , VARIABLE, VARIABLE_Y);
Symbol Symbol::Z              ("z" , VARIABLE, VARIABLE_Z);
// preliminary lut variable: A(lpha)
Symbol Symbol::A              ("a" , VARIABLE, VARIABLE_A);
// global bitdepth parameter for autoscale, since v2.2.1
Symbol Symbol::BITDEPTH       ("bitdepth" , VARIABLE, VARIABLE_BITDEPTH);
Symbol Symbol::SCRIPT_BITDEPTH("sbitdepth", VARIABLE, VARIABLE_SCRIPT_BITDEPTH);
// bit-depth adaptive constants, since v2.2.1
Symbol Symbol::RANGE_HALF     ("range_half", VARIABLE, VARIABLE_RANGE_HALF); // 128  scaled
Symbol Symbol::YRANGE_HALF    ("yrange_half", VARIABLE, VARIABLE_YRANGE_HALF);
Symbol Symbol::RANGE_MIN      ("range_min", VARIABLE, VARIABLE_RANGE_MIN);   // 0, or -0.5 for 32 bit float chroma
Symbol Symbol::YRANGE_MIN     ("yrange_min", VARIABLE, VARIABLE_YRANGE_MIN);
Symbol Symbol::RANGE_MAX      ("range_max", VARIABLE, VARIABLE_RANGE_MAX);   // 255, 4095, .. 65535
Symbol Symbol::YRANGE_MAX     ("yrange_max", VARIABLE, VARIABLE_YRANGE_MAX);
Symbol Symbol::RANGE_SIZE     ("range_size", VARIABLE, VARIABLE_RANGE_SIZE); // 256, 1024, 4096, 16384, 65536
Symbol Symbol::YMIN           ("ymin", VARIABLE, VARIABLE_YMIN); // 16 scaled
Symbol Symbol::YMAX           ("ymax", VARIABLE, VARIABLE_YMAX); // 235 or scaled
Symbol Symbol::CMIN           ("cmin", VARIABLE, VARIABLE_CMIN); // 16 scaled = LIMITED_YMIN
Symbol Symbol::CMAX           ("cmax", VARIABLE, VARIABLE_CMAX); // 240 scaled
Symbol Symbol::YFMIN          ("yfmin", VARIABLE, VARIABLE_YFMIN);
Symbol Symbol::YFMAX          ("yfmax", VARIABLE, VARIABLE_YFMAX);
Symbol Symbol::CFMIN          ("cfmin", VARIABLE, VARIABLE_CFMIN);
Symbol Symbol::CFMAX          ("cfmax", VARIABLE, VARIABLE_CFMAX);
// Math
Symbol Symbol::Cos            ("cos", FUNCTION, mtcos);
Symbol Symbol::Sin            ("sin", FUNCTION, mtsin);
Symbol Symbol::Tan            ("tan", FUNCTION, mttan);
Symbol Symbol::Log            ("log", FUNCTION, mtlog);
Symbol Symbol::Exp            ("exp", FUNCTION, mtexp);
Symbol Symbol::Abs            ("abs", FUNCTION, mtmabs);
Symbol Symbol::Atan           ("atan", FUNCTION, mtatan);
Symbol Symbol::Acos           ("acos", FUNCTION, mtacos);
Symbol Symbol::Asin           ("asin", FUNCTION, mtasin);
Symbol Symbol::Round          ("round", FUNCTION, mtround);
Symbol Symbol::Clip           ("clip", FUNCTION, mtclip);
Symbol Symbol::Min            ("min", FUNCTION, mtmin);
Symbol Symbol::Max            ("max", FUNCTION, mtmax);
Symbol Symbol::Ceil           ("ceil", FUNCTION, mtceil);
Symbol Symbol::Floor          ("floor", FUNCTION, mtfloor);
Symbol Symbol::Trunc          ("trunc", FUNCTION, mttrunc);
// automatic bit-depth scaling helpers, since v2.2.1
Symbol Symbol::ScaleByShift   ("scaleb", "@B", FUNCTION_WITH_BITDEPTH_AS_AUTOPARAM, upscaleByShift); // v 2.2.5: #B, #F -> @B, @F
Symbol Symbol::ScaleByStretch ("scalef", "@F", FUNCTION_WITH_BITDEPTH_AS_AUTOPARAM, upscaleByStretch); // with optinal scaleb and scalef aliases
Symbol Symbol::ScaleByShiftY  ("yscaleb", FUNCTION_WITH_BITDEPTH_AS_AUTOPARAM, upscaleByShiftY); // v 2.2.5: #B, #F -> @B, @F
Symbol Symbol::ScaleByStretchY("yscalef", FUNCTION_WITH_BITDEPTH_AS_AUTOPARAM, upscaleByStretchY); // with optinal scaleb and scalef aliases
// admin config
Symbol Symbol::SetScriptBitDepthI8("i8", 8.0, FUNCTION_CONFIG_SCRIPT_BITDEPTH, 0, NULL);
Symbol Symbol::SetScriptBitDepthI10("i10", 10.0, FUNCTION_CONFIG_SCRIPT_BITDEPTH, 0, NULL);
Symbol Symbol::SetScriptBitDepthI12("i12", 12.0, FUNCTION_CONFIG_SCRIPT_BITDEPTH, 0, NULL);
Symbol Symbol::SetScriptBitDepthI14("i14", 14.0, FUNCTION_CONFIG_SCRIPT_BITDEPTH, 0, NULL);
Symbol Symbol::SetScriptBitDepthI16("i16", 16.0, FUNCTION_CONFIG_SCRIPT_BITDEPTH, 0, NULL);
Symbol Symbol::SetScriptBitDepthF32("f32", 32.0, FUNCTION_CONFIG_SCRIPT_BITDEPTH, 0, NULL);
Symbol Symbol::SetFloatToClampUseI8Range("clamp_f_i8", -8.0, FUNCTION_CONFIG_SCRIPT_BITDEPTH, 0, NULL);
Symbol Symbol::SetFloatToClampUseI10Range("clamp_f_i10", -10.0, FUNCTION_CONFIG_SCRIPT_BITDEPTH, 0, NULL);
Symbol Symbol::SetFloatToClampUseI12Range("clamp_f_i12", -12.0, FUNCTION_CONFIG_SCRIPT_BITDEPTH, 0, NULL);
Symbol Symbol::SetFloatToClampUseI14Range("clamp_f_i14", -14.0, FUNCTION_CONFIG_SCRIPT_BITDEPTH, 0, NULL);
Symbol Symbol::SetFloatToClampUseI16Range("clamp_f_i16", -16.0, FUNCTION_CONFIG_SCRIPT_BITDEPTH, 0, NULL);
Symbol Symbol::SetFloatToClampUseF32Range("clamp_f_f32", -32.0, FUNCTION_CONFIG_SCRIPT_BITDEPTH, 0, NULL);
Symbol Symbol::SetFloatToClampUseF32Range_2("clamp_f", -32.0, FUNCTION_CONFIG_SCRIPT_BITDEPTH, 0, NULL);

Symbol Symbol::Dup("dup", DUP, 0); // dup 0
Symbol Symbol::Dup0("dup0", DUP, 0); // duplicates Nth stack element to the top
Symbol Symbol::Dup1("dup1", DUP, 1);
Symbol Symbol::Dup2("dup2", DUP, 2);
Symbol Symbol::Dup3("dup3", DUP, 3);
Symbol Symbol::Dup4("dup4", DUP, 4);
Symbol Symbol::Dup5("dup5", DUP, 5);
Symbol Symbol::Dup6("dup6", DUP, 6);
Symbol Symbol::Dup7("dup7", DUP, 7);
Symbol Symbol::Dup8("dup8", DUP, 8);
Symbol Symbol::Dup9("dup9", DUP, 9);
Symbol Symbol::Swap("swap", SWAP, 1); // swap 1 and 2 (n and top)
Symbol Symbol::Swap1("swap1", SWAP, 1); // swaps Nth stack element with the top (top is 0)
Symbol Symbol::Swap2("swap2", SWAP, 2);
Symbol Symbol::Swap3("swap3", SWAP, 3);
Symbol Symbol::Swap4("swap4", SWAP, 4);
Symbol Symbol::Swap5("swap5", SWAP, 5);
Symbol Symbol::Swap6("swap6", SWAP, 6);
Symbol Symbol::Swap7("swap7", SWAP, 7);
Symbol Symbol::Swap8("swap8", SWAP, 8);
Symbol Symbol::Swap9("swap9", SWAP, 9);

Symbol::Symbol() :
type(UNDEFINED), value(""), value2("")
{
}

// direct numeric literals from parser
Symbol::Symbol(String value, Type type) :
  type(type), vartype(VARIABLE_UNDEFINED), value(value), value2(""), nParameter(0), process0(NULL), process1(NULL), process2(NULL), process3(NULL)
{
  if (type == NUMBER)
    dValue = atof(value.c_str());
}


// dup, swap
// dup0 .. dup9, swap1..swap9
Symbol::Symbol(String value, Type type, int nParameter) :
  type(type), vartype(VARIABLE_UNDEFINED), value(value), value2(""), nParameter(nParameter), process0(NULL), process1(NULL), process2(NULL), process3(NULL)
{
}

// function, operator, ternary

Symbol::Symbol(String value, Type type, Process0 process) :
  type(type), vartype(VARIABLE_UNDEFINED), value(value), value2(""), nParameter(0), process0(process)
{
}

Symbol::Symbol(String value, Type type, Process1 process) :
type(type), vartype(VARIABLE_UNDEFINED), value(value), value2(""), nParameter(1), process1(process) 
{
}

Symbol::Symbol(String value, Type type, Process2 process) :
  type(type), vartype(VARIABLE_UNDEFINED), value(value), value2(""), nParameter(2), process2(process)
{
}

Symbol::Symbol(String value, Type type, Process3 process) :
  type(type), vartype(VARIABLE_UNDEFINED), value(value), value2(""), nParameter(3), process3(process)
{
}

// two tokens
Symbol::Symbol(String value, String value2, Type type, Process1 process) :
type(type), vartype(VARIABLE_UNDEFINED), value(value), value2(value2), nParameter(1), process1(process)
{
}

Symbol::Symbol(String value, String value2, Type type, Process2 process) :
  type(type), vartype(VARIABLE_UNDEFINED), value(value), value2(value2), nParameter(2), process2(process)
{
}

Symbol::Symbol(String value, String value2, Type type, Process3 process) :
  type(type), vartype(VARIABLE_UNDEFINED), value(value), value2(value2), nParameter(3), process3(process)
{
}

Symbol::Symbol(String value, String value2, Type type, ProcessScale process) :
  type(type), vartype(VARIABLE_UNDEFINED), value(value), value2(value2), nParameter(3), processScale(process)
{
}

Symbol::Symbol(String value, Type type, ProcessScale process) :
  type(type), vartype(VARIABLE_UNDEFINED), value(value), value2(""), nParameter(3), processScale(process)
{
}

// variables
// Type=VARIABLE always
Symbol::Symbol(String value, Type type, VarType vartype) :
  type(type), vartype(vartype), value(value), value2(""), nParameter(0)
{
}

// predefined numeric constants, like "Pi"
// Type=NUMBER always
Symbol::Symbol(String value, double dValue, Type type, int nParameter, Process1 process) :
type(type), vartype(VARIABLE_UNDEFINED), value(value), value2(""), nParameter(nParameter), dValue(dValue), process1(process)
{
}

/* dead code
void Symbol::setValue(double _dValue)
{
   this->dValue = _dValue;
}
*/

// called from rec_compute_old, why
double Symbol::getValue(double x, double y, double z) const
{
   switch ( type )
   {
   case VARIABLE: // ?
   case NUMBER:
     return dValue;
   default:
      return process3(x, y, z);
   }
}


void Context::calc_helpers()
{
  chroma_center_i = (float)(1 << (sbitdepth - 1));
#ifdef FLOAT_CHROMA_IS_HALF_CENTERED
  chroma_center_f = 0.5;
  chroma_lo_f = 0.0;
  chroma_hi_f = 1.0;
#else
  chroma_center_f = 0.0;
  chroma_lo_f = -0.5;
  chroma_hi_f = 0.5;
#endif

  float_input_scalefactor = 1.0; // for default and 32 bit
  if (scale_float && !fullrange_autoscale) {
    if (sbitdepth >= 8 && sbitdepth <= 16)
      float_input_scalefactor = float((1 << sbitdepth));
  }
  else {
    if (sbitdepth >= 8 && sbitdepth <= 16)
      float_input_scalefactor = float((1 << sbitdepth) - 1);
  }
  float_input_invscalefactor = 1.0f / float_input_scalefactor;
}

// true on error
bool Context::SetScaleInputs(String scale_inputs)
{
  // v2.2.15-
  shift_float = false;

  if (scale_inputs == "int") {
    fullrange_autoscale = true;
    scale_int = true;
    scale_float = false;
  } else if (scale_inputs == "intf") {
    fullrange_autoscale = true;
    scale_int = true;
    scale_float = false;
  }
  else if (scale_inputs == "floatUV") { 
    // v2.2.20
    // On chroma planes: when (!scale_float || sbitdepth == 32) then this option 
    // pre-shifts chroma pixels by +0.5 before applying the expression, then the result is shifted back by -0.5
    // Thus chroma expressions, which rely on a working range of 0..1.0 will work transparently 
    fullrange_autoscale = false;
    scale_int = false;
    scale_float = false; // !!
    shift_float = true; // !!
  }
  else if (scale_inputs == "float") {
    fullrange_autoscale = false;
    scale_int = false;
    scale_float = true;
  }
  else if (scale_inputs == "floatf") {
    fullrange_autoscale = true;
    scale_int = false;
    scale_float = true;
  }
  else if (scale_inputs == "all") {
    fullrange_autoscale = false;
    scale_int = true;
    scale_float = true;
  }
  else if (scale_inputs == "allf") {
    fullrange_autoscale = true;
    scale_int = true;
    scale_float = true;
  }
  else if (scale_inputs == "none") {
    scale_int = false;
    scale_float = false;
  }
  else {
    return true; // error
  }

#ifndef FLOAT_CHROMA_IS_HALF_CENTERED
  if (shift_float) {
    range_half_f[1] += 0.5; // chroma
    range_min_f[1] += 0.5;
    range_max_f[1] += 0.5;
    cmin_f += 0.5;
    cmax_f += 0.5;
  }
#endif

  calc_helpers();
  return false;
}


Context::Context(const std::deque<Symbol> &expression, String scale_inputs, int param_clamp_float_i) : Context(expression)
{
  clamp_float_i = param_clamp_float_i; // SetScaleInputs can override to true
  SetScaleInputs(scale_inputs);
}

Context::Context(const std::deque<Symbol> &expression)
{
   nPos = -1;
   nSymbols = expression.size();
   pSymbols = new Symbol[nSymbols];
   exprstack = new double[nSymbols];

   nSymbols_control = expression.size();
   pSymbols_control = new Symbol[nSymbols];

   // new v2.2.15 scaling
   fullrange_autoscale = false;
   scale_int = false;
   scale_float = false;
   shift_float = false;

   auto it = expression.begin();

   sbitdepth = 8; // default source bit depth for expressions, used in scale_inputs and scaleb/scalef/yscalef,yscaleb
   // scaleb/scalef converts constants from this bit depth to the current one
   // when scale_inputs == true, this is the target bit depth of the internal scaling

   //int default_float_autoscale_bitdepth = 0; // no clamp and autoscale
   //int default_all_autoscale_bitdepth = 0; // general autoscale v2.2.15-
   //bool default_fullrange_autoscale = false;

   int symbolCount = 0;
   int symbolCount_control = 0;
   for (int i = 0; i < nSymbols; i++, it++) {
     // control mnemonics are filtered here, they are not put in the expression
     if (it->type == Symbol::FUNCTION_CONFIG_SCRIPT_BITDEPTH) {
       pSymbols_control[symbolCount_control++] = *it;
       if (it->dValue > 0) {
         // i8, i10, i12, i14, i16, f32
         sbitdepth = (int)it->dValue;
       }
       else {
         // negative values show the bitdepth that inputs and output should autoscale.
         // clamp_f
         // clamp_f_i8..clamp_f_i16..clamp_f_f32
         //default_float_autoscale_bitdepth = -(int)it->dValue;
         scale_int = false;
         scale_float = true;
         shift_float = false;
         sbitdepth = -(int)it->dValue;
       }
     }
     else {
       pSymbols[symbolCount++] = *it;
     }
   }
   nSymbols = symbolCount;
   nSymbols_control = symbolCount_control;

   //sbitdepth = default_sbitdepth;

   // fill predefined constants for faster rec_compute access. Integer bit-depth only
   for (int bits = 8; bits <= 16; bits++) {
     a_range_half[bits - 8] = 128 << (bits - 8); // 
     a_range_min[bits - 8] = 0; // min_pixel_value. 8*16bit: 0, for the sake of completeness
     a_range_max[bits - 8] = (1 << bits) - 1; // max_pixel_value. 255, 1023, 4095, 16383, 65535
     a_range_size[bits - 8] = (1 << bits); // 256, 1024, 4096, 16384, 65536
     a_ymin[bits - 8] = 16 << (bits - 8); // 16 scaled
     a_ymax[bits - 8] = 235 << (bits - 8);// 235 scaled
     a_cmin[bits - 8] = 16 << (bits - 8); // 16 scaled
     a_cmax[bits - 8] = 240 << (bits - 8);// 240 scaled
   }

   // [0]: luma, [1]: chroma
   range_half_f[0] = 0.5;
#ifdef FLOAT_CHROMA_IS_HALF_CENTERED
   range_half_f[1] = 0.5; // chroma
#else
   range_half_f[1] = 0.0;
#endif

   // [0]: luma, [1]: chroma
   range_min_f[0] = 0.0;
   range_max_f[0] = 1.0;
#ifdef FLOAT_CHROMA_IS_HALF_CENTERED
   range_min_f[1] = 0.0;
   range_max_f[1] = 1.0;
#else
   range_min_f[1] = -0.5;
   range_max_f[1] = 0.5;
#endif

   range_size_f = 1.0;

   ymin_f = 16.0 / 255;
   ymax_f = 235.0 / 255;
   cmin_f = (16.0 - 128.0)/ 255;
   cmax_f = (240.0 - 128.0) / 255;
#ifdef FLOAT_CHROMA_IS_HALF_CENTERED
   cmin_f += 0.5f; // back to 0.5 centered
   cmax_f += 0.5f;
#endif

   // target bitdepth, into which float is converted
   // float_autoscale_bitdepth = default_float_autoscale_bitdepth; always sbitdepth

   // target bitdepth, into which any input bitdepth is converted
   //all_autoscale_bitdepth = default_all_autoscale_bitdepth;always sbitdepth

   // when all_autoscale is specified, it applies also to float input (old masktools2 option)
   //if (float_autoscale_bitdepth == 0 && all_autoscale_bitdepth != 0)
   //  float_autoscale_bitdepth = all_autoscale_bitdepth;

   // precalculate scale factor and its inverse
   calc_helpers();
}

Context::~Context()
{
   delete[] pSymbols;
   delete[] exprstack;
   delete[] pSymbols_control;
}

double Context::rec_compute()
{
  double last = 0;

  Symbol &s_first = pSymbols[0];

  int p = 0;

  switch (s_first.type)
  {
  case Symbol::NUMBER: { last = s_first.dValue; break; }
  case Symbol::VARIABLE: {
    switch (s_first.vartype) {
    case Symbol::VARIABLE_X: { last = x; break; }
    case Symbol::VARIABLE_Y: { last = y; break; }
    case Symbol::VARIABLE_Z: { last = z; break; }
    case Symbol::VARIABLE_A: { last = a; break; }
    case Symbol::VARIABLE_BITDEPTH: { last = bitdepth; break; } // bit-depth for autoscale
    case Symbol::VARIABLE_SCRIPT_BITDEPTH: { last = (double)sbitdepth; break; } // source bit depth for autoscale

    // these are adaptively returning the range for the actually processed plane, which can be luma or chroma
    case Symbol::VARIABLE_RANGE_HALF: { last = bitdepth == 32 ? range_half_f[luma_chroma] : a_range_half[bitdepth - 8]; break; } // 0.0 for float chroma
    case Symbol::VARIABLE_RANGE_MIN: { last = bitdepth == 32 ? range_min_f[luma_chroma] : a_range_min[bitdepth - 8]; break; }// min_pixel_value. 0 or -0.5
    case Symbol::VARIABLE_RANGE_MAX: { last = bitdepth == 32 ? range_max_f[luma_chroma] : a_range_max[bitdepth - 8]; break; }// max_pixel_value. 255, 1023, 4095, 16383, 65535 (1.0 for float, 0.5 float chroma

    // these are always returning the range for Y
    case Symbol::VARIABLE_YRANGE_HALF: { last = bitdepth == 32 ? range_half_f[0] : a_range_half[bitdepth - 8]; break; }  // 0.5 for float chroma
    case Symbol::VARIABLE_YRANGE_MIN: { last = bitdepth == 32 ? range_min_f[0] : a_range_min[bitdepth - 8]; break; }// min_pixel_value. 0
    case Symbol::VARIABLE_YRANGE_MAX: { last = bitdepth == 32 ? range_max_f[0] : a_range_max[bitdepth - 8]; break; }// max_pixel_value. 255, 1023, 4095, 16383, 65535 (1.0 for float)

    case Symbol::VARIABLE_RANGE_SIZE: { last = bitdepth == 32 ? range_size_f : a_range_size[bitdepth - 8]; break; } // 256, 1024, 4096, 16384, 65536 (1.0 for float)
    case Symbol::VARIABLE_YMIN: { last = bitdepth == 32 ? ymin_f : a_ymin[bitdepth - 8]; break;  }   // 16 scaled
    case Symbol::VARIABLE_YMAX: { last = bitdepth == 32 ? ymax_f : a_ymax[bitdepth - 8]; break; } // 235 scaled
    case Symbol::VARIABLE_CMIN: { last = bitdepth == 32 ? cmin_f : a_cmin[bitdepth - 8]; break;  } // 16 scaled
    case Symbol::VARIABLE_CMAX: { last = bitdepth == 32 ? cmax_f : a_cmax[bitdepth - 8]; break;  } // 240 scaled
    case Symbol::VARIABLE_YFMIN: { last = bitdepth == 32 ? ymin_f : bitdepth == 10 ? 64 : bitdepth == 12 ? 257 : bitdepth == 14 ? 1028 : bitdepth == 16 ? 4112 : 16; break;  }
    case Symbol::VARIABLE_YFMAX: { last = bitdepth == 32 ? ymax_f : bitdepth == 10 ? 1175 : bitdepth == 12 ? 3995 : bitdepth == 14 ? 15275 : bitdepth == 16 ? 60395 : 235; break; }
    case Symbol::VARIABLE_CFMIN: { last = bitdepth == 32 ? cmin_f : bitdepth == 10 ? 64 : bitdepth == 12 ? 257 : bitdepth == 14 ? 1028 : bitdepth == 16 ? 4112 : 16; break;  }
    case Symbol::VARIABLE_CFMAX: { last = bitdepth == 32 ? cmax_f : bitdepth == 10 ? 1200 : bitdepth == 12 ? 4080 : bitdepth == 14 ? 15600 : bitdepth == 16 ? 61680 : 240; break;  }
    default: assert(0);
    }
    break;
  }
  default: return 0; // assert
  }

  for (int i = 1; i < nPos; i++) {
    Symbol &s = pSymbols[i];

    switch (s.type)
    {
    case Symbol::NUMBER: { exprstack[p++] = last; last=s.dValue; break; }
    case Symbol::VARIABLE: {
      switch (s.vartype) {
      case Symbol::VARIABLE_X: { exprstack[p++] = last; last = x; break; }
      case Symbol::VARIABLE_Y: { exprstack[p++] = last; last = y; break; }
      case Symbol::VARIABLE_Z: { exprstack[p++] = last; last = z; break; }
      case Symbol::VARIABLE_A: { exprstack[p++] = last; last = a; break; }
      case Symbol::VARIABLE_BITDEPTH: { exprstack[p++] = last; last = bitdepth; break; } // bit-depth for autoscale
      case Symbol::VARIABLE_SCRIPT_BITDEPTH: { exprstack[p++] = last; last = (double)sbitdepth; break; } // source bit depth for autoscale

      case Symbol::VARIABLE_RANGE_HALF: { exprstack[p++] = last; last = bitdepth == 32 ? range_half_f[luma_chroma] : a_range_half[bitdepth - 8]; break; } // or 0.0 for float in the future?
      case Symbol::VARIABLE_RANGE_MIN: { exprstack[p++] = last; last = bitdepth == 32 ? range_min_f[luma_chroma] : a_range_min[bitdepth - 8]; break; } // min_pixel_value. 0 or -0.5f
      case Symbol::VARIABLE_RANGE_MAX: { exprstack[p++] = last; last = bitdepth == 32 ? range_max_f[luma_chroma] : a_range_max[bitdepth - 8]; break; } // max_pixel_value. 255, 1023, 4095, 16383, 65535 (0.5 for float)
      case Symbol::VARIABLE_YRANGE_HALF: { exprstack[p++] = last; last = bitdepth == 32 ? range_half_f[0] : a_range_half[bitdepth - 8]; break; } // 0.5
      case Symbol::VARIABLE_YRANGE_MIN: { exprstack[p++] = last; last = bitdepth == 32 ? range_min_f[0] : a_range_min[bitdepth - 8]; break; } // min_pixel_value. 0
      case Symbol::VARIABLE_YRANGE_MAX: { exprstack[p++] = last; last = bitdepth == 32 ? range_max_f[0] : a_range_max[bitdepth - 8]; break; } // max_pixel_value. 255, 1023, 4095, 16383, 65535 (1.0 for float)
      case Symbol::VARIABLE_RANGE_SIZE: { exprstack[p++] = last; last = bitdepth == 32 ? range_size_f : a_range_size[bitdepth - 8]; break; } // 256, 1024, 4096, 16384, 65536 (1.0 for float)
      case Symbol::VARIABLE_YMIN: { exprstack[p++] = last; last = bitdepth == 32 ? ymin_f : a_ymin[bitdepth - 8]; break;  }   // 16 scaled
      case Symbol::VARIABLE_YMAX: { exprstack[p++] = last; last = bitdepth == 32 ? ymax_f : a_ymax[bitdepth - 8]; break; } // 235 scaled
      case Symbol::VARIABLE_CMIN: { exprstack[p++] = last; last = bitdepth == 32 ? cmin_f : a_cmin[bitdepth - 8]; break;  } // 16 scaled
      case Symbol::VARIABLE_CMAX: { exprstack[p++] = last; last = bitdepth == 32 ? cmax_f : a_cmax[bitdepth - 8]; break;  } // 240 scaled
      case Symbol::VARIABLE_YFMIN: { exprstack[p++] = last; last = bitdepth == 32 ? ymin_f : bitdepth == 10 ? 64 : bitdepth == 12 ? 257 : bitdepth == 14 ? 1028 : bitdepth == 16 ? 4112 : 16; break;  }
      case Symbol::VARIABLE_YFMAX: { exprstack[p++] = last; last = bitdepth == 32 ? ymax_f : bitdepth == 10 ? 1175 : bitdepth == 12 ? 3995 : bitdepth == 14 ? 15275 : bitdepth == 16 ? 60395 : 235; break; }
      case Symbol::VARIABLE_CFMIN: { exprstack[p++] = last; last = bitdepth == 32 ? cmin_f : bitdepth == 10 ? 64 : bitdepth == 12 ? 257 : bitdepth == 14 ? 1028 : bitdepth == 16 ? 4112 : 16; break;  }
      case Symbol::VARIABLE_CFMAX: { exprstack[p++] = last; last = bitdepth == 32 ? cmax_f : bitdepth == 10 ? 1200 : bitdepth == 12 ? 4080 : bitdepth == 14 ? 15600 : bitdepth == 16 ? 61680 : 240; break;  }
      default: assert(0);
      }
      break;
    }

    case Symbol::DUP: {
      int distance = s.nParameter; // 1..9  duplicates the nth distance stack value to the top
      if(distance != 0)
        last = exprstack[p - distance];
      exprstack[p++] = last;
      break;
    }

    case Symbol::SWAP:
    {
      int distance = s.nParameter; // 1..9  exchanges stacktop with the nth distance stack value
      double p1 = exprstack[p - distance];
      exprstack[p - distance] = last;
      last = p1;
      break;
    }

    case Symbol::FUNCTION_WITH_BITDEPTH_AS_AUTOPARAM: // silent bit-depth parameter for autoscale
      // only exists with one user parameter, plus three silent params
      last = s.processScale(last, bitdepth, sbitdepth, luma_chroma == 1, shift_float);
      break;

    // OPERATOR, FUNCTION, TERNARY
    default:
      switch (s.nParameter)
      {
      case 2:
      {
        double xx = exprstack[--p];
        last = s.process2(xx, last); // two-operand function/operator
        break;
      }
      case 1: {
        last = s.process1(last); // one-operand function/operator
        break;
      }
      case 3:
      {
        double yy = exprstack[--p];  // three-operand
        double xx = exprstack[--p];
        last = s.process3(xx,yy,last);
        break;
      }
      default: // function with zero parameters, none
      {
        exprstack[p++] = last;
        last = s.process0(); 
        break; 
      }
      }
    }
  }

  return last;
}

double Context::rec_compute_old()
{
  const Symbol &s = pSymbols[--nPos];

  switch (s.type)
  {
  case Symbol::NUMBER: return s.dValue;
  case Symbol::VARIABLE:
    switch (s.vartype) {
    case Symbol::VARIABLE_X: return x;
    case Symbol::VARIABLE_Y: return y;
    case Symbol::VARIABLE_Z: return z;
    case Symbol::VARIABLE_A: return a;
    case Symbol::VARIABLE_BITDEPTH: return bitdepth; // bit-depth for autoscale
    case Symbol::VARIABLE_SCRIPT_BITDEPTH: return (double)sbitdepth; // source bit depth for autoscale

    case Symbol::VARIABLE_RANGE_HALF: return bitdepth == 32 ? 0.5 : (128 << (bitdepth - 8)); // or 0.0 for float in the future?
    case Symbol::VARIABLE_RANGE_MIN: return bitdepth == 32 ? 0.0 : 0; // max_pixel_value. 255, 1023, 4095, 16383, 65535 (1.0 for float)
    case Symbol::VARIABLE_RANGE_MAX: return bitdepth == 32 ? 1.0 : ((1 << bitdepth) - 1); // max_pixel_value. 255, 1023, 4095, 16383, 65535 (1.0 for float)
    case Symbol::VARIABLE_YRANGE_HALF: return bitdepth == 32 ? 0.5 : (128 << (bitdepth - 8));
    case Symbol::VARIABLE_YRANGE_MIN: return bitdepth == 32 ? 0.0 : 0;
    case Symbol::VARIABLE_YRANGE_MAX: return bitdepth == 32 ? 1.0 : ((1 << bitdepth) - 1);
    case Symbol::VARIABLE_RANGE_SIZE: return bitdepth == 32 ? 1.0 : (1 << bitdepth); // 256, 1024, 4096, 16384, 65536 (1.0 for float)
    case Symbol::VARIABLE_YMIN: return bitdepth == 32 ? 16.0 / 255 : (16 << (bitdepth - 8));    // 16 scaled
    case Symbol::VARIABLE_YMAX: return bitdepth == 32 ? 235.0 / 255 : (235 << (bitdepth - 8));  // 235 scaled
    case Symbol::VARIABLE_CMIN: return bitdepth == 32 ? 16.0 / 255 : (16 << (bitdepth - 8));    // 16 scaled
    case Symbol::VARIABLE_CMAX: return bitdepth == 32 ? 240.0 / 255 : (240 << (bitdepth - 8));  // 240 scaled
    case Symbol::VARIABLE_YFMIN: return bitdepth == 32 ? 16.0 / 255 : bitdepth == 10 ? 64 : bitdepth == 12 ? 257 : bitdepth == 14 ? 1028 : bitdepth == 16 ? 4112 : 16;
    case Symbol::VARIABLE_YFMAX: return bitdepth == 32 ? 235.0 / 255 : bitdepth == 10 ? 1175 : bitdepth == 12 ? 3995 : bitdepth == 14 ? 15275 : bitdepth == 16 ? 60395 : 235;
    case Symbol::VARIABLE_CFMIN: return bitdepth == 32 ? 16.0 / 255 : bitdepth == 10 ? 64 : bitdepth == 12 ? 257 : bitdepth == 14 ? 1028 : bitdepth == 16 ? 4112 : 16;
    case Symbol::VARIABLE_CFMAX: return bitdepth == 32 ? 240.0 / 255 : bitdepth == 10 ? 1200 : bitdepth == 12 ? 4080 : bitdepth == 14 ? 15600 : bitdepth == 16 ? 61680 : 240;
    default: 
      assert(0);
      return 0;
    }
    break;
  case Symbol::FUNCTION_WITH_BITDEPTH_AS_AUTOPARAM: // silent bit-depth parameter for autoscale
    return s.process3(rec_compute_old(), bitdepth, sbitdepth); // automatic bit-depth parameters

  default:
    switch (s.nParameter)
    {
    case 2:
    {
      double yy = rec_compute_old();
      double xx = rec_compute_old();
      return s.process2(xx, yy);
    }
    case 1: return s.process1(rec_compute_old());
    case 3:
    {
      double zz = rec_compute_old();
      double yy = rec_compute_old();
      double xx = rec_compute_old();
      return s.process3(xx, yy, zz);
    }
    default: return s.process0();
    }
  }
}

/*
double Context::compute(double _x, double _y, double _z, double _a, int _bitdepth, bool _chroma)
{
   nPos = nSymbols;
   this->x = _x;
   this->y = _y;
   this->z = _z;
   this->a = _a;

   this->luma_chroma = _chroma ? 1 : 0;
   this->bitdepth = _bitdepth;
   // all other expr constants are calculated from bitdepth

   return rec_compute(); // check x86 rec_compute_old with x,y,z,a is faster
}
*/
double Context::compute_4(double _x, double _y, double _z, double _a, int _bitdepth, bool _chroma)
{
  nPos = nSymbols;
  this->x = _x;
  this->y = _y;
  this->z = _z;
  this->a = _a;

  this->luma_chroma = _chroma ? 1 : 0;
  this->bitdepth = _bitdepth;
  // all other expr constants are calculated from bitdepth
  
  return rec_compute(); // on x86 rec_compute_old is MUCH faster
}

double Context::compute_3(double _x, double _y, double _z, int _bitdepth, bool _chroma)
{
  nPos = nSymbols;
  this->x = _x;
  this->y = _y;
  this->z = _z;

  this->luma_chroma = _chroma ? 1 : 0;
  this->bitdepth = _bitdepth;
  // all other expr constants are calculated from bitdepth

  return rec_compute();
}

double Context::compute_2(double _x, double _y, int _bitdepth, bool _chroma)
{
  nPos = nSymbols;
  this->x = _x;
  this->y = _y;

  this->luma_chroma = _chroma ? 1 : 0;
  this->bitdepth = _bitdepth;
  // all other expr constants are calculated from bitdepth

  return rec_compute();
}

double Context::compute_1(double _x, int _bitdepth, bool _chroma)
{
  nPos = nSymbols;
  this->x = _x;

  this->luma_chroma = _chroma ? 1 : 0;
  this->bitdepth = _bitdepth;
  // all other expr constants are calculated from bitdepth

  return rec_compute();
}

String Context::rec_infix()
{
    const Symbol &s = pSymbols[--nPos];

    switch ( s.type )
    {
    case Symbol::VARIABLE:
      switch (s.vartype) {
      case Symbol::VARIABLE_X:
      case Symbol::VARIABLE_Y:
      case Symbol::VARIABLE_Z:
      case Symbol::VARIABLE_A:
      case Symbol::VARIABLE_BITDEPTH:
      case Symbol::VARIABLE_SCRIPT_BITDEPTH:
      case Symbol::VARIABLE_RANGE_HALF:
      case Symbol::VARIABLE_RANGE_MIN:
      case Symbol::VARIABLE_RANGE_MAX:
      case Symbol::VARIABLE_YRANGE_HALF:
      case Symbol::VARIABLE_YRANGE_MIN:
      case Symbol::VARIABLE_YRANGE_MAX:
      case Symbol::VARIABLE_RANGE_SIZE:
      case Symbol::VARIABLE_YMIN:
      case Symbol::VARIABLE_YMAX:
      case Symbol::VARIABLE_CMIN:
      case Symbol::VARIABLE_CMAX:
      case Symbol::VARIABLE_YFMIN:
      case Symbol::VARIABLE_YFMAX:
      case Symbol::VARIABLE_CFMIN:
      case Symbol::VARIABLE_CFMAX:
        return s.value;
      }
    case Symbol::NUMBER: return s.value;
    case Symbol::FUNCTION:
        if (s.nParameter == 1) {
            return s.value + "(" + rec_infix() + ")";
        } else if (s.nParameter == 2) {
            auto op2 = rec_infix();
            return s.value + "(" + rec_infix() + "," + op2 + ")";
        } else {
            auto op3 = rec_infix();
            auto op2 = rec_infix();
            return s.value + "(" + rec_infix() + "," + op2 + "," + op3 + ")";
        }
    case Symbol::OPERATOR:
        {
            auto op2 = rec_infix();
            return "(" + rec_infix() + s.value + op2 + ")";
        }
    case Symbol::TERNARY:
        {
            auto op3 = rec_infix();
            auto op2 = rec_infix();
            return "((" + rec_infix() + ") ? " + op2 + " : " + op3 + ")";
        }
    case Symbol::FUNCTION_WITH_BITDEPTH_AS_AUTOPARAM:
    {
      return s.value + "(" + rec_infix() + ")";
    }

    case Symbol::DUP:
    {
      // really a stack operation, try to fake convert as function to infix
      return s.value + "(" + rec_infix() + ")";
    }

    case Symbol::SWAP:
    {
      // really a stack operation, try to fake convert as function to infix
      return s.value + "(" + rec_infix() + ")";
    }

    default:
        assert(0);
        return "";
    }
}

String Context::infix()
{
   nPos = nSymbols;

   return rec_infix();
}

bool Context::check()
{
   return true;
}