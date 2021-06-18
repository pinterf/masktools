#include "parser.h"

using namespace Filtering;

Parser::Parser::Parser()
{
}

Parser::Parser::Parser(const String &parsed_string, const String &separators)
{
   parse(parsed_string, separators);
}

Parser::Parser &Parser::Parser::addSymbol(const Symbol &symbol)
{
   symbols.push_back(symbol);

   return *this;
}

const Parser::Symbol *Parser::Parser::findSymbol(const String &value) const
{
    // this one only finds exact symbols
    // special syntax elements should be treated here
    // Little problem: we should give unique Symbol to all 26*3 user variable method?
    // Better have introduce special symbol types instead
    //   'A'..'Z' variables
    //   'A@'..'Z@' variable store
    //   'A^'..'Z^' variable store and pop from stack
    //   dupN and swapN where N is an integer 0..
    // Constants that are bit-depth dependent are changed later in a 2nd pass to constants (NUMBER)
    for (auto &symbol: symbols) {
      /* temporarily off
        if (value.length >= 1) {
          if (value[0] >= 'A' && value[0] <= 'Z')
          {
            // user variable related
            if (value.length == 1)
            {
              const Symbol s = new Symbol("A", Symbol::VARIABLE, Symbol::);
            }
              
          }
        }
        */
        if (symbol.value == value || symbol.value2 == value) {
            return &symbol;
        }
    }

   return nullptr;
}

Parser::Symbol Parser::Parser::stringToSymbol(const String &value) const
{ 
    auto found = findSymbol(value);
    return found == nullptr
        ? Symbol(value, Symbol::NUMBER)
        : *found;
}

Parser::Parser &Parser::Parser::parse(const String &_parsed_string, const String &separators)
{
   this->parsed_string = _parsed_string;

   size_t nPos = _parsed_string.find_first_not_of(separators, 0);
   size_t nEndPos;

   elements.clear();

   while ( nPos != String::npos && (nEndPos = _parsed_string.find_first_of(separators, nPos)) != String::npos )
   {
      elements.push_back(stringToSymbol(_parsed_string.substr(nPos, nEndPos - nPos)));
      nPos = _parsed_string.find_first_not_of(separators, nEndPos);
   }

   if ( nPos != String::npos )
      elements.push_back(stringToSymbol(_parsed_string.substr(nPos)));

   return *this;
}

String Parser::Parser::getParsedString() const
{
   return parsed_string;
}

int Parser::Parser::count() const
{
   return elements.size();
}

Parser::Parser Parser::getDefaultParser()
{
   Parser parser;

   /* arithmetic operators */
   parser.addSymbol(Symbol::Addition).addSymbol(Symbol::Division).addSymbol(Symbol::Multiplication).addSymbol(Symbol::Substraction).addSymbol(Symbol::Modulo).addSymbol(Symbol::Power);
   /* comparison operators */
   parser.addSymbol(Symbol::Equal).addSymbol(Symbol::Equal2).addSymbol(Symbol::NotEqual).addSymbol(Symbol::Inferior).addSymbol(Symbol::InferiorStrict).addSymbol(Symbol::Superior).addSymbol(Symbol::SuperiorStrict);
   /* logic operators */
   parser.addSymbol(Symbol::And).addSymbol(Symbol::Or).addSymbol(Symbol::AndNot).addSymbol(Symbol::Xor);
   /* unsigned binary operators */
   parser.addSymbol(Symbol::AndUB).addSymbol(Symbol::OrUB).addSymbol(Symbol::XorUB).addSymbol(Symbol::NegateUB).addSymbol(Symbol::PosShiftUB).addSymbol(Symbol::NegShiftUB);
   /* signed binary operators */
   parser.addSymbol(Symbol::AndSB).addSymbol(Symbol::OrSB).addSymbol(Symbol::XorSB).addSymbol(Symbol::NegateSB).addSymbol(Symbol::PosShiftSB).addSymbol(Symbol::NegShiftSB);
   /* ternary operator */
   parser.addSymbol(Symbol::Interrogation);
   /* function */
   parser.addSymbol(Symbol::Abs).addSymbol(Symbol::Acos).addSymbol(Symbol::Asin).addSymbol(Symbol::Atan).addSymbol(Symbol::Cos).addSymbol(Symbol::Exp).addSymbol(Symbol::Log).addSymbol(Symbol::Sin).addSymbol(Symbol::Tan).addSymbol(Symbol::Min).addSymbol(Symbol::Max).addSymbol(Symbol::Clip);
   /* rounding */
   parser.addSymbol(Symbol::Round).addSymbol(Symbol::Floor).addSymbol(Symbol::Trunc).addSymbol(Symbol::Ceil);
   /* number */
   parser.addSymbol(Symbol::Pi);
   /* auto bitdepth conversion: BITDEPTH (bitdepth) and two functions */
   parser.addSymbol(Symbol::BITDEPTH).addSymbol(Symbol::SCRIPT_BITDEPTH);
   parser.addSymbol(Symbol::ScaleByShift).addSymbol(Symbol::ScaleByStretch);
   parser.addSymbol(Symbol::ScaleByShiftY).addSymbol(Symbol::ScaleByStretchY);
   /* swap and dup */
   parser.addSymbol(Symbol::Swap).addSymbol(Symbol::Dup);
   parser.addSymbol(Symbol::Dup0).addSymbol(Symbol::Dup1).addSymbol(Symbol::Dup2).addSymbol(Symbol::Dup3).addSymbol(Symbol::Dup4).addSymbol(Symbol::Dup5).addSymbol(Symbol::Dup6).addSymbol(Symbol::Dup7).addSymbol(Symbol::Dup8).addSymbol(Symbol::Dup9);
   parser.addSymbol(Symbol::Swap1).addSymbol(Symbol::Swap2).addSymbol(Symbol::Swap3).addSymbol(Symbol::Swap4).addSymbol(Symbol::Swap5).addSymbol(Symbol::Swap6).addSymbol(Symbol::Swap7).addSymbol(Symbol::Swap8).addSymbol(Symbol::Swap9);
   /* config commands for setting base bit depth of the script */
   parser.addSymbol(Symbol::SetScriptBitDepthI8).addSymbol(Symbol::SetScriptBitDepthI10).addSymbol(Symbol::SetScriptBitDepthI12);
   parser.addSymbol(Symbol::SetScriptBitDepthI14).addSymbol(Symbol::SetScriptBitDepthI16).addSymbol(Symbol::SetScriptBitDepthF32);
   parser.addSymbol(Symbol::SetFloatToClampUseI8Range).addSymbol(Symbol::SetFloatToClampUseI10Range).addSymbol(Symbol::SetFloatToClampUseI12Range).addSymbol(Symbol::SetFloatToClampUseI14Range);
   parser.addSymbol(Symbol::SetFloatToClampUseI16Range).addSymbol(Symbol::SetFloatToClampUseF32Range).addSymbol(Symbol::SetFloatToClampUseF32Range_2);
   /* special bit-depth adaptive constants */
   parser.addSymbol(Symbol::RANGE_HALF).addSymbol(Symbol::RANGE_MIN).addSymbol(Symbol::RANGE_MAX).addSymbol(Symbol::YRANGE_HALF).addSymbol(Symbol::YRANGE_MIN).addSymbol(Symbol::YRANGE_MAX).addSymbol(Symbol::RANGE_SIZE);
   parser.addSymbol(Symbol::YMIN).addSymbol(Symbol::YFMIN).addSymbol(Symbol::YMAX).addSymbol(Symbol::YFMAX);
   parser.addSymbol(Symbol::CMIN).addSymbol(Symbol::CFMIN).addSymbol(Symbol::CMAX).addSymbol(Symbol::CFMAX);

   /* Symbol X, X and Y, X and Y and Z are added additionally at before processing lut expression */
   /* like this:       
      Parser::Parser parser = Parser::getDefaultParser().addSymbol(Parser::Symbol::X).addSymbol(Parser::Symbol::Y);
  */

   return parser;
}