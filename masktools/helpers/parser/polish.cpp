#include "polish.h"
#include "../../../common/parser/parser.h"
#include <cstring>
#include <cstdlib>
#include <climits>
#include <cstdio>

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
      const size_t number_start = pos;
      if (peek() == '+' || peek() == '-')
         pos++;
      bool digits = false;
      while (is_digit(peek())) {
         pos++;
         digits = true;
      }
      bool dot = false;
      if (peek() == '.') {
         pos++;
         dot = true;
         while (is_digit(peek())) {
            pos++;
            digits = true;
         }
      }
      if (!digits) {
         pos = start;
         return false;
      }
      bool exponent = false;
      if (peek() == 'e' || peek() == 'E') {
         pos++;
         if (peek() == '+' || peek() == '-')
            pos++;
         if (!is_digit(peek())) {
            pos = start;
            return false;
         }
         while (is_digit(peek()))
            pos++;
         exponent = true;
      }
      if (!dot && !exponent) {
         pos = start;
         return false;
      }
      const double value = strtod(str.substr(number_start, pos - number_start).c_str(), nullptr);
      add_double(value);
      return true;
   }

   void add_double(double value)
   {
      char buffer[400];
      snprintf(buffer, sizeof(buffer), "%7f", value);
      buffer[19] = 0; // as before: at most 19 characters
      rpn.append(buffer).append(" ");
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
