#ifndef __Mt_Parser_H__
#define __Mt_Parser_H__

#include "../utils/utils.h"
#include "symbol.h"
#include <deque>

namespace Filtering { namespace Parser {

constexpr auto SYMBOL_SEPARATORS = " \r\n\t";
constexpr auto SYMBOL_SEPARATORS_COORD = " \r\n\t(),;.";

class Parser {
   String parsed_string;
   String error_string;
   int err_pos;
   std::deque<Symbol> elements;
   std::deque<Symbol> symbols;

public:
   Parser();
   Parser(const String &parsed_string, const String &separators);

public:
   Parser &addSymbol(const Symbol &symbol);
private:
   const Symbol *findSymbol(const String &value) const;
   Symbol stringToSymbol(const String &value, bool InvalidSymbolIsZero) const;
   Parser& parse_internal(const String& _parsed_string, const String& separators, bool InvalidSymbolIsZero);
public:
   Parser& parse(const String& parsed_string, const String& separators);
   Parser& parse_strict(const String& parsed_string, const String& separators);
   // ExpressionDependency flags of an expression (none when it is invalid: the regular parse reports that)
   int dependencies(const String& expression);

   String getParsedString() const;
   int count() const;
   int getErrorPos() const { return err_pos; }
   String getFailedSymbol() const { return error_string; }

   std::deque<Symbol> &getExpression() { return elements; }

};
Parser getDefaultParser();

} } // namespace Parser, Filtering

#endif
