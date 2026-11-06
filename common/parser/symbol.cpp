#include "symbol.h"
#include <math.h>
#include <sstream>
#include <string>
#include <stdexcept>

using namespace Filtering;
using namespace Filtering::Parser;

// basic math
static double addition        (double x, double y) { return x + y; }
static double multiplication  (double x, double y) { return x * y; }
static double division        (double x, double y) { return x / y; }
static double substraction    (double x, double y) { return x - y; }
static double power           (double x, double y) { return pow(x, y); }
static double modulo          (double x, double y) { return fmod(x, y); }
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
static double _not             (double x) { return x > 0 ? -1 : 1; }
// Unsigned Bit arithmetic
static double andUB           (double x, double y) { return double(clip<uint64_t, double>(x) & clip<uint64_t, double>(y)); }
static double orUB            (double x, double y) { return double(clip<uint64_t, double>(x) | clip<uint64_t, double>(y)); }
static double xorUB           (double x, double y) { return double(clip<uint64_t, double>(x) ^ clip<uint64_t, double>(y)); }
static double negateUB        (double x) { return double(~clip<uint64_t, double>(x)); }
static double posshiftUB      (double x, double y) { return y >= 0 ? double(clip<uint64_t, double>(x) << clip<int64_t, double>(y)) : double(clip<uint64_t, double>(x) >> clip<int64_t, double>(-y)); }
static double negshiftUB      (double x, double y) { return y >= 0 ? double(clip<uint64_t, double>(x) >> clip<int64_t, double>(y)) : double(clip<uint64_t, double>(x) << clip<int64_t, double>(-y)); }
// Signed Bit arithmetic
static double andSB           (double x, double y) { return double(clip<int64_t, double>(x) & clip<int64_t, double>(y)); }
static double orSB            (double x, double y) { return double(clip<int64_t, double>(x) | clip<int64_t, double>(y)); }
static double xorSB           (double x, double y) { return double(clip<int64_t, double>(x) ^ clip<int64_t, double>(y)); }
static double negateSB        (double x) { return double(~clip<int64_t, double>(x)); }
static double posshiftSB      (double x, double y) { return y >= 0 ? double(clip<int64_t, double>(x) << clip<int64_t, double>(y)) : double(clip<int64_t, double>(x) >> clip<int64_t, double>(-y)); }
static double negshiftSB      (double x, double y) { return y >= 0 ? double(clip<int64_t, double>(x) >> clip<int64_t, double>(y)) : double(clip<int64_t, double>(x) << clip<int64_t, double>(-y)); }
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
static double mtatan2           (double y, double x) { return atan2(y, x); }
static double mtround           (double x) { return double(convert<int64_t, double>( x )); }
static double mtclip            (double x, double y, double z) { return clip<double, double>( x, y, z ); }
static double mtmin             (double x, double y) { return min<double>( x, y ); }
static double mtmax             (double x, double y) { return max<double>( x, y ); }
static double mtfloor           (double x) { return floor(x); }
static double mtceil            (double x) { return ceil(x); }
static double mttrunc           (double x) { return double(int64_t(x)); }
static double mtsqrt            (double x) { return x > 0 ? sqrt(x) : 0; } // negative: 0, as the Expr JIT
static double mtneg             (double x) { return -x; }
static double mtsgn             (double x) { return x < 0 ? -1 : x > 0 ? 1 : 0; }

// bit depth conversion helpers. x:value on 8 bit scale y: target bit depth 8-32 z:base bit depth
// Range and offset constants for any bit depth and full / limited mix: get_bits_conv_constants of AviSynth+
// (convert_helper.h), the routine of ConvertBits and of scaleb, scalef in Expr. Keep them the same.
// A level conversion: chroma is scaled around its center, limited range luma between 16 and 235.
namespace {

// The limited range convention of 32 bit float, see limited_range_float.h in AviSynth+
constexpr float limited_float_luma_span = 219 / 255.0f;   // 16..235
constexpr float limited_float_luma_offset = 16.0f / 255;  // float value of 16
constexpr float limited_float_chroma_span = 112 / 255.0f; // +/-112

struct bits_conv_constants {
  float src_offset;
  float mul_factor;
  float dst_offset;
};

bits_conv_constants get_bits_conv_constants(bool use_chroma, bool fulls, bool fulld, int srcBitDepth, int dstBitDepth)
{
  bits_conv_constants d;
  float src_span, dst_span;
  if (use_chroma) {
    // full range chroma span is intentionally 255/2.0, 127.5 at 8 bit
    src_span = (srcBitDepth == 32) ?
      (fulls ? 0.5f : limited_float_chroma_span) :
      (fulls ? (float)((1 << srcBitDepth) - 1) / 2.0f : (float)(112 << (srcBitDepth - 8)));
    dst_span = (dstBitDepth == 32) ?
      (fulld ? 0.5f : limited_float_chroma_span) :
      (fulld ? (float)((1 << dstBitDepth) - 1) / 2.0f : (float)(112 << (dstBitDepth - 8)));
    d.src_offset = (srcBitDepth == 32) ? 0.0f : (float)(1 << (srcBitDepth - 1));
    d.dst_offset = (dstBitDepth == 32) ? 0.0f : (float)(1 << (dstBitDepth - 1));
  }
  else {
    src_span = (srcBitDepth == 32) ?
      (fulls ? 1.0f : limited_float_luma_span) :
      (float)(fulls ? ((1 << srcBitDepth) - 1) : (219 << (srcBitDepth - 8)));
    dst_span = (dstBitDepth == 32) ?
      (fulld ? 1.0f : limited_float_luma_span) :
      (float)(fulld ? ((1 << dstBitDepth) - 1) : (219 << (dstBitDepth - 8)));
    d.src_offset = (srcBitDepth == 32) ?
      (fulls ? 0.0f : limited_float_luma_offset) :
      (fulls ? 0.0f : (float)(16 << (srcBitDepth - 8)));
    d.dst_offset = (dstBitDepth == 32) ?
      (fulld ? 0.0f : limited_float_luma_offset) :
      (fulld ? 0.0f : (float)(16 << (dstBitDepth - 8)));
  }
  d.mul_factor = dst_span / src_span;
  return d;
}

double scale_level(double x, int target_bit_depth, int source_bit_depth, bool chroma, bool shift_float, bool full)
{
  // no shortcut for the same depth: Expr goes through the steps there too, they can round
  bits_conv_constants d = get_bits_conv_constants(chroma, full, full, source_bit_depth, target_bit_depth);
  if (chroma && shift_float && target_bit_depth == 32 && source_bit_depth < 32) // "floatUV": from -0.5..0.5 to 0..1
    d.dst_offset = 0.5f;
  // in 32 bit float, step by step, as Expr: the stored float result is then the same to the last bit
  float result = (float)x;
  result -= d.src_offset;
  result *= d.mul_factor;
  result += d.dst_offset;
  return result;
}

} // namespace

static double do_upscaleByShift(double x, int y, int z, bool chroma, bool shift_float)
{
  return scale_level(x, y, z, chroma, shift_float, false);
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

static double do_upscaleByStretch(double x, int y, int z, bool chroma, bool shift_float)
{
  return scale_level(x, y, z, chroma, shift_float, true);
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
Symbol Symbol::Power          ("^" , "pow", OPERATOR, power);
Symbol Symbol::Modulo         ("%" , OPERATOR, modulo);
Symbol Symbol::Interrogation  ("?" , TERNARY , interrogation);
Symbol Symbol::Equal          ("==", OPERATOR, equal);
Symbol Symbol::Equal2         ("=", OPERATOR, equal);
Symbol Symbol::NotEqual       ("!=", OPERATOR, notEqual);
Symbol Symbol::Inferior       ("<=", OPERATOR, inferior);
Symbol Symbol::InferiorStrict ("<" , OPERATOR, inferiorStrict);
Symbol Symbol::Superior       (">=", OPERATOR, superior);
Symbol Symbol::SuperiorStrict (">" , OPERATOR, superiorStrict);
Symbol Symbol::And            ("&" , "and", OPERATOR, _and);
Symbol Symbol::Or             ("|" , "or", OPERATOR, _or);
Symbol Symbol::AndNot         ("&!", OPERATOR, andNot);
Symbol Symbol::Xor            ("\xB0" , "@", OPERATOR, _xor); // degree-sign 0xB0, has codepage problems, kept only for compatibility
Symbol Symbol::XorName        ("\xB0" , "xor", OPERATOR, _xor);
Symbol Symbol::Not            ("not", FUNCTION, _not);
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
Symbol Symbol::Width          ("width", VARIABLE, VARIABLE_WIDTH);
Symbol Symbol::Height         ("height", VARIABLE, VARIABLE_HEIGHT);
Symbol Symbol::SpatialX       ("sx", VARIABLE, VARIABLE_SX);
Symbol Symbol::SpatialY       ("sy", VARIABLE, VARIABLE_SY);
Symbol Symbol::SpatialXRelative("sxr", VARIABLE, VARIABLE_SXR);
Symbol Symbol::SpatialYRelative("syr", VARIABLE, VARIABLE_SYR);
Symbol Symbol::FrameNo        ("frameno", VARIABLE, VARIABLE_FRAMENO);
Symbol Symbol::Time           ("time", VARIABLE, VARIABLE_TIME);
// Math
Symbol Symbol::Cos            ("cos", FUNCTION, mtcos);
Symbol Symbol::Sin            ("sin", FUNCTION, mtsin);
Symbol Symbol::Tan            ("tan", FUNCTION, mttan);
Symbol Symbol::Log            ("log", FUNCTION, mtlog);
Symbol Symbol::Exp            ("exp", FUNCTION, mtexp);
Symbol Symbol::Abs            ("abs", FUNCTION, mtmabs);
Symbol Symbol::Atan           ("atan", FUNCTION, mtatan);
Symbol Symbol::Atan2          ("atan2", FUNCTION, mtatan2);
Symbol Symbol::Acos           ("acos", FUNCTION, mtacos);
Symbol Symbol::Asin           ("asin", FUNCTION, mtasin);
Symbol Symbol::Round          ("round", FUNCTION, mtround);
Symbol Symbol::Clip           ("clip", FUNCTION, mtclip);
Symbol Symbol::Min            ("min", FUNCTION, mtmin);
Symbol Symbol::Max            ("max", FUNCTION, mtmax);
Symbol Symbol::Ceil           ("ceil", FUNCTION, mtceil);
Symbol Symbol::Floor          ("floor", FUNCTION, mtfloor);
Symbol Symbol::Trunc          ("trunc", FUNCTION, mttrunc);
Symbol Symbol::Sqrt           ("sqrt", FUNCTION, mtsqrt);
Symbol Symbol::Neg            ("neg", FUNCTION, mtneg);
Symbol Symbol::Sgn            ("sgn", FUNCTION, mtsgn);
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

static bool local_independent_convert(const std::string& text, double &result)
{
  if (text == "")
    return false;
  std::istringstream iss(text);
  iss.imbue(std::locale::classic());
  iss >> result;
  if (iss.eof())
    return true;
  else
    return false;
}
// direct numeric literals from parser
Symbol::Symbol(String value, Type type, bool InvalidSymbolIsZero) :
  type(type), vartype(VARIABLE_UNDEFINED), value(value), value2(""), nParameter(0), process0(NULL), process1(NULL), process2(NULL), process3(NULL)
{
  if (type == NUMBER) {
    // instead of atof(value.c_str()); not good, no error report
    //errno = 0; // thread-safe error code
    //dValue = std::strtod(value.c_str(), &nexts); not good, local dependepent
    bool success = local_independent_convert(value, /*ref*/dValue);
    if (!InvalidSymbolIsZero && !success)
    {
      // fail
      this->type = UNDEFINED; // signal to parser about the error
    }
  }
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
  type(type), vartype(vartype), value(value), value2(""), nParameter(0), dValue(0.0)
{
}

// predefined numeric constants, like "Pi"
// Type=NUMBER always
Symbol::Symbol(String value, double dValue, Type type, int nParameter, Process1 process) :
type(type), vartype(VARIABLE_UNDEFINED), value(value), value2(""), nParameter(nParameter), dValue(dValue), process1(process)
{
}

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


int Filtering::Parser::expression_dependencies(const std::deque<Symbol> &expression)
{
  int dependencies = 0;
  for (const Symbol &s : expression) {
    if (s.type != Symbol::VARIABLE)
      continue;
    switch (s.vartype) {
    case Symbol::VARIABLE_SX: case Symbol::VARIABLE_SY:
    case Symbol::VARIABLE_SXR: case Symbol::VARIABLE_SYR:
      dependencies |= DEPENDS_ON_POSITION;
      break;
    case Symbol::VARIABLE_WIDTH: case Symbol::VARIABLE_HEIGHT:
      dependencies |= DEPENDS_ON_SIZE;
      break;
    case Symbol::VARIABLE_FRAMENO: case Symbol::VARIABLE_TIME:
      dependencies |= DEPENDS_ON_FRAME;
      break;
    default:
      break;
    }
  }
  return dependencies;
}

void Context::set_plane(int width, int height, int frame_number, int frame_count)
{
  plane_width = width;
  plane_height = height;
  // as Expr: 0..1 inclusive, 0 on a single column or row
  sxr_divisor = width > 1 ? width - 1 : 1;
  syr_divisor = height > 1 ? height - 1 : 1;
  frameno = frame_number;
  reltime = frame_count > 1 ? (double)frame_number / (frame_count - 1) : 0.0;
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
   position_dependent = (expression_dependencies(expression) & DEPENDS_ON_POSITION) != 0;
   pSymbols.reserve(expression.size());
   exprstack.reserve(expression.size());

   // new v2.2.15 scaling
   fullrange_autoscale = false;
   scale_int = false;
   scale_float = false;
   shift_float = false;

   sbitdepth = 8; // default source bit depth for expressions, used in scale_inputs and scaleb/scalef/yscalef,yscaleb
   // scaleb/scalef converts constants from this bit depth to the current one
   // when scale_inputs == true, this is the target bit depth of the internal scaling

   //int default_float_autoscale_bitdepth = 0; // no clamp and autoscale
   //int default_all_autoscale_bitdepth = 0; // general autoscale v2.2.15-
   //bool default_fullrange_autoscale = false;

   // separate real and control symbols
   for (auto &it: expression) {
     // control mnemonic (we have only one kind) is filtered here, they are not put in the expression
     if (it.type == Symbol::FUNCTION_CONFIG_SCRIPT_BITDEPTH) {
       if (it.dValue > 0) {
         // i8, i10, i12, i14, i16, f32
         sbitdepth = (int)it.dValue;
       }
       else {
         // negative values show the bitdepth that inputs and output should autoscale.
         // clamp_f
         // clamp_f_i8..clamp_f_i16..clamp_f_f32
         //default_float_autoscale_bitdepth = -(int)it->dValue;
         scale_int = false;
         scale_float = true;
         shift_float = false;
         sbitdepth = -(int)it.dValue;
       }
     }
     else {
       pSymbols.push_back(it);
     }
   }

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
}

double Context::rec_compute()
{
  compute_error = compute_error_t::CE_NONE;

  // return 0 on empty expression (not error!)
  if (pSymbols.size() == 0)
    return 0.0;

  double last = 0;

  Symbol &s_first = pSymbols[0];

  // we have one 'last' and intermediade stack
  // in the end stack must be of zero size again
  // and a result in 'last'
  exprstack.clear();

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
    case Symbol::VARIABLE_WIDTH: { last = plane_width; break; }
    case Symbol::VARIABLE_HEIGHT: { last = plane_height; break; }
    case Symbol::VARIABLE_SX: { last = sx; break; }
    case Symbol::VARIABLE_SY: { last = sy; break; }
    case Symbol::VARIABLE_SXR: { last = sx / sxr_divisor; break; }
    case Symbol::VARIABLE_SYR: { last = sy / syr_divisor; break; }
    case Symbol::VARIABLE_FRAMENO: { last = frameno; break; }
    case Symbol::VARIABLE_TIME: { last = reltime; break; }
    default: {
      compute_error = compute_error_t::CE_INVALID_INTERNAL_VARIABLE; // this is internal error, cannot happen
      assert(0);
    }
    }
    break;
  }
  default: {
    compute_error = compute_error_t::CE_INVALID_FIRST_TAG;
    return 0; // assert
  }
  }

  for (auto i = 1; i < (int)pSymbols.size(); i++) {
    Symbol &s = pSymbols[i];

    switch (s.type)
    {
    case Symbol::NUMBER: { 
      exprstack.push_back(last);
      last=s.dValue;
      break; 
    }
    case Symbol::VARIABLE: {
      exprstack.push_back(last);
      switch (s.vartype) {
      case Symbol::VARIABLE_X: { last = x; break; }
      case Symbol::VARIABLE_Y: { last = y; break; }
      case Symbol::VARIABLE_Z: { last = z; break; }
      case Symbol::VARIABLE_A: { last = a; break; }
      case Symbol::VARIABLE_BITDEPTH: { last = bitdepth; break; } // bit-depth for autoscale
      case Symbol::VARIABLE_SCRIPT_BITDEPTH: { last = (double)sbitdepth; break; } // source bit depth for autoscale

      case Symbol::VARIABLE_RANGE_HALF: { last = bitdepth == 32 ? range_half_f[luma_chroma] : a_range_half[bitdepth - 8]; break; } // or 0.0 for float in the future?
      case Symbol::VARIABLE_RANGE_MIN: { last = bitdepth == 32 ? range_min_f[luma_chroma] : a_range_min[bitdepth - 8]; break; } // min_pixel_value. 0 or -0.5f
      case Symbol::VARIABLE_RANGE_MAX: { last = bitdepth == 32 ? range_max_f[luma_chroma] : a_range_max[bitdepth - 8]; break; } // max_pixel_value. 255, 1023, 4095, 16383, 65535 (0.5 for float)
      case Symbol::VARIABLE_YRANGE_HALF: { last = bitdepth == 32 ? range_half_f[0] : a_range_half[bitdepth - 8]; break; } // 0.5
      case Symbol::VARIABLE_YRANGE_MIN: { last = bitdepth == 32 ? range_min_f[0] : a_range_min[bitdepth - 8]; break; } // min_pixel_value. 0
      case Symbol::VARIABLE_YRANGE_MAX: { last = bitdepth == 32 ? range_max_f[0] : a_range_max[bitdepth - 8]; break; } // max_pixel_value. 255, 1023, 4095, 16383, 65535 (1.0 for float)
      case Symbol::VARIABLE_RANGE_SIZE: { last = bitdepth == 32 ? range_size_f : a_range_size[bitdepth - 8]; break; } // 256, 1024, 4096, 16384, 65536 (1.0 for float)
      case Symbol::VARIABLE_YMIN: { last = bitdepth == 32 ? ymin_f : a_ymin[bitdepth - 8]; break;  }   // 16 scaled
      case Symbol::VARIABLE_YMAX: { last = bitdepth == 32 ? ymax_f : a_ymax[bitdepth - 8]; break; } // 235 scaled
      case Symbol::VARIABLE_CMIN: { last = bitdepth == 32 ? cmin_f : a_cmin[bitdepth - 8]; break;  } // 16 scaled
      case Symbol::VARIABLE_CMAX: { last = bitdepth == 32 ? cmax_f : a_cmax[bitdepth - 8]; break;  } // 240 scaled
      case Symbol::VARIABLE_WIDTH: { last = plane_width; break; }
      case Symbol::VARIABLE_HEIGHT: { last = plane_height; break; }
      case Symbol::VARIABLE_SX: { last = sx; break; }
      case Symbol::VARIABLE_SY: { last = sy; break; }
      case Symbol::VARIABLE_SXR: { last = sx / sxr_divisor; break; }
      case Symbol::VARIABLE_SYR: { last = sy / syr_divisor; break; }
      case Symbol::VARIABLE_FRAMENO: { last = frameno; break; }
      case Symbol::VARIABLE_TIME: { last = reltime; break; }
      default: assert(0);
      }
      break;
    }

    case Symbol::DUP: {
      int distance = s.nParameter; // 1..9  duplicates the nth distance stack value to the top
      exprstack.push_back(last); // the current top stays below the copy
      if (distance != 0) {
        auto newptr = (int)exprstack.size() - 1 - distance;
        if (newptr < 0) { // out of stack addressing. FIXME: error?
          compute_error = compute_error_t::CE_DUP_INDEX;
          last = 0.0;
        }
        else
          last = exprstack[newptr];
      }
      break;
    }

    case Symbol::SWAP:
    {
      int distance = s.nParameter; // 1..9  exchanges stacktop with the nth distance stack value
      auto newptr = (int)exprstack.size() - distance;
      if (newptr < 0) { // out of stack addressing. FIXME: error?
        compute_error = compute_error_t::CE_SWAP_INDEX;
        last = 0.0;
      }
      else {
        std::swap(exprstack[newptr], last);
      }
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
        if (exprstack.size() != 0) {
          double xx = exprstack.back(); exprstack.pop_back();
          last = s.process2(xx, last); // two-operand function/operator
        }
        else
        {
          last = 0.0;
          compute_error = compute_error_t::CE_NOT_ENOUGH_OPERANDS;
        }
        break;
      }
      case 1: {
        last = s.process1(last); // one-operand function/operator
        break;
      }
      case 3:
      {
        if (exprstack.size() >= 2) {
          double yy = exprstack.back(); exprstack.pop_back(); // three-operand
          double xx = exprstack.back(); exprstack.pop_back();
          last = s.process3(xx, yy, last);
        }
        else
        {
          last = 0.0;
          compute_error = compute_error_t::CE_NOT_ENOUGH_OPERANDS;
        }
        break;
      }
      default: // function with zero parameters, none
      {
        exprstack.push_back(last);
        last = s.process0(); 
        break; 
      }
      }
    }
  }

#if 0
  if (compute_error != compute_error_t::CE_NONE) {
    // excessive member in expression! (e.g. "x x"
    int xx = 1; // FIXME DEBUG
  } 
  else if (exprstack.size() > 0) {
    // or unbalanced stack (e.g. "x +" instead of "x 2 +"
    compute_error = compute_error_t::CE_EXTRA_EXPRESSION_LEFT;
    int xx = 1; // FIXME DEBUG
  }
#endif

  return last;
}

double Context::compute_4(double _x, double _y, double _z, double _a, int _bitdepth, bool _chroma)
{
  this->x = _x;
  this->y = _y;
  this->z = _z;
  this->a = _a;

  this->luma_chroma = _chroma ? 1 : 0;
  this->bitdepth = _bitdepth;
  // all other expr constants are calculated from bitdepth
  
  return rec_compute();
}

double Context::compute_3(double _x, double _y, double _z, int _bitdepth, bool _chroma)
{
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
  this->x = _x;
  this->y = _y;

  this->luma_chroma = _chroma ? 1 : 0;
  this->bitdepth = _bitdepth;
  // all other expr constants are calculated from bitdepth

  return rec_compute();
}

double Context::compute_1(double _x, int _bitdepth, bool _chroma)
{
  this->x = _x;

  this->luma_chroma = _chroma ? 1 : 0;
  this->bitdepth = _bitdepth;
  // all other expr constants are calculated from bitdepth

  return rec_compute();
}

// The reverse polish expression run on a stack of infix strings; dup and swap copy and exchange them.
String Context::infix()
{
   std::vector<String> stack;
   auto pop = [&stack]() {
      String top = stack.back();
      stack.pop_back();
      return top;
   };

   for (const Symbol &s : pSymbols) {
      int operands;
      switch (s.type) {
      case Symbol::OPERATOR: operands = 2; break;
      case Symbol::TERNARY: operands = 3; break;
      case Symbol::FUNCTION: operands = s.nParameter; break;
      case Symbol::FUNCTION_WITH_BITDEPTH_AS_AUTOPARAM: operands = 1; break;
      case Symbol::DUP: case Symbol::SWAP: operands = s.nParameter + 1; break;
      default: operands = 0; break; // variables, constants, numbers
      }
      if ((int)stack.size() < operands)
         throw std::runtime_error("mt_infix: not enough operands for \"" + s.value + "\"");

      switch (s.type) {
      case Symbol::OPERATOR: {
         auto op2 = pop();
         auto op1 = pop();
         // "x&sy" reads back as x &s y: a space keeps the sx, sy constant after an operator with a signed form
         const bool has_signed_form = s.value == "&" || s.value == "|" || s.value == "\xB0" || s.value == "<<" || s.value == ">>";
         const bool needs_space = has_signed_form && (op2.compare(0, 2, "sx") == 0 || op2.compare(0, 2, "sy") == 0);
         stack.push_back("(" + op1 + s.value + (needs_space ? " " : "") + op2 + ")");
         break;
      }
      case Symbol::TERNARY: {
         auto op3 = pop();
         auto op2 = pop();
         auto op1 = pop();
         stack.push_back("((" + op1 + ") ? " + op2 + " : " + op3 + ")");
         break;
      }
      case Symbol::FUNCTION:
      case Symbol::FUNCTION_WITH_BITDEPTH_AS_AUTOPARAM: {
         String args;
         for (int i = 0; i < operands; i++)
            args = (i + 1 < operands ? "," : "") + pop() + args;
         stack.push_back(s.value + "(" + args + ")");
         break;
      }
      case Symbol::DUP:
         stack.push_back(stack[stack.size() - 1 - s.nParameter]);
         break;
      case Symbol::SWAP:
         std::swap(stack.back(), stack[stack.size() - 1 - s.nParameter]);
         break;
      default:
         stack.push_back(s.value);
         break;
      }
   }

   if (stack.empty())
      throw std::runtime_error("mt_infix: empty expression");
   if (stack.size() > 1)
      throw std::runtime_error("mt_infix: " + std::to_string(stack.size() - 1) + " value(s) left over");
   return stack.back();
}

bool Context::check()
{
   return true;
}

int Context::get_compute_error()
{
  return compute_error;
}
