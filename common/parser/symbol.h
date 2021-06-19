#ifndef __Mt_Symbol_H__
#define __Mt_Symbol_H__

#include "../utils/utils.h"
#include <deque>
#include <stack>


namespace Filtering { namespace Parser {

class Symbol {
public:
   typedef enum {
      NUMBER, // const
      VARIABLE,
      INPUT, // lowercase x,y,z,a : 0..25
      USER_VARIABLE, // uppercase 'A'..'Z' : 0..25
      USER_VARIABLE_STORE, // 'A@'..'Z@': 0..25
      USER_VARIABLE_STORE_AND_POP, // 'A^'..'Z^': 0..25
      FUNCTION_WITH_BITDEPTH_AS_AUTOPARAM,
      DUP,
      SWAP,
      FUNCTION_CONFIG_SCRIPT_BITDEPTH,
      FUNCTION_CONFIG_AUTO_BITDEPTH,
      FUNCTION_CONFIG_AUTO_BITDEPTH_FULL,
      OPERATOR,
      FUNCTION,
      TERNARY,
      UNDEFINED
   } Type;

   typedef enum {
     VARIABLE_X,
     VARIABLE_Y,
     VARIABLE_Z,
     // 4th variable since v2.2.1
     VARIABLE_A,

     VARIABLE_BITDEPTH, // automatic silent parameter of the expression v2.2.1
     VARIABLE_SCRIPT_BITDEPTH, // base bit depth of the values to scale v2.2.2, settable with i8..i16,f32 keywords

     // special adaptive constants filled by bitdepth
     VARIABLE_RANGE_HALF,
     VARIABLE_RANGE_MIN,
     VARIABLE_RANGE_MAX,
     VARIABLE_YRANGE_HALF,
     VARIABLE_YRANGE_MIN,
     VARIABLE_YRANGE_MAX,
     VARIABLE_RANGE_SIZE,
     VARIABLE_YMIN,
     VARIABLE_YMAX,
     VARIABLE_CMIN,
     VARIABLE_CMAX,
     VARIABLE_YFMIN,
     VARIABLE_YFMAX,
     VARIABLE_CFMIN,
     VARIABLE_CFMAX,

     VARIABLE_UNDEFINED,
   } VarType;


public:

   Type type;
   VarType vartype;
   String value;
   String value2;
   int nParameter;
   double dValue;
   int iValue; // user/internal variable index, input index, etc...
   typedef double (*Process0)();
   typedef double (*Process1)(double x);
   typedef double (*Process2)(double x, double y);
   typedef double (*Process3)(double x, double y, double z);
   typedef double (*ProcessScale)(double x, int y, int z, bool chroma, bool shift_float);
   Process0 process0;
   Process1 process1;
   Process2 process2;
   Process3 process3;
   ProcessScale processScale;

private:

private:
public:

   Symbol();
   // direct numeric literals from parser
   Symbol(String value, Type type);
   // dup, swap
   Symbol(String value, Type type, int nParameter);
   // functions, operators, ternary
   Symbol(String value, Type type, Process0 process);
   Symbol(String value, Type type, Process1 process);
   Symbol(String value, Type type, Process2 process);
   Symbol(String value, Type type, Process3 process);
   // same with secondary tokens
   Symbol(String value, String value2, Type type, Process1 process);
   Symbol(String value, String value2, Type type, Process2 process);
   Symbol(String value, String value2, Type type, Process3 process);
   Symbol(String value, Type type, ProcessScale process);
   Symbol(String value, String value2, Type type, ProcessScale process);
   // variables
   Symbol(String value, Type type, VarType vartype);
   // numbers
   Symbol(String value, double dValue, Type type, int nParameter, Process1 process);


   // void setValue(double dValue); dead code
   double getValue(double x, double y, double z) const;

   static Symbol Addition;
   static Symbol Multiplication;
   static Symbol Division;
   static Symbol Substraction;
   static Symbol Power;
   static Symbol Modulo;
   static Symbol Interrogation;
   static Symbol Equal;
   static Symbol Equal2;
   static Symbol NotEqual;
   static Symbol Inferior;
   static Symbol InferiorStrict;
   static Symbol Superior;
   static Symbol SuperiorStrict;
   static Symbol And;
   static Symbol Or;
   static Symbol AndNot;
   static Symbol Xor;
   static Symbol AndUB;
   static Symbol OrUB;
   static Symbol XorUB;
   static Symbol NegateUB;
   static Symbol PosShiftUB;
   static Symbol NegShiftUB;
   static Symbol AndSB;
   static Symbol OrSB;
   static Symbol XorSB;
   static Symbol NegateSB;
   static Symbol PosShiftSB;
   static Symbol NegShiftSB;
   static Symbol Pi;
   static Symbol X;
   static Symbol Y;
   static Symbol Z;
   // v2.2.1
   // 4th variable: a
   static Symbol A;
   // Auto bitdepth conv
   static Symbol BITDEPTH;
   static Symbol SCRIPT_BITDEPTH;
   // special adaptive constants filled by bitdepth
   static Symbol RANGE_HALF;
   static Symbol RANGE_MIN;
   static Symbol RANGE_MAX;
   static Symbol YRANGE_HALF;
   static Symbol YRANGE_MIN;
   static Symbol YRANGE_MAX;
   static Symbol RANGE_SIZE;
   static Symbol YMIN;
   static Symbol YMAX;
   static Symbol CMIN;
   static Symbol CMAX;
   static Symbol YFMIN;
   static Symbol YFMAX;
   static Symbol CFMIN;
   static Symbol CFMAX;

   static Symbol Cos;
   static Symbol Sin;
   static Symbol Tan;
   static Symbol Log;
   static Symbol Abs;
   static Symbol Exp;
   static Symbol Acos;
   static Symbol Atan;
   static Symbol Asin;
   static Symbol Round;
   static Symbol Clip;
   static Symbol Min;
   static Symbol Max;
   static Symbol Ceil;
   static Symbol Floor;
   static Symbol Trunc;
   // auto bit depth conversions
   static Symbol ScaleByShift;
   static Symbol ScaleByStretch;
   static Symbol ScaleByShiftY;
   static Symbol ScaleByStretchY;
   // script bit depth setters
   static Symbol SetScriptBitDepthI8;
   static Symbol SetScriptBitDepthI10;
   static Symbol SetScriptBitDepthI12;
   static Symbol SetScriptBitDepthI14;
   static Symbol SetScriptBitDepthI16;
   static Symbol SetScriptBitDepthF32;
   static Symbol SetFloatToClampUseI8Range;
   static Symbol SetFloatToClampUseI10Range;
   static Symbol SetFloatToClampUseI12Range;
   static Symbol SetFloatToClampUseI14Range;
   static Symbol SetFloatToClampUseI16Range;
   static Symbol SetFloatToClampUseF32Range;
   static Symbol SetFloatToClampUseF32Range_2;
   // v.2.2.5 extensions
   static Symbol Swap;
   static Symbol Dup;
   // v 2.2.25
   static Symbol Swap1;
   static Symbol Swap2;
   static Symbol Swap3;
   static Symbol Swap4;
   static Symbol Swap5;
   static Symbol Swap6;
   static Symbol Swap7;
   static Symbol Swap8;
   static Symbol Swap9;
   static Symbol Dup0;
   static Symbol Dup1;
   static Symbol Dup2;
   static Symbol Dup3;
   static Symbol Dup4;
   static Symbol Dup5;
   static Symbol Dup6;
   static Symbol Dup7;
   static Symbol Dup8;
   static Symbol Dup9;
};

class Context {

   Symbol *pSymbols;
   int nSymbols;
   int nPos;

   Symbol *pSymbols_control;
   int nSymbols_control;

   double x, y, z, a;
   int bitdepth; // bit depth
   int luma_chroma; // 0: luma(non-chroma) 1: chroma

   int sbitdepth; // source bit depth of values to scale

   // predefined bit-depth dependent constant 8..16 -> 0..8
   // calculated in advance for realtime speed gain
   double a_range_half[9];
   double a_range_min[9];
   double a_range_max[9];
   double a_range_size[9];
   double a_ymin[9];
   double a_ymax[9];
   double a_cmin[9];
   double a_cmax[9];
   double range_half_f[2]; // luma/chroma
   double range_min_f[2];  // luma/chroma
   double range_max_f[2];  // luma/chroma
   double range_size_f;
   double ymin_f;
   double ymax_f;
   double cmin_f; // limited range
   double cmax_f;

   double *exprstack;

   // helpers for float input autoscales
   // 0: none
   // 8, 10, 12, 14, 16: scale input 0..1 float to this range
   // 32: no scaling needed but 0..1 clamping is still done before returning the result
   int float_autoscale_bitdepth;
   float float_input_scalefactor;
   float float_input_invscalefactor;

   // helpers for any input autoscales
   // 0: none
   // 8, 10, 12, 14, 16: scale input this range before Expr, convert back after expr
   // int all_autoscale_bitdepth; this is sbitdepth
   // Since U and V range in 32 bit float mode is -0.5 .. 0.5, "floatUV" mode is pre-shifting the input to the 0.5 centered range before applying the expression
   // then the result is shifted back to the original -0.5..0.5 range
   bool fullrange_autoscale; // when autoscaling, conversion is limited or full-range-style
   bool scale_int;
   bool scale_float;
   bool shift_float; // special internal flag since v2.2.20: set when scale_inputs is "floatUV"
   int clamp_float_i;
   float chroma_center_i;
   float chroma_center_f;
   float chroma_lo_f;
   float chroma_hi_f;
   double sbitdepth_f; // source bit depth of values to scale, avoid conversions

   void calc_helpers();

   double rec_compute();
   double rec_compute_old();
   String rec_infix();

public:
   
   Context(const std::deque<Symbol> &expression);
   Context(const std::deque<Symbol> &expression, String scale_inputs, int param_clamp_float);

   ~Context();

   bool SetScaleInputs(String scale_inputs); // v2.2.15-

   bool check();
   String infix();

   double compute_1(double x, int bitdepth, bool chroma);
   double compute_2(double x, double y, int bitdepth, bool chroma);
   double compute_3(double x, double y, double z, int bitdepth, bool chroma);
   double compute_4(double x, double y, double z, double a, int bitdepth, bool chroma);
   // v2.2.1: variable a
   //double compute(double x, double y = -1.0, double z = -1.0, double a = -1.0, int bitdepth, bool chroma);
   
   MT_FORCEINLINE Byte compute_byte_x(int _x) { 
     if (!scale_int || sbitdepth == 8)
       return clip<Byte, double>(compute_1(_x, 8, false));
     // 1.) convert 8 bit data to all_autoscale_bitdepth 2.) Compute 3.) Convert Back to 8 bits
     if (!fullrange_autoscale) { // limited
       const int bitdiff = (sbitdepth - 8);
       return clip<Byte, double>(compute_1(_x << bitdiff, sbitdepth, false) / (1 << bitdiff));
     }
     else {
       const double factor = ((1 << sbitdepth) - 1) / 255.0;
       return clip<Byte, double>(compute_1(_x * factor, sbitdepth, false) / factor);
     }
   } // byte: default 8 bit, chroma=false: n/a for 8 bits

   MT_FORCEINLINE Byte compute_byte_xy(int _x, int _y) { 
     if (!scale_int || sbitdepth == 8)
       return clip<Byte, double>(compute_2(_x, _y, 8, false));
     // 1.) convert 8 bit data to all_autoscale_bitdepth 2.) Compute 3.) Convert Back to 8 bits
     if (!fullrange_autoscale) { // limited
       const int bitdiff = (sbitdepth - 8);
       return clip<Byte, double>(compute_2(_x << bitdiff, _y << bitdiff, sbitdepth, false) / (1 << bitdiff));
     }
     else {
       const double factor = ((1 << sbitdepth) - 1) / 255.0;
       return clip<Byte, double>(compute_2(_x * factor, _y * factor, sbitdepth, false) / factor);
     }
   } // byte: default 8 bit, chroma=false: n/a for 8 bits

   MT_FORCEINLINE Byte compute_byte_xyz(int _x, int _y, int _z) { 
     if (!scale_int || sbitdepth == 8)
       return clip<Byte, double>(compute_3(_x, _y, _z, 8, false));
     // 1.) convert 8 bit data to all_autoscale_bitdepth 2.) Compute 3.) Convert Back to 8 bits
     if (!fullrange_autoscale) { // limited
       const int bitdiff = (sbitdepth - 8);
       return clip<Byte, double>(compute_3(_x << bitdiff, _y << bitdiff, _z << bitdiff, sbitdepth, false) / (1 << bitdiff));
     }
     else {
       const double factor = ((1 << sbitdepth) - 1) / 255.0;
       return clip<Byte, double>(compute_3(_x * factor, _y * factor, _z * factor, sbitdepth, false) / factor);
     }
   } // byte: default 8 bit, chroma=false: n/a for 8 bits

   MT_FORCEINLINE Byte compute_byte_xyza(int _x, int _y, int _z, int _a) { 
     if (!scale_int || sbitdepth == 8)
       return clip<Byte, double>( compute_4(_x, _y, _z, _a, 8, false) );
     // 1.) convert 8 bit data to all_autoscale_bitdepth 2.) Compute 3.) Convert Back to 8 bits
     if (!fullrange_autoscale) { // limited
       const int bitdiff = (sbitdepth - 8);
       return clip<Byte, double>(compute_4(_x << bitdiff, _y << bitdiff, _z << bitdiff, _a << bitdiff, sbitdepth, false) / (1 << bitdiff));
     }
     else {
       const double factor = ((1 << sbitdepth) - 1) / 255.0;
       return clip<Byte, double>(compute_4(_x * factor, _y * factor, _z * factor, _a * factor, sbitdepth, false) / factor);
     }
   } // byte: default 8 bit, chroma=false: n/a for 8 bits
   
   //used from lutspa relative
   Byte compute_byte_xy_dblinput(double _x, double _y) { return clip<Byte, double>(compute_2(_x, _y, 8, false)); } // byte: default 8 bit, chroma=false: n/a for 8 bits

   template<int bits_per_pixel>
   MT_FORCEINLINE Word compute_word_x(int _x) {
     if (!scale_int || sbitdepth == bits_per_pixel) {
       if (bits_per_pixel == 16) // no min/max, faster
         return clip<Word, double>(compute_1(_x, bits_per_pixel, false)); // chroma = false: n/a for 8 bits
       else
         return min(clip<Word, double>(compute_1(_x, bits_per_pixel, false)), (Word)((1 << bits_per_pixel) - 1));
     }
     // 1.) convert 10-16 bits_per_pixel bit data to all_autoscale_bitdepth 2.) Compute 3.) Convert Back to bits_per_pixel bits
     if (!fullrange_autoscale) { // limited
       const int bitdiff = (sbitdepth - bits_per_pixel); // plus or minus
       if (bitdiff > 0) {
         // shift to bigger
         if (bits_per_pixel == 16)
           return clip<Word, double>(compute_1(_x << bitdiff, sbitdepth, false) / (1 << bitdiff)); // chroma = false: n/a for 10-16 bits
         else
           return min(clip<Word, double>(compute_1(_x << bitdiff, sbitdepth, false) / (1 << bitdiff)), (Word)((1 << bits_per_pixel) - 1));
       }
       else {
         // shift to smaller. Do not shift as int, precision would be lost
         const double factor = (double)(1 << -bitdiff);
         if (bits_per_pixel == 16)
           return clip<Word, double>(compute_1(_x / factor, sbitdepth, false) * factor); // chroma = false: n/a for 10-16 bits
         else
           return min(clip<Word, double>(compute_1(_x / factor, sbitdepth, false) * factor), (Word)((1 << bits_per_pixel) - 1));
       }
     }
     else {
       const double factor = (double)((1 << sbitdepth) - 1) / ((1 << bits_per_pixel) - 1);
       if (bits_per_pixel == 16)
         return clip<Word, double>(compute_1(_x * factor, sbitdepth, false) / factor); // chroma = false: n/a for 10-16 bits
       else
         return min(clip<Word, double>(compute_1(_x * factor, sbitdepth, false) / factor), (Word)((1 << bits_per_pixel) - 1));
     }
   }

   template<int bits_per_pixel>
   MT_FORCEINLINE Word compute_word_xy(int _x, int _y) {
     if (!scale_int || sbitdepth == bits_per_pixel) {
       if (bits_per_pixel == 16) // no min/max, faster
         return clip<Word, double>(compute_2(_x, _y, bits_per_pixel, false));
       else
         return min(clip<Word, double>(compute_2(_x, _y, bits_per_pixel, false)), (Word)((1 << bits_per_pixel) - 1));
     }
     // 1.) convert 10-16 bits_per_pixel bit data to all_autoscale_bitdepth 2.) Compute 3.) Convert Back to bits_per_pixel bits
     if (!fullrange_autoscale) { // limited
       const int bitdiff = (sbitdepth - bits_per_pixel); // plus or minus
       if (bitdiff > 0) {
         // shift to bigger
         if (bits_per_pixel == 16)
           return clip<Word, double>(compute_2(_x << bitdiff, _y << bitdiff, sbitdepth, false) / (1 << bitdiff)); // chroma = false: n/a for 10-16 bits
         else
           return min(clip<Word, double>(compute_2(_x << bitdiff, _y << bitdiff, sbitdepth, false) / (1 << bitdiff)), (Word)((1 << bits_per_pixel) - 1));
       }
       else {
         // shift to smaller. Do not shift as int, precision would be lost
         const double factor = (double)(1 << -bitdiff);
         if (bits_per_pixel == 16)
           return clip<Word, double>(compute_2(_x / factor, _y / factor, sbitdepth, false) * factor); // chroma = false: n/a for 10-16 bits
         else
           return min(clip<Word, double>(compute_2(_x / factor, _y / factor, sbitdepth, false) * factor), (Word)((1 << bits_per_pixel) - 1));
       }
     }
     else {
       const double factor = (double)((1 << sbitdepth) - 1) / ((1 << bits_per_pixel) - 1);
       if (bits_per_pixel == 16)
         return clip<Word, double>(compute_2(_x * factor, _y * factor, sbitdepth, false) / factor); // chroma = false: n/a for 10-16 bits
       else
         return min(clip<Word, double>(compute_2(_x * factor, _y * factor, sbitdepth, false) / factor), (Word)((1 << bits_per_pixel) - 1));
     }
   }

   // for lutspa, no autoconvert bitdepth here
   MT_FORCEINLINE Word compute_word_xy_safe(int _x, int _y, int _bitdepth)
   {
     return min(clip<Word, double>(compute_2(_x, _y, _bitdepth, false)), (Word)((1 << _bitdepth) - 1));
   }
   MT_FORCEINLINE Word compute_word_xy_safe_dblinput(double _x, double _y, int _bitdepth)
   {
     return min(clip<Word, double>(compute_2(_x, _y, _bitdepth, false)), (Word)((1 << _bitdepth) - 1));
   }

   template<int bits_per_pixel>
   MT_FORCEINLINE Word compute_word_xyz(int _x, int _y, int _z) {
     if (!scale_int || sbitdepth == bits_per_pixel) {
       if (bits_per_pixel == 16) // no min/max, faster
         return clip<Word, double>(compute_3(_x, _y, _z, bits_per_pixel, false)); // chroma = false: n/a for 8 bits
       else
         return min(clip<Word, double>(compute_3(_x, _y, _z, bits_per_pixel, false)), (Word)((1 << bits_per_pixel) - 1));
     }
     // 1.) convert 10-16 bits_per_pixel bit data to all_autoscale_bitdepth 2.) Compute 3.) Convert Back to bits_per_pixel bits
     if (!fullrange_autoscale) { // limited
       const int bitdiff = (sbitdepth - bits_per_pixel); // plus or minus
       if (bitdiff > 0) {
         // shift to bigger
         if (bits_per_pixel == 16)
           return clip<Word, double>(compute_3(_x << bitdiff, _y << bitdiff, _z << bitdiff, sbitdepth, false) / (1 << bitdiff)); // chroma = false: n/a for 10-16 bits
         else
           return min(clip<Word, double>(compute_3(_x << bitdiff, _y << bitdiff, _z << bitdiff, sbitdepth, false) / (1 << bitdiff)), (Word)((1 << bits_per_pixel) - 1));
       }
       else {
         // shift to smaller. Do not shift as int, precision would be lost
         const double factor = (double)(1 << -bitdiff);
         if (bits_per_pixel == 16)
           return clip<Word, double>(compute_3(_x / factor, _y / factor, _z / factor, sbitdepth, false) * factor); // chroma = false: n/a for 10-16 bits
         else
           return min(clip<Word, double>(compute_3(_x / factor, _y / factor, _z / factor, sbitdepth, false) * factor), (Word)((1 << bits_per_pixel) - 1));
       }
     }
     else {
       const double factor = (double)((1 << sbitdepth) - 1) / ((1 << bits_per_pixel) - 1);
       if (bits_per_pixel == 16)
         return clip<Word, double>(compute_3(_x * factor, _y * factor, _z * factor, sbitdepth, false) / factor); // chroma = false: n/a for 10-16 bits
       else
         return min(clip<Word, double>(compute_3(_x * factor, _y * factor, _z * factor, sbitdepth, false) / factor), (Word)((1 << bits_per_pixel) - 1));
     }
   }

   template<int bits_per_pixel>
   MT_FORCEINLINE Word compute_word_xyza(int _x, int _y, int _z, int _a) {
     if (!scale_int || sbitdepth == bits_per_pixel) {
       if (bits_per_pixel == 16) // no min/max, faster
         return clip<Word, double>(compute_4(_x, _y, _z, _a, bits_per_pixel, false));
       else
         return min(clip<Word, double>(compute_4(_x, _y, _z, _a, bits_per_pixel, false)), (Word)((1 << bits_per_pixel) - 1));
     }
     // 1.) convert 10-16 bits_per_pixel bit data to all_autoscale_bitdepth 2.) Compute 3.) Convert Back to bits_per_pixel bits
     if (!fullrange_autoscale) { // limited
       const int bitdiff = (sbitdepth - bits_per_pixel); // plus or minus
       if (bitdiff > 0) {
         // shift to bigger
         if (bits_per_pixel == 16)
           return clip<Word, double>(compute_4(_x << bitdiff, _y << bitdiff, _z << bitdiff, _a << bitdiff, sbitdepth, false) / (1 << bitdiff)); // chroma = false: n/a for 10-16 bits
         else
           return min(clip<Word, double>(compute_4(_x << bitdiff, _y << bitdiff, _z << bitdiff, _a << bitdiff, sbitdepth, false) / (1 << bitdiff)), (Word)((1 << bits_per_pixel) - 1));
       }
       else {
         // shift to smaller. Do not shift as int, precision would be lost
         const double factor = (double)(1 << -bitdiff);
         if (bits_per_pixel == 16)
           return clip<Word, double>(compute_4(_x / factor, _y / factor, _z / factor, _a / factor, sbitdepth, false) * factor); // chroma = false: n/a for 10-16 bits
         else
           return min(clip<Word, double>(compute_4(_x / factor, _y / factor, _z / factor, _a / factor, sbitdepth, false) * factor), (Word)((1 << bits_per_pixel) - 1));
       }
     }
     else {
       const double factor = (double)((1 << sbitdepth) - 1) / ((1 << bits_per_pixel) - 1);
       if (bits_per_pixel == 16)
         return clip<Word, double>(compute_4(_x * factor, _y * factor, _z * factor, _a * factor, sbitdepth, false) / factor); // chroma = false: n/a for 10-16 bits
       else
         return min(clip<Word, double>(compute_4(_x * factor, _y * factor, _z * factor, _a * factor, sbitdepth, false) / factor), (Word)((1 << bits_per_pixel) - 1));
     }
   }

   // single float parameter in, float result. _bitdepth is explicite 32
   MT_FORCEINLINE Float compute_float_x(double _x, bool _chroma)
   {
     float result;
     if (!scale_float || sbitdepth == 32) {
#ifdef FLOAT_CHROMA_IS_HALF_CENTERED
       if (clamp_float_i > 0) {
         result = (float)((compute_1(_x, 32, chroma)));
         return max(min(result, 1.0f), 0.0f);
       }
#endif
       result = shift_float && _chroma ? (float)(compute_1(_x + 0.5f, 32, _chroma) - 0.5f) : (float)(compute_1(_x, 32, _chroma));
     }
     else {

       // When lut expression writers have problems with writing proper one-size-fits-all general expressions special float input:
       // Float input in converted into 8-16 bit range, specified in float_autoscale_bitdepth parameter, 
       // but we are not truncating to integer, keep the whole thing in double
       // Then we pass this data converted into fake bitdepth (8..16).
       // scale up input, scale down result, note: we are keeping the chroma centers properly
       if (_chroma) {
         const double converted_input_x = float_input_scalefactor * (_x - chroma_center_f) + chroma_center_i;
         result = (float)((compute_1(converted_input_x, sbitdepth, _chroma)));
         result = float_input_invscalefactor * (result - chroma_center_i) + chroma_center_f;
       }
       else {
         result = (float)(float_input_invscalefactor * (compute_1(float_input_scalefactor * _x, sbitdepth, _chroma)));
       }
     }

     if(_chroma) // clamp_float == 2: clamp to 0..1 instead of -0.5..+0.5
       result = clamp_float_i == 1 ? max(min(result, chroma_hi_f), chroma_lo_f) : clamp_float_i == 2 ? max(min(result, 1.0f), 0.0f) : result;
     else
       result = clamp_float_i > 0 ? max(min(result, 1.0f), 0.0f) : result;

     return result;
   }

   MT_FORCEINLINE Float compute_float_xy(double _x, double _y, bool _chroma)
   {
     float result;
     if (!scale_float || sbitdepth == 32) {
#ifdef FLOAT_CHROMA_IS_HALF_CENTERED
       if (clamp_float_i > 0) {
         result = (float)((compute_2(_x, _y, 32, chroma)));
         return max(min(result, 1.0f), 0.0f);
       }
#endif
       result = shift_float && _chroma ? (float)(compute_2(_x + 0.5f, _y + 0.5f, 32, _chroma) - 0.5f) : (float)(compute_2(_x, _y, 32, _chroma));
     }
     else {

       if (_chroma) {
         const double converted_input_x = float_input_scalefactor * (_x - chroma_center_f) + chroma_center_i;
         const double converted_input_y = float_input_scalefactor * (_y - chroma_center_f) + chroma_center_i;
         result = (float)((compute_2(converted_input_x, converted_input_y, sbitdepth, _chroma)));
         result = float_input_invscalefactor * (result - chroma_center_i) + chroma_center_f;
       }
       else {
         result = (float)(float_input_invscalefactor * (compute_2(float_input_scalefactor * _x, float_input_scalefactor * _y, sbitdepth, _chroma)));
       }
     }

     if (_chroma) // clamp_float == 2: clamp to 0..1 instead of -0.5..+0.5
       result = clamp_float_i == 1 ? max(min(result, chroma_hi_f), chroma_lo_f) : clamp_float_i == 2 ? max(min(result, 1.0f), 0.0f) : result;
     else
       result = clamp_float_i > 0 ? max(min(result, 1.0f), 0.0f) : result;

     return result;
   }

   template<int _bitdepth>
   MT_FORCEINLINE Float compute_float_xy_intinput(int _x, int _y)
   {
     return (float)(compute_2(_x, _y, _bitdepth, false));
   }

   MT_FORCEINLINE Float compute_float_xyz(double _x, double _y, double _z, bool _chroma)
   {
     float result;
     if (!scale_float || sbitdepth == 32) {
#ifdef FLOAT_CHROMA_IS_HALF_CENTERED
       if (clamp_float_i > 0) {
         result = (float)((compute_3(_x, _y, _z, 32, chroma)));
         return max(min(result, 1.0f), 0.0f);
       }
#endif
       result = shift_float && _chroma ? (float)(compute_3(_x + 0.5f, _y + 0.5f, _z + 0.5f, 32, _chroma) - 0.5f) : (float)(compute_3(_x, _y, _z, 32, _chroma));
     }
     else {

       if (_chroma) {
         const double converted_input_x = float_input_scalefactor * (_x - chroma_center_f) + chroma_center_i;
         const double converted_input_y = float_input_scalefactor * (_y - chroma_center_f) + chroma_center_i;
         const double converted_input_z = float_input_scalefactor * (_z - chroma_center_f) + chroma_center_i;
         result = (float)((compute_3(converted_input_x, converted_input_y, converted_input_z, sbitdepth, _chroma)));
         result = float_input_invscalefactor * (result - chroma_center_i) + chroma_center_f;
       }
       else {
         result = (float)(float_input_invscalefactor * (compute_3(float_input_scalefactor * _x, float_input_scalefactor * _y, float_input_scalefactor * _z, sbitdepth, _chroma)));
       }
     }

     if (_chroma) // clamp_float == 2: clamp to 0..1 instead of -0.5..+0.5
       result = clamp_float_i == 1 ? max(min(result, chroma_hi_f), chroma_lo_f) : clamp_float_i == 2 ? max(min(result, 1.0f), 0.0f) : result;
     else
       result = clamp_float_i > 0 ? max(min(result, 1.0f), 0.0f) : result;

     return result;
   }

   MT_FORCEINLINE Float compute_float_xyza(double _x, double _y, double _z, double _a, bool _chroma)
   {
     float result;
     if (!scale_float || sbitdepth == 32) {
#ifdef FLOAT_CHROMA_IS_HALF_CENTERED
       if (clamp_float_i > 0) {
         result = (float)((compute_4(_x, _y, _z, _a, 32, chroma)));
         return max(min(result, 1.0f), 0.0f);
       }
#endif
       result = shift_float && _chroma ? (float)(compute_4(_x + 0.5f, _y + 0.5f, _z + 0.5f, _a + 0.5f, 32, _chroma) - 0.5f) : (float)(compute_4(_x, _y, _z, _a, 32, _chroma));
     }
     else {

       if (_chroma) {
         const double converted_input_x = float_input_scalefactor * (_x - chroma_center_f) + chroma_center_i;
         const double converted_input_y = float_input_scalefactor * (_y - chroma_center_f) + chroma_center_i;
         const double converted_input_z = float_input_scalefactor * (_z - chroma_center_f) + chroma_center_i;
         const double converted_input_a = float_input_scalefactor * (_a - chroma_center_f) + chroma_center_i;
         result = (float)((compute_4(converted_input_x, converted_input_y, converted_input_z, converted_input_a, sbitdepth, _chroma)));
         result = float_input_invscalefactor * (result - chroma_center_i) + chroma_center_f;
       }
       else {
         result = (float)(float_input_invscalefactor * (compute_4(float_input_scalefactor * _x, float_input_scalefactor * _y, float_input_scalefactor * _z, float_input_scalefactor * _a, sbitdepth, _chroma)));
       }
     }

     if (_chroma) // clamp_float == 2: clamp to 0..1 instead of -0.5..+0.5
       result = clamp_float_i == 1 ? max(min(result, chroma_hi_f), chroma_lo_f) : clamp_float_i == 2 ? max(min(result, 1.0f), 0.0f) : result;
     else
       result = clamp_float_i > 0 ? max(min(result, 1.0f), 0.0f) : result;

     return result;
   }

};

} } // namespace Parser, Filtering

#endif
