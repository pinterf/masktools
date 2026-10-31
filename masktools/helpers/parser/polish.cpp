#include "polish.h"
#include "../../../common/parser/parser.h"
#include <cstring>
#include <cstdlib>
#include <climits>
#include <cstdio>
#include <cfloat>
#include <cmath>

namespace Filtering { namespace MaskTools { namespace Helpers { namespace PolishConverter {

/*
   Infix -> reverse polish converter of mt_polish.

   A backtracking recursive descent parser. Operands and operators are appended to the output as soon as their
   rule matches, also inside an alternative that fails later (e.g. "min(x)" gives "x "), and parsing stops at
   the first character that does not fit (the rest is ignored). Whitespace is allowed between tokens.

   Grammar, in order of the alternatives:
      termter = term6 ( '?' termter ':' termter )*
      term6   = term5 ( ("!&" | "|" | "&" | "\xB0" | "@" | "&u" | "|u" | "\xB0u" | "@u" | ">>" | ">>u" | "<<" | "<<u"
                         | "&s" | "|s" | "\xB0s" | "@s" | ">>s" | "<<s") term5 )*
      term5   = term4 ( ("==" | "!=" | "<=" | ">=" | "<" | ">") term4 )*
      term4   = term3 ( ('+' | '-') term3 )*
      term3   = term2 ( '%' term2 )*
      term2   = term1 ( ('*' | '/') term1 )*
      term1   = factor ( '^' factor )*
      factor  = real (with '.' or exponent) | int | '(' termter ')' | constant | function | 'x' | 'y' | 'z' | 'a'
      function = name '(' termter (',' termter)* ')'
*/

namespace {

struct Function {
   const char *name;
   int nArgs;
};

const char *const constants[] = {
   "bitdepth", "sbitdepth", "range_half", "range_min", "range_max", "yrange_half", "yrange_min", "yrange_max",
   "range_size", "ymin", "ymax", "cmin", "cmax", "pi"
};

const Function functions[] = {
   { "@F", 1 }, { "scalef", 1 }, { "yscalef", 1 }, { "@B", 1 }, { "scaleb", 1 }, { "yscaleb", 1 },
   { "~u", 1 }, { "~s", 1 }, { "abs", 1 }, { "sin", 1 }, { "cos", 1 }, { "tan", 1 }, { "exp", 1 }, { "log", 1 },
   { "atan2", 2 }, { "atan", 1 }, { "acos", 1 }, { "asin", 1 }, { "round", 1 },
   { "dup0", 1 }, { "dup1", 1 }, { "dup2", 1 }, { "dup3", 1 }, { "dup4", 1 }, { "dup5", 1 }, { "dup6", 1 },
   { "dup7", 1 }, { "dup8", 1 }, { "dup9", 1 }, { "dup", 1 },
   { "swap1", 1 }, { "swap2", 1 }, { "swap3", 1 }, { "swap4", 1 }, { "swap5", 1 }, { "swap6", 1 }, { "swap7", 1 },
   { "swap8", 1 }, { "swap9", 1 }, { "swap", 1 },
   { "min", 2 }, { "max", 2 }, { "clip", 3 }
};

const char *const variables[] = { "x", "y", "z", "a" };

// binary operators per precedence level, term1 (highest) to term6
const char *const operators1[] = { "^" };
const char *const operators2[] = { "*", "/" };
const char *const operators3[] = { "%" };
const char *const operators4[] = { "+", "-" };
const char *const operators5[] = { "==", "!=", "<=", ">=", "<", ">" };
const char *const operators6[] = { "!&", "|", "&", "\xB0", "@", "&u", "|u", "\xB0u", "@u", ">>", ">>u", "<<", "<<u",
                                   "&s", "|s", "\xB0s", "@s", ">>s", "<<s" };

struct Level {
   const char *const *operators;
   int count;
};

const Level levels[] = {
   { nullptr, 0 },
   { operators1, sizeof(operators1) / sizeof(operators1[0]) },
   { operators2, sizeof(operators2) / sizeof(operators2[0]) },
   { operators3, sizeof(operators3) / sizeof(operators3[0]) },
   { operators4, sizeof(operators4) / sizeof(operators4[0]) },
   { operators5, sizeof(operators5) / sizeof(operators5[0]) },
   { operators6, sizeof(operators6) / sizeof(operators6[0]) },
};

class InfixToPolish {
   const String &str;
   size_t pos;
   String &rpn;

   static bool is_space(char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v' || c == '\f'; }
   static bool is_digit(char c) { return c >= '0' && c <= '9'; }
   char peek() const { return pos < str.size() ? str[pos] : '\0'; }

   void skip_spaces()
   {
      while (pos < str.size() && is_space(str[pos]))
         pos++;
   }

   void add(const char *name) { rpn.append(name).append(" "); }

   // whitespace, then the exact text
   bool literal(const char *text)
   {
      const size_t start = pos;
      skip_spaces();
      const size_t len = strlen(text);
      if (str.compare(pos, len, text) == 0 && pos + len <= str.size()) {
         pos += len;
         return true;
      }
      pos = start;
      return false;
   }

   // [+-]digits: an int, fails on overflow
   bool integer()
   {
      const size_t start = pos;
      skip_spaces();
      bool negative = false;
      if (peek() == '+' || peek() == '-') {
         negative = peek() == '-';
         pos++;
      }
      if (!is_digit(peek())) {
         pos = start;
         return false;
      }
      long long value = 0;
      while (is_digit(peek())) {
         value = value * 10 + (peek() - '0');
         if (value > (long long)INT_MAX + 1) {
            pos = start;
            return false;
         }
         pos++;
      }
      if (negative)
         value = -value;
      if (value > INT_MAX || value < INT_MIN) {
         pos = start;
         return false;
      }
      rpn.append(std::to_string((int)value)).append(" ");
      return true;
   }

   // [+-]digits[.digits][(e|E)[+-]digits] with a dot or an exponent; a leading or trailing dot is allowed
   bool real()
   {
      const size_t start = pos;
      skip_spaces();
      bool negative = false;
      if (peek() == '+' || peek() == '-') {
         negative = peek() == '-';
         pos++;
      }
      String digits;
      int fraction_digits = 0;
      while (is_digit(peek()))
         digits += str[pos++];
      bool dot = false;
      if (peek() == '.') {
         pos++;
         dot = true;
         while (is_digit(peek())) {
            digits += str[pos++];
            fraction_digits++;
         }
      }
      if (digits.empty()) {
         pos = start;
         return false;
      }
      int exponent = 0;
      bool has_exponent = false;
      if (peek() == 'e' || peek() == 'E') {
         pos++;
         bool negative_exponent = false;
         if (peek() == '+' || peek() == '-') {
            negative_exponent = peek() == '-';
            pos++;
         }
         if (!is_digit(peek())) {
            pos = start;
            return false;
         }
         while (is_digit(peek())) {
            if (exponent < 100000) // saturate, the result is 0 or inf way before that
               exponent = exponent * 10 + (peek() - '0');
            pos++;
         }
         if (negative_exponent)
            exponent = -exponent;
         has_exponent = true;
      }
      if (!dot && !has_exponent) {
         pos = start;
         return false;
      }
      // the digits go to strtod without a decimal point: correctly rounded and independent of the locale
      const String number = digits + "e" + std::to_string(exponent - fraction_digits);
      const double value = strtod(number.c_str(), nullptr);
      if (std::isinf(value)) { // out of the double range: no match, like an int above INT_MAX
         pos = start;
         return false;
      }
      rpn.append(double_to_string(negative ? -value : value)).append(" ");
      return true;
   }

   // Shortest decimal digits which read back to the same value, laid out like Python's repr() (1.5, 1.0,
   // 0.0001, 1e-05, 1.5e+16), '.' whatever the locale (as Avisynth+ Format). Finite values only.
   // Starting at DBL_DIG digits is enough: when the shortest form has k <= DBL_DIG digits, printing DBL_DIG
   // digits gives the same k digits followed by zeros, which are stripped. Subnormals start from 1 digit.
   // A power of two has a narrower rounding interval below than above: when the nearest decimal is below and
   // does not read back, the next one up may (2^-24: 5.960464477539063e-08, not 5.9604644775390625e-08).
   static String double_to_string(double d)
   {
      if (d == 0)
         return std::signbit(d) ? "-0.0" : "0.0";

      const double a = std::fabs(d);
      int binary_exponent;
      const bool power_of_two = std::frexp(a, &binary_exponent) == 0.5;
      // digits read back without a decimal point, so locale independent
      auto reads_back = [a](const String &digits, int exponent) {
         const String number = digits + "e" + std::to_string(exponent - (int)(digits.size() - 1));
         return strtod(number.c_str(), nullptr) == a;
      };

      String digits; // significant digits, without the decimal point
      int exponent = 0; // decimal exponent of the first digit
      const bool subnormal = a < DBL_MIN;
      for (int precision = subnormal ? 1 : DBL_DIG; precision <= 17; precision++) {
         char buffer[40];
         snprintf(buffer, sizeof(buffer), "%.*e", precision - 1, a); // e.g. "1.5e-05"
         // collect the digits, skipping the decimal point (which can be locale dependent)
         digits.clear();
         const char *p = buffer;
         for (; *p && *p != 'e' && *p != 'E'; p++)
            if (is_digit(*p))
               digits += *p;
         exponent = (int)strtol(p + 1, nullptr, 10);
         if (reads_back(digits, exponent))
            break;
         if (power_of_two) {
            // one unit up in the last digit
            String up = digits;
            int i = (int)up.size() - 1;
            for (; i >= 0 && up[i] == '9'; i--)
               up[i] = '0';
            int up_exponent = exponent;
            if (i >= 0)
               up[i]++;
            else {
               up.insert(up.begin(), '1');
               up.pop_back();
               up_exponent++;
            }
            if (reads_back(up, up_exponent)) {
               digits = up;
               exponent = up_exponent;
               break;
            }
         }
      }
      while (digits.size() > 1 && digits.back() == '0')
         digits.pop_back();

      String s = d < 0 ? "-" : "";
      const int ndigits = (int)digits.size();
      if (exponent < -4 || exponent >= 16) {
         // 1e-05, 1.5e+16
         s += digits[0];
         if (ndigits > 1)
            s += "." + digits.substr(1);
         const int abs_exponent = std::abs(exponent);
         s += exponent < 0 ? "e-" : "e+";
         if (abs_exponent < 10)
            s += '0';
         s += std::to_string(abs_exponent);
      }
      else if (exponent < 0) {
         // 0.0001, 0.015
         s += "0." + String(-exponent - 1, '0') + digits;
      }
      else if (ndigits > exponent + 1) {
         // 1.5, 123.25
         s += digits.substr(0, exponent + 1) + "." + digits.substr(exponent + 1);
      }
      else {
         // 1.0, 100.0, 1234567890123456.0
         s += digits + String(exponent + 1 - ndigits, '0') + ".0";
      }
      return s;
   }

   bool function()
   {
      for (const Function &f : functions) {
         const size_t start = pos;
         if (literal(f.name) && literal("(") && termter()) {
            bool ok = true;
            for (int i = 1; i < f.nArgs && ok; i++)
               ok = literal(",") && termter();
            if (ok && literal(")")) {
               add(f.name);
               return true;
            }
         }
         pos = start;
      }
      return false;
   }

   bool factor()
   {
      const size_t start = pos;
      if (real() || integer())
         return true;
      if (literal("(") && termter() && literal(")"))
         return true;
      pos = start;
      for (const char *c : constants) {
         if (literal(c)) {
            add(c);
            return true;
         }
      }
      if (function())
         return true;
      for (const char *v : variables) {
         if (literal(v)) {
            add(v);
            return true;
         }
      }
      return false;
   }

   bool term(int level)
   {
      if (level == 0)
         return factor();
      if (!term(level - 1))
         return false;
      for (;;) {
         const size_t start = pos;
         bool matched = false;
         for (int i = 0; i < levels[level].count && !matched; i++) {
            const char *op = levels[level].operators[i];
            if (literal(op) && term(level - 1)) {
               add(op);
               matched = true;
            }
            else
               pos = start;
         }
         if (!matched)
            return true;
      }
   }

   bool termter()
   {
      if (!term(6))
         return false;
      for (;;) {
         const size_t start = pos;
         if (literal("?") && termter() && literal(":") && termter())
            add("?");
         else {
            pos = start;
            return true;
         }
      }
   }

public:
   InfixToPolish(const String &str, String &rpn) : str(str), pos(0), rpn(rpn) {}
   void convert() { termter(); }
};

} // namespace

String Converter(const String &str)
{
   String result;
   InfixToPolish(str, result).convert();
   return result;
}

String Infix(const String &str)
{
   Parser::Parser parser = Parser::getDefaultParser().addSymbol(Parser::Symbol::X).addSymbol(Parser::Symbol::Y).addSymbol(Parser::Symbol::Z);
   parser.parse(str, Parser::SYMBOL_SEPARATORS);
   Parser::Context ctx(parser.getExpression());
   return ctx.infix();
}

} } } }
